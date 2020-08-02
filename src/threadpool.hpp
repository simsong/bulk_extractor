// Copyright (c) 2018 Ethan Margaillan <contact@ethan.jp>.
// Licensed under the MIT Licence
// https://raw.githubusercontent.com/Ethan13310/Thread-Pool-Cpp/master/LICENSE
// https://github.com/Ethan13310/Thread-Pool-Cpp/blob/master/ThreadPool.hpp

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class thread_pool
{
    // Task function
    using task_type = std::function<void()>;

public:
    explicit thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
    {
        for (std::size_t i{ 0 }; i < thread_count; ++i) {
            m_workers.emplace_back(std::bind(&thread_pool::thread_loop, this));
        }
    }

    ~thread_pool()
    {
        if (m_workers.size() > 0) {
            join();
        }
    }

    thread_pool(thread_pool const &) = delete;
    //thread_pool(thread_pool &&) = default;

    thread_pool &operator=(thread_pool const &) = delete;
    //thread_pool &operator=(thread_pool &&) = default;

    // Push a new task into the queue
    template <class Func, class... Args>
    auto push(Func &&fn, Args &&...args)
    {
        using return_type = typename std::result_of<Func(Args...)>::type;

        auto task{ std::make_shared<std::packaged_task<return_type()>>(
                                                                       std::bind(std::forward<Func>(fn), std::forward<Args>(args)...)
                                                                       ) };

        auto future{ task->get_future() };
        std::unique_lock<std::mutex> lock{ m_mutex };

        m_tasks.emplace([task]() {
                            (*task)();
                        });

        lock.unlock();
        m_notifier.notify_one();
        return future;
    }

    // Remove all pending tasks from the queue
    void clear()
    {
        std::unique_lock<std::mutex> lock{ m_mutex };

        while (!m_tasks.empty()) {
            m_tasks.pop();
        }
    }

    // Wait all workers to finish
    void join()
    {
        m_stop = true;
        m_notifier.notify_all();

        for (auto &thread : m_workers) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        m_workers.clear();
    }

    // Get the number of active and waiting workers
    std::size_t thread_count() const
    {
        return m_workers.size();
    }

    // Get the number of active workers
    std::size_t active_count() const
    {
        return m_active;
    }

private:
    // Thread main loop
    void thread_loop()
    {
        while (true) {
            // Wait for a new task
            auto task{ next_task() };

            if (task) {
                ++m_active;
                task();
                --m_active;
            }
            else if (m_stop) {
                // No more task + stop required
                break;
            }
        }
    }

    // Get the next pending task
    task_type next_task()
    {
        std::unique_lock<std::mutex> lock{ m_mutex };

        m_notifier.wait(lock, [this]() {
                                  return !m_tasks.empty() || m_stop;
                              });

        if (m_tasks.empty()) {
            // No pending task
            return {};
        }

        auto task{ m_tasks.front() };
        m_tasks.pop();
        return task;
    }

    std::atomic<bool> m_stop{ false };
    std::atomic<std::size_t> m_active{ 0 };

    std::condition_variable m_notifier;
    std::mutex m_mutex;

    std::vector<std::thread> m_workers;
    std::queue<task_type> m_tasks;
};
#endif
