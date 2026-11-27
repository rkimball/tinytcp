#include <atomic>
#include <memory>
#include <new>
#include <string.h>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "helpers.hpp"
#include "os/event.hpp"
#include "os/thread.hpp"

using os_test::Contains;
using os_test::Line;
using os_test::WaitUntil;

namespace
{

std::string DumpEvents()
{
    return os_test::Dump(os::Event::dump_info);
}

std::string DumpThreads()
{
    return os_test::Dump(os::Thread::dump_info);
}

// A thread that waits for an event, and what it has done so far.
struct Waiter
{
    os::Event* event;
    std::atomic<bool> started;
    std::atomic<bool> done;
    std::atomic<bool> got;
};

void WaitForEvent(void* param)
{
    Waiter* waiter = static_cast<Waiter*>(param);
    waiter->started = true;
    waiter->got = waiter->event->Wait(__FILE__, __LINE__);
    waiter->done = true;
}

// The size of the name buffer in os::Event, which is private. Keep in step with NAME_LENGTH_MAX.
const size_t EventNameSize = 80;

// An Event built in memory that is full of 'X', so that anything the constructor does not write
// stands out.
class EventInDirtyMemory
{
public:
    explicit EventInDirtyMemory(const char* name)
    {
        memset(m_storage, 'X', sizeof(m_storage));
        m_event = new (m_storage) os::Event(name);
    }
    ~EventInDirtyMemory() { m_event->~Event(); }
    os::Event* operator->() { return m_event; }

private:
    alignas(os::Event) unsigned char m_storage[sizeof(os::Event)];
    os::Event* m_event;
};

// True if every byte of the name buffer was written: nothing is left over from before, and there
// is a terminator inside the buffer.
bool NameIsComplete(const char* name, size_t size)
{
    return memchr(name, 'X', size) == nullptr && memchr(name, 0, size) != nullptr;
}

} // namespace

TEST(EventTest, ConstructorTest)
{
    // Test case 1: Check if the constructor initializes the name correctly
    const char* name1 = "Event1";
    os::Event event1(name1);
    EXPECT_STREQ(name1, event1.GetName());

    // Test case 2: Check if the constructor handles an empty name correctly
    const char* name2 = "";
    os::Event event2(name2);
    EXPECT_STREQ(name2, event2.GetName());

    // Test case 3: Check if the constructor handles a long name correctly
    const char* name3 = "ThisIsALongEventName";
    os::Event event3(name3);
    EXPECT_STREQ(name3, event3.GetName());
}

TEST(EventTest, LongestNameThatFitsIsKept)
{
    std::string name(EventNameSize - 1, 'n');
    EventInDirtyMemory event(name.c_str());
    ASSERT_TRUE(NameIsComplete(event->GetName(), EventNameSize));
    EXPECT_EQ(name, event->GetName());
}

TEST(EventTest, ANameThatIsTooLongIsTruncated)
{
    for (size_t length : {EventNameSize, EventNameSize + 1, size_t(200), size_t(5000)})
    {
        std::string name(length, 'n');
        name[EventNameSize - 1] = 'L'; // the first character that does not fit
        EventInDirtyMemory event(name.c_str());
        ASSERT_TRUE(NameIsComplete(event->GetName(), EventNameSize)) << "length " << length;
        EXPECT_EQ(name.substr(0, EventNameSize - 1), event->GetName()) << "length " << length;
    }
}

TEST(EventTest, ANullNameIsTheEmptyString)
{
    EventInDirtyMemory event(nullptr);
    ASSERT_TRUE(NameIsComplete(event->GetName(), EventNameSize));
    EXPECT_STREQ("", event->GetName());
}

TEST(EventTest, TheWholeNameBufferIsWritten)
{
    EventInDirtyMemory event("abc");
    const char* name = event->GetName();
    EXPECT_STREQ("abc", name);
    for (size_t i = 3; i < EventNameSize; i++)
    {
        EXPECT_EQ(0, name[i]) << "byte " << i;
    }
}

TEST(EventTest, AnEventWithANullNameStillWorks)
{
    os::Event event(nullptr);
    event.Notify();
    EXPECT_TRUE(event.Wait(__FILE__, __LINE__));

    // It is listed with an empty name, not with "(null)"
    std::string text = DumpEvents();
    EXPECT_FALSE(Contains(text, "(null)"));
    EXPECT_TRUE(Contains(text, std::string(30, ' ') + "|"));
}

TEST(EventTest, NameIsCopied)
{
    std::string name = "copied-name";
    os::Event event(name.c_str());
    name = "changed";
    EXPECT_STREQ("copied-name", event.GetName());
}

TEST(EventTest, NotifyBeforeWaitMakesWaitReturnAtOnce)
{
    os::Event event("notify-first");
    event.Notify();
    EXPECT_TRUE(event.Wait(__FILE__, __LINE__));
}

TEST(EventTest, NotifyBeforeWaitWithATimeout)
{
    os::Event event("notify-first-timeout");
    event.Notify();
    EXPECT_TRUE(event.Wait(__FILE__, __LINE__, 100));
}

TEST(EventTest, NotifyWakesAThreadThatIsWaiting)
{
    os::Event event("wake-std-thread");
    std::atomic<bool> started(false);
    std::atomic<bool> done(false);
    std::thread waiter(
        [&]
        {
            started = true;
            event.Wait(__FILE__, __LINE__);
            done = true;
        });
    ASSERT_TRUE(WaitUntil([&] { return started.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done.load()) << "Wait returned before anything was notified";

    event.Notify();
    EXPECT_TRUE(WaitUntil([&] { return done.load(); }));
    waiter.join();
}

TEST(EventTest, WaitUsesUpTheNotification)
{
    os::Event event("one-notification");
    event.Notify();
    event.Wait(__FILE__, __LINE__);

    // The notification is gone, so a second wait has to wait for a second notify.
    std::atomic<bool> started(false);
    std::atomic<bool> done(false);
    std::thread waiter(
        [&]
        {
            started = true;
            event.Wait(__FILE__, __LINE__);
            done = true;
        });
    ASSERT_TRUE(WaitUntil([&] { return started.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done.load());

    event.Notify();
    EXPECT_TRUE(WaitUntil([&] { return done.load(); }));
    waiter.join();
}

TEST(EventTest, SeveralNotifiesBeforeAWaitAreOneNotification)
{
    os::Event event("latched");
    event.Notify();
    event.Notify();
    event.Notify();
    event.Wait(__FILE__, __LINE__);

    std::atomic<bool> started(false);
    std::atomic<bool> done(false);
    std::thread waiter(
        [&]
        {
            started = true;
            event.Wait(__FILE__, __LINE__);
            done = true;
        });
    ASSERT_TRUE(WaitUntil([&] { return started.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(done.load());

    event.Notify();
    EXPECT_TRUE(WaitUntil([&] { return done.load(); }));
    waiter.join();
}

TEST(EventTest, CanBeUsedAgainAndAgain)
{
    os::Event event("ping-pong");
    os::Event reply("ping-pong-reply");
    std::atomic<int> rounds(0);
    const int total = 500;

    std::thread other(
        [&]
        {
            for (int i = 0; i < total; i++)
            {
                event.Wait(__FILE__, __LINE__);
                rounds++;
                reply.Notify();
            }
        });
    for (int i = 0; i < total; i++)
    {
        event.Notify();
        reply.Wait(__FILE__, __LINE__);
    }
    other.join();
    EXPECT_EQ(total, rounds.load());
}

TEST(EventTest, WakesEveryEventThatIsNotifiedAndNoOthers)
{
    os::Event first("separate-first");
    os::Event second("separate-second");
    std::atomic<bool> first_done(false);
    std::atomic<bool> second_done(false);
    std::thread a(
        [&]
        {
            first.Wait(__FILE__, __LINE__);
            first_done = true;
        });
    std::thread b(
        [&]
        {
            second.Wait(__FILE__, __LINE__);
            second_done = true;
        });

    first.Notify();
    EXPECT_TRUE(WaitUntil([&] { return first_done.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(second_done.load());

    second.Notify();
    EXPECT_TRUE(WaitUntil([&] { return second_done.load(); }));
    a.join();
    b.join();
}

TEST(EventTest, AnOsThreadCanWaitForAnEvent)
{
    os::Event event("os-thread-event");
    Waiter waiter;
    waiter.event = &event;
    waiter.started = false;
    waiter.done = false;
    waiter.got = false;

    os::Thread thread;
    thread.Create(WaitForEvent, "event-waiter", 0, 0, &waiter);
    ASSERT_TRUE(WaitUntil([&] { return waiter.started.load(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_FALSE(waiter.done.load());

    event.Notify();
    ASSERT_TRUE(WaitUntil([&] { return waiter.done.load(); }));
    EXPECT_TRUE(waiter.got.load());
}

TEST(EventTest, DumpInfoHasAHeader)
{
    std::string text = DumpEvents();
    EXPECT_TRUE(Contains(text, "Event                         |Thread Name         |State\n"));
    EXPECT_TRUE(
        Contains(text, "------------------------------+--------------------+------------\n"));
}

TEST(EventTest, DumpInfoListsTheEvent)
{
    os::Event event("listed-event");
    std::string row = Line(DumpEvents(), "listed-event");
    EXPECT_FALSE(row.empty());
    // name | waiting thread (none) | state
    EXPECT_EQ("listed-event                  |                    |          ", row);
}

TEST(EventTest, DumpInfoNamesTheThreadThatIsWaiting)
{
    os::Event event("watched-event");
    Waiter waiter;
    waiter.event = &event;
    waiter.started = false;
    waiter.done = false;
    waiter.got = false;

    os::Thread thread;
    thread.Create(WaitForEvent, "watched-waiter", 0, 0, &waiter);
    ASSERT_TRUE(WaitUntil([&] { return waiter.started.load(); }));

    // The waiting thread shows up in the event's row, and the event in the thread's row.
    EXPECT_TRUE(
        WaitUntil([&] { return Contains(Line(DumpEvents(), "watched-event"), "watched-waiter"); }));
    EXPECT_TRUE(WaitUntil(
        [&] {
            return Contains(Line(DumpThreads(), "watched-waiter"),
                            "pending event \"watched-event\"");
        }));

    event.Notify();
    ASSERT_TRUE(WaitUntil([&] { return waiter.done.load(); }));

    EXPECT_TRUE(WaitUntil(
        [&] { return !Contains(Line(DumpEvents(), "watched-event"), "watched-waiter"); }));
    EXPECT_FALSE(Contains(Line(DumpThreads(), "watched-waiter"), "pending event"));
}

TEST(EventTest, DumpInfoListsEveryEvent)
{
    os::Event first("list-all-first");
    os::Event second("list-all-second");
    std::string text = DumpEvents();
    EXPECT_TRUE(Contains(text, "list-all-first"));
    EXPECT_TRUE(Contains(text, "list-all-second"));
}

TEST(EventTest, DestroyedEventIsRemovedFromDumpInfo)
{
    {
        os::Event event("short-lived-event");
        EXPECT_TRUE(Contains(DumpEvents(), "short-lived-event"));
    }
    EXPECT_FALSE(Contains(DumpEvents(), "short-lived-event"));
}

TEST(EventTest, DestroyedEventSlotCanBeReused)
{
    for (int i = 0; i < 100; i++)
    {
        os::Event event("recycled-event");
        event.Notify();
        event.Wait(__FILE__, __LINE__);
    }
    os::Event last("recycled-last");
    EXPECT_TRUE(Contains(DumpEvents(), "recycled-last"));
    EXPECT_FALSE(Contains(DumpEvents(), "recycled-event"));
}

TEST(EventTest, MoreEventsThanTheListHoldsAllStillWork)
{
    // The list that dump_info() uses has room for 20 events. The rest are not listed but work.
    const int count = 40;
    std::vector<std::string> names;
    for (int i = 0; i < count; i++)
    {
        names.push_back("overflow-event-" + std::to_string(i));
    }
    {
        std::vector<std::unique_ptr<os::Event>> events;
        for (int i = 0; i < count; i++)
        {
            events.emplace_back(new os::Event(names[i].c_str()));
        }
        for (auto& event : events)
        {
            event->Notify();
            EXPECT_TRUE(event->Wait(__FILE__, __LINE__));
        }

        std::string text = DumpEvents();
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

    std::string text = DumpEvents();
    for (const std::string& name : names)
    {
        EXPECT_FALSE(Contains(text, name)) << name << " is still listed after it was destroyed";
    }
}

// KNOWN BUG: on Linux Wait() ignores its timeout and waits until the event is notified, which
// the Windows implementation does not. Do not enable this test until that is fixed, because until
// then it never returns.
TEST(EventTest, DISABLED_WaitGivesUpAfterTheTimeout)
{
    os::Event event("timeout");
    auto start = std::chrono::steady_clock::now();
    bool notified = event.Wait(__FILE__, __LINE__, 50);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(notified);
    EXPECT_GE(elapsed, std::chrono::milliseconds(40));
    EXPECT_LT(elapsed, std::chrono::seconds(2));
}
