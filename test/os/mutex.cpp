#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "helpers.hpp"
#include "os/mutex.hpp"
#include "os/thread.hpp"

using os_test::Contains;
using os_test::Line;
using os_test::WaitUntil;

namespace
{

std::string DumpMutexes()
{
    return os_test::Dump(os::Mutex::dump_info);
}

// Work for an os::Thread: take the mutex, bump the counter and give the mutex back.
struct SharedCounter
{
    os::Mutex* mutex;
    int counter;
    int increments;
    std::atomic<int> finished;
};

void IncrementUnderMutex(void* param)
{
    SharedCounter* shared = static_cast<SharedCounter*>(param);
    for (int i = 0; i < shared->increments; i++)
    {
        shared->mutex->Take(__FILE__, __LINE__);
        shared->counter++;
        shared->mutex->Give();
    }
    shared->finished++;
}

} // namespace

TEST(MutexTest, NameIsReturned)
{
    os::Mutex mutex("name-test");
    EXPECT_STREQ("name-test", mutex.GetName());
}

TEST(MutexTest, ANullNameIsTheEmptyString)
{
    os::Mutex mutex(nullptr);
    ASSERT_NE(nullptr, mutex.GetName());
    EXPECT_STREQ("", mutex.GetName());

    // It still works, and it is listed with an empty name, not with "(null)"
    mutex.Take(__FILE__, __LINE__);
    std::string text = DumpMutexes();
    mutex.Give();
    EXPECT_FALSE(Contains(text, "(null)"));
    EXPECT_TRUE(Contains(text, std::string(20, ' ') + "|"));
}

TEST(MutexTest, TakeAndGiveOnOneThread)
{
    os::Mutex mutex("single-thread");
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();
}

TEST(MutexTest, CanBeTakenAgainByTheThreadThatHoldsIt)
{
    os::Mutex mutex("recursive");
    mutex.Take(__FILE__, __LINE__);
    mutex.Take(__FILE__, __LINE__);
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();
    mutex.Give();
    mutex.Give();

    // All three gives were needed, and are enough: another thread can now take it.
    std::atomic<bool> got(false);
    std::thread other(
        [&]
        {
            mutex.Take(__FILE__, __LINE__);
            got = true;
            mutex.Give();
        });
    EXPECT_TRUE(WaitUntil([&] { return got.load(); }));
    other.join();
}

TEST(MutexTest, StaysTakenUntilEveryTakeHasBeenGivenBack)
{
    os::Mutex mutex("held-twice");
    mutex.Take(__FILE__, __LINE__);
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();

    std::atomic<bool> attempting(false);
    std::atomic<bool> got(false);
    std::thread other(
        [&]
        {
            attempting = true;
            mutex.Take(__FILE__, __LINE__);
            got = true;
            mutex.Give();
        });
    ASSERT_TRUE(WaitUntil([&] { return attempting.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(got.load()) << "the other thread got a mutex that is still taken once";

    mutex.Give();
    EXPECT_TRUE(WaitUntil([&] { return got.load(); }));
    other.join();
}

TEST(MutexTest, TakenMutexBlocksOtherThreads)
{
    os::Mutex mutex("blocking");
    mutex.Take(__FILE__, __LINE__);

    std::atomic<bool> attempting(false);
    std::atomic<bool> got(false);
    std::thread other(
        [&]
        {
            attempting = true;
            mutex.Take(__FILE__, __LINE__);
            got = true;
            mutex.Give();
        });
    ASSERT_TRUE(WaitUntil([&] { return attempting.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(got.load());

    mutex.Give();
    EXPECT_TRUE(WaitUntil([&] { return got.load(); }));
    other.join();
}

TEST(MutexTest, GiveWithoutTakeIsHarmless)
{
    os::Mutex mutex("unbalanced-give");
    mutex.Give();
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();
}

TEST(MutexTest, ProtectsDataSharedByThreadsThatAreNotOsThreads)
{
    os::Mutex mutex("std-threads");
    int counter = 0;
    const int threads = 4;
    const int increments = 20000;

    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++)
    {
        workers.emplace_back(
            [&]
            {
                for (int j = 0; j < increments; j++)
                {
                    mutex.Take(__FILE__, __LINE__);
                    counter++;
                    mutex.Give();
                }
            });
    }
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ(threads * increments, counter);
}

TEST(MutexTest, ProtectsDataSharedByOsThreads)
{
    os::Mutex mutex("os-threads");
    SharedCounter shared;
    shared.mutex = &mutex;
    shared.counter = 0;
    shared.increments = 5000;
    shared.finished = 0;

    const int threads = 4;
    std::vector<std::unique_ptr<os::Thread>> workers;
    for (int i = 0; i < threads; i++)
    {
        workers.emplace_back(new os::Thread());
        workers.back()->Create(IncrementUnderMutex, "mutex-worker", 0, 0, &shared);
    }
    ASSERT_TRUE(WaitUntil([&] { return shared.finished.load() == threads; }));
    EXPECT_EQ(threads * shared.increments, shared.counter);
}

TEST(MutexTest, DumpInfoHasAHeader)
{
    std::string text = DumpMutexes();
    EXPECT_TRUE(Contains(text, " Name               | State | Owner              | Line | File"));
}

TEST(MutexTest, DumpInfoListsTheMutex)
{
    os::Mutex mutex("listed-mutex");
    std::string text = DumpMutexes();
    EXPECT_TRUE(Contains(text, "listed-mutex"));
}

TEST(MutexTest, DumpInfoShowsWhereTheMutexWasTaken)
{
    os::Mutex mutex("owned-mutex");

    mutex.Take("owner_file.cpp", 321);
    std::string taken = Line(DumpMutexes(), "owned-mutex");
    EXPECT_TRUE(Contains(taken, "owner_file.cpp")) << taken;
    EXPECT_TRUE(Contains(taken, "321")) << taken;

    mutex.Give();
    std::string given = Line(DumpMutexes(), "owned-mutex");
    EXPECT_FALSE(Contains(given, "owner_file.cpp")) << given;
    EXPECT_FALSE(Contains(given, "321")) << given;
    EXPECT_TRUE(Contains(given, "-1")) << given;
}

TEST(MutexTest, DumpInfoRowIsAlignedInColumns)
{
    os::Mutex mutex("aligned");
    mutex.Take("f.cpp", 7);
    std::string row = Line(DumpMutexes(), "aligned");
    mutex.Give();

    // name | state | owner | line | file
    EXPECT_EQ("aligned             |       |                    |7     |f.cpp", row);
}

TEST(MutexTest, DumpInfoListsEveryMutex)
{
    os::Mutex first("list-all-first");
    os::Mutex second("list-all-second");
    os::Mutex third("list-all-third");
    std::string text = DumpMutexes();
    EXPECT_TRUE(Contains(text, "list-all-first"));
    EXPECT_TRUE(Contains(text, "list-all-second"));
    EXPECT_TRUE(Contains(text, "list-all-third"));
}

TEST(MutexTest, DestroyedMutexIsRemovedFromDumpInfo)
{
    {
        os::Mutex mutex("short-lived-mutex");
        EXPECT_TRUE(Contains(DumpMutexes(), "short-lived-mutex"));
    }
    EXPECT_FALSE(Contains(DumpMutexes(), "short-lived-mutex"));
}

TEST(MutexTest, DestroyedMutexSlotCanBeReused)
{
    // Many create and destroy cycles must not use up the room in the list.
    for (int i = 0; i < 100; i++)
    {
        os::Mutex mutex("recycled-mutex");
        mutex.Take(__FILE__, __LINE__);
        mutex.Give();
    }
    os::Mutex last("recycled-last");
    EXPECT_TRUE(Contains(DumpMutexes(), "recycled-last"));
    EXPECT_FALSE(Contains(DumpMutexes(), "recycled-mutex"));
}

TEST(MutexTest, MoreMutexesThanTheListHoldsAllStillWork)
{
    // The list that dump_info() uses has room for 16 mutexes. The rest are not listed but work.
    const int count = 40;
    std::vector<std::string> names;
    for (int i = 0; i < count; i++)
    {
        names.push_back("overflow-" + std::to_string(i));
    }
    {
        std::vector<std::unique_ptr<os::Mutex>> mutexes;
        for (int i = 0; i < count; i++)
        {
            mutexes.emplace_back(new os::Mutex(names[i].c_str()));
        }
        for (auto& mutex : mutexes)
        {
            mutex->Take(__FILE__, __LINE__);
            mutex->Give();
        }

        std::string text = DumpMutexes();
        int listed = 0;
        for (const std::string& name : names)
        {
            if (Contains(text, name))
            {
                listed++;
            }
        }
        EXPECT_GT(listed, 0);
        EXPECT_LT(listed, count);
    }

    std::string text = DumpMutexes();
    for (const std::string& name : names)
    {
        EXPECT_FALSE(Contains(text, name)) << name << " is still listed after it was destroyed";
    }
}
