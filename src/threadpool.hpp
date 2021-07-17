/* Copyright (c) 2018 Ethan Margaillan <contact@ethan.jp>.
 *
 *  Licensed under the MIT Licence
 *  https://raw.githubusercontent.com/Ethan13310/Thread-Pool-Cpp/master/LICENSE
 *  https://github.com/Ethan13310/Thread-Pool-Cpp/blob/master/ThreadPool.hpp
 */


/**
 * usage:

   create a threadpool with 10 workers:
   class threadpool t(10);

   Ask a worker to fun function process() with 'arg' from the calling context::
   t.push( [arg]{ process(arg); } );

   Wait until all workers are finished their current tasks:
   t.join()

*/

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
#include <map>
#include <string>
#include <iostream>
#include <chrono>

class threadpool
{
    // Task function
    using task_type = std::function<void()>;

public:
    struct status_t {
        std::string msg {};                       // message
        std::chrono::time_point<std::chrono::high_resolution_clock> t0 {}; // when the status was set
    };

    std::atomic<bool>        m_stop{ false }; // set true to terminate workers
    std::atomic<std::size_t> m_active{ 0 };   // number of active workers
    std::condition_variable  m_notifier {};
    mutable std::mutex       M {};              // protects m_workers, m_tasks
    std::vector<std::thread> m_workers {};
    std::queue<task_type>    m_tasks {};

    mutable std::mutex m_status_mutex {};                       // protext m_status_mutex
    std::map<std::thread::id, status_t> m_status {}; // status of each thread

    explicit threadpool(std::size_t thread_count = std::thread::hardware_concurrency()) {
        for (std::size_t i{ 0 }; i < thread_count; ++i) {
            m_workers.emplace_back(std::bind(&threadpool::thread_loop, this));
        }
    }

    ~threadpool() {
        if (m_workers.size() > 0) {
            join();
        }
    }

    threadpool(threadpool const &) = delete;
    threadpool &operator=(threadpool const &) = delete;

    // Push a new task into the queue
    template <class Func, class... Args>
    auto push(Func &&fn, Args &&...args) {
        std::cerr << "tp:push:1\n";
        using return_type = typename std::result_of<Func(Args...)>::type;
        std::cerr << "tp:push:2\n";
        auto task { std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<Func>(fn), std::forward<Args>(args)...)
                ) };
        std::cerr << "tp:push:3\n";
        auto future{ task->get_future() };
        std::cerr << "tp:push:4\n";
        std::unique_lock<std::mutex> lock{ M };
        std::cerr << "tp:push:5\n";
        m_tasks.emplace([task]() { (*task)(); });
        std::cerr << "tp:push:6\n";
        //lock.unlock();  is this valid?
        m_notifier.notify_one();
        std::cerr << "tp:push:7\n";
        return future;
    }

    // Set status
    void set_status(const std::string &msg){
        std::unique_lock<std::mutex> lock(m_status_mutex);
        struct status_t &status = m_status[std::this_thread::get_id()];
        status.msg = msg;
        status.t0  = std::chrono::high_resolution_clock::now();
    }

    // Dump the status
    void dump_status(std::ostream &os) {
        std::unique_lock<std::mutex> lock(m_status_mutex);
        os.setf(std::ios::fixed,std::ios::floatfield);
        os.precision(3);
        auto now = std::chrono::high_resolution_clock::now();
        for(const auto &it : m_status) {
            const struct status_t &status = it.second;
            auto elapsed = now - status.t0;
            long long miliseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000;
            os << status.msg << " ( " << miliseconds << " ms) \n";
        }
        os.unsetf(std::ios::fixed);
    }

    // Remove all pending tasks from the queue
    void clear() {
        std::unique_lock<std::mutex> lock(M);
        while (!m_tasks.empty()) {
            m_tasks.pop();
        }
    }

    // Wait all workers to finish
    // This should probably make a threadsafe copy of m_workers first...
    void join() {
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
    std::size_t thread_count() const {
        std::unique_lock<std::mutex> lock(M);
        return m_workers.size();
    }

    // Get the number of active workers
    std::size_t active_count() const {
        return m_active;
    }

    // Get the number of active tasks
    std::size_t task_count() const {
        std::unique_lock<std::mutex> lock(M);
        return m_tasks.size();
    }

private:
    // Thread main loop
    void thread_loop() {
        while (true) {
            // Wait for a new task
            auto task{ next_task() };

            if (task) {
                ++m_active;
                set_status("ACTIVE");
                task();
                set_status("IDLE");
                --m_active;
            }
            else if (m_stop) {
                // No more task + stop required
                break;
            }
        }
    }

    // Get the next pending task
    task_type next_task() {
        std::unique_lock<std::mutex> lock(M);
        m_notifier.wait(lock, [this]() {
                                  return !m_tasks.empty() || m_stop;
                              });

        if (m_tasks.empty()) {
            return {};                  // No pending task
        }

        auto task{ m_tasks.front() };
        m_tasks.pop();
        return task;
    }

};
#endif
