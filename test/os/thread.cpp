#include <atomic>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include <gtest/gtest.h>

#include "helpers.hpp"
#include "os/event.hpp"
#include "os/mutex.hpp"
#include "os/thread.hpp"

using os_test::Contains;
using os_test::Gate;
using os_test::Line;
using os_test::WaitUntil;

namespace
{

std::string DumpThreads()
{
    return os_test::Dump(os::Thread::dump_info);
}

// What a test thread tells the test, and what the test tells it.
struct Job
{
    Gate gate; // the thread does not return until the test opens this
    std::atomic<bool> started{false};
    std::atomic<bool> done{false};
    std::atomic<os::Thread*> current{nullptr}; // what GetCurrent() returned in the thread
    std::atomic<int> value{0};
    os::Mutex* mutex = nullptr;
    unsigned long sleep = 0;
    std::atomic<long> elapsed_ms{-1};
};

// Waits for the gate. Nothing in here changes the state the os classes track for the thread.
void ParkedUntilGateOpens(void* param)
{
    Job* job = static_cast<Job*>(param);
    job->current = os::Thread::GetCurrent();
    job->started = true;
    job->gate.Wait();
    job->done = true;
}

void ReturnAtOnce(void* param)
{
    Job* job = static_cast<Job*>(param);
    job->current = os::Thread::GetCurrent();
    job->started = true;
    job->done = true;
}

void SetValue(void* param)
{
    static_cast<Job*>(param)->value = 42;
    static_cast<Job*>(param)->done = true;
}

void TakeTheMutex(void* param)
{
    Job* job = static_cast<Job*>(param);
    job->started = true;
    job->mutex->Take(__FILE__, __LINE__);
    job->mutex->Give();
    job->done = true;
}

void SleepMilliseconds(void* param)
{
    Job* job = static_cast<Job*>(param);
    job->started = true;
    auto start = std::chrono::steady_clock::now();
    os::Thread::Sleep(job->sleep, __FILE__, __LINE__);
    auto elapsed = std::chrono::steady_clock::now() - start;
    job->elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    job->done = true;
}

void SleepMicroseconds(void* param)
{
    Job* job = static_cast<Job*>(param);
    job->started = true;
    auto start = std::chrono::steady_clock::now();
    os::Thread::USleep(job->sleep, __FILE__, __LINE__);
    auto elapsed = std::chrono::steady_clock::now() - start;
    job->elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    job->done = true;
}

// Create() calls Thread::Initialize() the first time. Run one thread so that has happened.
void EnsureInitialized()
{
    static bool initialized = false;
    if (!initialized)
    {
        Job job;
        os::Thread thread;
        thread.Create(ReturnAtOnce, "initializer", 0, 0, &job);
        WaitUntil([&] { return job.done.load(); });
        initialized = true;
    }
}

// A thread that is registered and kept alive while the test looks at it.
class ParkedThread
{
public:
    explicit ParkedThread(const char* name)
    {
        thread.Create(ParkedUntilGateOpens, name, 0, 0, &job);
        WaitUntil([&] { return job.started.load(); });
    }
    ~ParkedThread()
    {
        job.gate.Open();
        WaitUntil([&] { return job.done.load(); });
    }

    Job job;
    os::Thread thread;
};

// A Thread built in memory that is full of 'X', so that anything the constructor does not write
// stands out.
class ThreadInDirtyMemory
{
public:
    ThreadInDirtyMemory()
    {
        memset(m_storage, 'X', sizeof(m_storage));
        m_thread = new (m_storage) os::Thread();
    }
    ~ThreadInDirtyMemory() { m_thread->~Thread(); }
    os::Thread* operator->() { return m_thread; }

private:
    alignas(os::Thread) unsigned char m_storage[sizeof(os::Thread)];
    os::Thread* m_thread;
};

} // namespace

TEST(ThreadTest, CreateRunsTheFunctionWithItsParameter)
{
    Job job;
    os::Thread thread;
    EXPECT_EQ(0, thread.Create(SetValue, "set-value", 0, 0, &job));
    ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    EXPECT_EQ(42, job.value.load());
}

TEST(ThreadTest, NameIsTheOneGivenToCreate)
{
    ParkedThread parked("named-thread");
    EXPECT_STREQ("named-thread", parked.thread.GetName());
}

TEST(ThreadTest, ALongNameIsTruncated)
{
    std::string name(100, 'x');
    ParkedThread parked(name.c_str());
    EXPECT_EQ(static_cast<size_t>(os::Thread::NAME_LENGTH_MAX - 1),
              strlen(parked.thread.GetName()));
    EXPECT_EQ(name.substr(0, os::Thread::NAME_LENGTH_MAX - 1), parked.thread.GetName());
}

TEST(ThreadTest, TheLongestNameThatFitsIsKept)
{
    std::string name(os::Thread::NAME_LENGTH_MAX - 1, 'n');
    ParkedThread parked(name.c_str());
    EXPECT_EQ(name, parked.thread.GetName());
}

TEST(ThreadTest, ANameOneCharacterTooLongLosesTheLastCharacter)
{
    std::string name(os::Thread::NAME_LENGTH_MAX, 'n');
    name.back() = 'L';
    ParkedThread parked(name.c_str());
    EXPECT_EQ(name.substr(0, os::Thread::NAME_LENGTH_MAX - 1), parked.thread.GetName());
}

TEST(ThreadTest, ANullNameIsTheEmptyString)
{
    ParkedThread parked(nullptr);
    EXPECT_STREQ("", parked.thread.GetName());

    // It is listed with an empty name, not with "(null)"
    std::string text = DumpThreads();
    EXPECT_FALSE(Contains(text, "(null)"));
    EXPECT_TRUE(Contains(text, std::string(20, ' ') + "|"));
}

TEST(ThreadTest, TheNameOfAThreadThatWasNeverCreatedIsTheEmptyString)
{
    ThreadInDirtyMemory thread;
    const char* name = thread->GetName();
    EXPECT_STREQ("", name);
    // Every byte of the buffer was written, not just the first
    EXPECT_EQ(nullptr, memchr(name, 'X', os::Thread::NAME_LENGTH_MAX));
}

TEST(ThreadTest, PriorityIsKept)
{
    Job job;
    os::Thread thread;
    thread.Create(ReturnAtOnce, "priority", 0, 7, &job);
    EXPECT_EQ(7, thread.Priority);
    WaitUntil([&] { return job.done.load(); });
}

TEST(ThreadTest, GetCurrentInsideAThreadIsThatThread)
{
    ParkedThread parked("get-current");
    EXPECT_EQ(&parked.thread, parked.job.current.load());
}

TEST(ThreadTest, EachThreadSeesItself)
{
    ParkedThread first("current-first");
    ParkedThread second("current-second");
    EXPECT_EQ(&first.thread, first.job.current.load());
    EXPECT_EQ(&second.thread, second.job.current.load());
    EXPECT_NE(first.job.current.load(), second.job.current.load());
}

TEST(ThreadTest, GetCurrentIsNullOnAThreadThatIsNotAnOsThread)
{
    EnsureInitialized();
    EXPECT_EQ(nullptr, os::Thread::GetCurrent());

    os::Thread* seen = reinterpret_cast<os::Thread*>(1);
    std::thread other([&] { seen = os::Thread::GetCurrent(); });
    other.join();
    EXPECT_EQ(nullptr, seen);
}

TEST(ThreadTest, ManyThreadsRunAtTheSameTime)
{
    const int count = 10;
    std::vector<std::unique_ptr<ParkedThread>> threads;
    for (int i = 0; i < count; i++)
    {
        threads.emplace_back(new ParkedThread("running-together"));
    }
    for (auto& parked : threads)
    {
        EXPECT_TRUE(parked->job.started.load());
        EXPECT_FALSE(parked->job.done.load());
    }
}

TEST(ThreadTest, DumpInfoHasAHeader)
{
    std::string text = DumpThreads();
    EXPECT_TRUE(Contains(text, "--------------------+------------------------------------\n"));
    EXPECT_TRUE(Contains(text, "Thread Name         | State\n"));
    EXPECT_TRUE(
        Contains(text, "\"Reading\" means that the thread is reading a device or network socket"));
}

TEST(ThreadTest, DumpInfoListsTheThreadThatCalledInitialize)
{
    EnsureInitialized();
    EXPECT_TRUE(Contains(DumpThreads(), "main"));
}

TEST(ThreadTest, DumpInfoListsARunningThread)
{
    ParkedThread parked("listed-thread");
    std::string row = Line(DumpThreads(), "listed-thread");
    EXPECT_FALSE(row.empty());
    // The name is padded to 20 characters
    EXPECT_EQ("listed-thread       |", row.substr(0, 21));
}

TEST(ThreadTest, DumpInfoListsEveryThread)
{
    ParkedThread first("list-all-first");
    ParkedThread second("list-all-second");
    std::string text = DumpThreads();
    EXPECT_TRUE(Contains(text, "list-all-first"));
    EXPECT_TRUE(Contains(text, "list-all-second"));
}

TEST(ThreadTest, DumpInfoShowsEachState)
{
    ParkedThread parked("state-thread");
    os::Mutex mutex("state-mutex");
    os::Event event("state-event");

    parked.thread.SetState(os::Thread::INIT, "file.cpp", 1, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|init"))
        << Line(DumpThreads(), "state-thread");

    parked.thread.SetState(os::Thread::RUNNING, "file.cpp", 2, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|running"));

    parked.thread.SetState(os::Thread::PENDING_MUTEX, "file.cpp", 3, &mutex);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|pending on mutex \"state-mutex\""));

    parked.thread.SetState(os::Thread::PENDING_MUTEX, "file.cpp", 4, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|pending on mutex \"UNKNOWN\""));

    parked.thread.SetState(os::Thread::PENDING_EVENT, "file.cpp", 5, &event);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|pending event \"state-event\""));

    parked.thread.SetState(os::Thread::PENDING_EVENT, "file.cpp", 6, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|pending event \"UNKNOWN\""));

    parked.thread.USleepTime = 1234;
    parked.thread.SetState(os::Thread::SLEEPING, "sleeper.cpp", 77, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "state-thread"), "|sleeping 1234 at sleeper.cpp 77"))
        << Line(DumpThreads(), "state-thread");
}

TEST(ThreadTest, ClearStateGoesBackToRunning)
{
    ParkedThread parked("cleared-thread");
    parked.thread.SetState(os::Thread::PENDING_EVENT, "file.cpp", 1, nullptr);
    EXPECT_TRUE(Contains(Line(DumpThreads(), "cleared-thread"), "pending event"));
    parked.thread.ClearState();
    EXPECT_TRUE(Contains(Line(DumpThreads(), "cleared-thread"), "|running"));
}

TEST(ThreadTest, DumpInfoShowsAThreadThatIsWaitingForAMutex)
{
    os::Mutex mutex("contended-mutex");
    mutex.Take(__FILE__, __LINE__);

    Job job;
    job.mutex = &mutex;
    os::Thread thread;
    thread.Create(TakeTheMutex, "mutex-waiter", 0, 0, &job);

    EXPECT_TRUE(WaitUntil(
        [&] {
            return Contains(Line(DumpThreads(), "mutex-waiter"),
                            "pending on mutex \"contended-mutex\"");
        }));
    EXPECT_FALSE(job.done.load());

    mutex.Give();
    ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    EXPECT_FALSE(Contains(Line(DumpThreads(), "mutex-waiter"), "pending on mutex"));
}

TEST(ThreadTest, ThreadIsRemovedFromDumpInfoWhenItsFunctionReturns)
{
    ParkedThread* parked = new ParkedThread("returning-thread");
    EXPECT_TRUE(Contains(DumpThreads(), "returning-thread"));

    parked->job.gate.Open();
    ASSERT_TRUE(WaitUntil([&] { return parked->job.done.load(); }));
    EXPECT_TRUE(WaitUntil([&] { return !Contains(DumpThreads(), "returning-thread"); }));
    delete parked;
}

TEST(ThreadTest, DestroyedThreadIsRemovedFromDumpInfo)
{
    {
        ParkedThread parked("destroyed-thread");
        EXPECT_TRUE(Contains(DumpThreads(), "destroyed-thread"));
    }
    EXPECT_TRUE(WaitUntil([&] { return !Contains(DumpThreads(), "destroyed-thread"); }));
}

TEST(ThreadTest, ManyShortLivedThreadsLeaveNothingBehind)
{
    for (int i = 0; i < 200; i++)
    {
        Job job;
        os::Thread thread;
        thread.Create(ReturnAtOnce, "quick-thread", 0, 0, &job);
        ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    }
    EXPECT_FALSE(Contains(DumpThreads(), "quick-thread"));
}

TEST(ThreadTest, MoreThreadsThanTheListHoldsAllStillRun)
{
    // The list that dump_info() uses has room for 48 threads. The rest are not listed but run.
    const int count = 60;
    std::vector<std::unique_ptr<ParkedThread>> threads;
    for (int i = 0; i < count; i++)
    {
        threads.emplace_back(new ParkedThread("crowd"));
    }
    for (auto& parked : threads)
    {
        EXPECT_TRUE(parked->job.started.load());
    }

    std::string text = DumpThreads();
    int listed = 0;
    for (size_t at = text.find("crowd"); at != std::string::npos; at = text.find("crowd", at + 1))
    {
        listed++;
    }
    EXPECT_GT(listed, 0);
    EXPECT_LT(listed, count);

    for (auto& parked : threads)
    {
        parked->job.gate.Open();
    }
    for (auto& parked : threads)
    {
        EXPECT_TRUE(WaitUntil([&] { return parked->job.done.load(); }));
    }
    threads.clear();
    EXPECT_FALSE(Contains(DumpThreads(), "crowd"));
}

TEST(ThreadTest, SleepWaitsAtLeastThatLongOnAnOsThread)
{
    Job job;
    job.sleep = 150;
    os::Thread thread;
    thread.Create(SleepMilliseconds, "sleeper", 0, 0, &job);

    // While it sleeps, the thread says for how long
    EXPECT_TRUE(WaitUntil([&] { return thread.USleepTime == 150000; }, 1000));
    ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    EXPECT_GE(job.elapsed_ms.load(), 140);
    EXPECT_LT(job.elapsed_ms.load(), 3000);
    EXPECT_EQ(0u, thread.USleepTime);
}

TEST(ThreadTest, USleepWaitsAtLeastThatLongOnAnOsThread)
{
    Job job;
    job.sleep = 120000;
    os::Thread thread;
    thread.Create(SleepMicroseconds, "micro-sleeper", 0, 0, &job);

    EXPECT_TRUE(WaitUntil([&] { return thread.USleepTime == 120000; }, 1000));
    ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    EXPECT_GE(job.elapsed_ms.load(), 110);
    EXPECT_LT(job.elapsed_ms.load(), 3000);
    EXPECT_EQ(0u, thread.USleepTime);
}

TEST(ThreadTest, SleepAlsoWorksOnAThreadThatIsNotAnOsThread)
{
    EnsureInitialized();
    auto start = std::chrono::steady_clock::now();
    os::Thread::Sleep(60, __FILE__, __LINE__);
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(55));
    EXPECT_LT(elapsed, std::chrono::seconds(3));

    std::atomic<long> other_ms(-1);
    std::thread other(
        [&]
        {
            auto begin = std::chrono::steady_clock::now();
            os::Thread::Sleep(60, __FILE__, __LINE__);
            other_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - begin)
                           .count();
        });
    other.join();
    EXPECT_GE(other_ms.load(), 55);
}

TEST(ThreadTest, USleepAlsoWorksOnAThreadThatIsNotAnOsThread)
{
    EnsureInitialized();
    auto start = std::chrono::steady_clock::now();
    os::Thread::USleep(60000, __FILE__, __LINE__);
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(55));
    EXPECT_LT(elapsed, std::chrono::seconds(3));

    std::atomic<long> other_ms(-1);
    std::thread other(
        [&]
        {
            auto begin = std::chrono::steady_clock::now();
            os::Thread::USleep(60000, __FILE__, __LINE__);
            other_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - begin)
                           .count();
        });
    other.join();
    EXPECT_GE(other_ms.load(), 55);
}

TEST(ThreadTest, SleepingForNothingReturnsAtOnce)
{
    auto start = std::chrono::steady_clock::now();
    os::Thread::Sleep(0, __FILE__, __LINE__);
    os::Thread::USleep(0, __FILE__, __LINE__);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds(500));
}

TEST(ThreadTest, WaitForExitOnAThreadThatHasFinishedReturns)
{
    Job job;
    os::Thread thread;
    thread.Create(ReturnAtOnce, "finished", 0, 0, &job);
    ASSERT_TRUE(WaitUntil([&] { return job.done.load(); }));
    thread.WaitForExit();
    thread.WaitForExit(100);
    SUCCEED();
}

// KNOWN BUG: on Linux WaitForExit() does nothing, so it returns at once even though the thread
// is still running. Windows waits. Enable this test when WaitForExit() is implemented here.
TEST(ThreadTest, DISABLED_WaitForExitWaitsForTheThread)
{
    ParkedThread parked("still-running");
    auto start = std::chrono::steady_clock::now();
    parked.thread.WaitForExit(100);
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(90));
}

// Until the first thread is created the key that GetCurrent() reads does not exist. It is zero,
// which is a valid key if another library, googletest for one, has created keys of its own. The
// function used to return that library's data, and Mutex::Take() then wrote thread state into it,
// which corrupted the heap. A fresh process is needed because Initialize() only runs once.
TEST(ThreadDeathTest, GetCurrentBeforeAnyThreadExistsDoesNotReadAnotherLibrarysData)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_EXIT(
        {
            static int sentinel;
            pthread_key_t key;
            pthread_key_create(&key, nullptr); // so that at least key 0 exists
            for (pthread_key_t each = 0; each < 64; each++)
            {
                pthread_setspecific(each, &sentinel);
            }
            _exit(os::Thread::GetCurrent() == nullptr ? 0 : 3);
        },
        ::testing::ExitedWithCode(0),
        "");
}
