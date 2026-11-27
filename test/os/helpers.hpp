#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "os/output.hpp"

namespace os_test
{

// A TextOutput that appends to the std::string passed as the context.
inline void Append(void* context, const char* text, size_t length)
{
    static_cast<std::string*>(context)->append(text, length);
}

// Runs a dump_info() function and returns everything it printed.
inline std::string Dump(void (*dump_info)(os::TextOutput, void*))
{
    std::string text;
    dump_info(Append, &text);
    return text;
}

inline bool Contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

// The line of text that contains needle, without its newline. Empty if there is none.
inline std::string Line(const std::string& text, const std::string& needle)
{
    size_t at = text.find(needle);
    if (at == std::string::npos)
    {
        return "";
    }
    size_t start = text.rfind('\n', at);
    start = (start == std::string::npos) ? 0 : start + 1;
    size_t end = text.find('\n', at);
    return text.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// Polls until condition() is true. Gives up after timeout_ms and returns the last value.
template <typename Condition>
bool WaitUntil(Condition condition, int timeout_ms = 3000)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (!condition())
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            return condition();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

// Lets a test hold threads where it wants them. It is built on std:: primitives so waiting on it
// does not touch the state that the os classes track for their own threads.
class Gate
{
public:
    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return m_open; });
    }

    void Open()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_open = true;
        }
        m_condition.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_open = false;
};

} // namespace os_test
