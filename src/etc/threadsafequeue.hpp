// http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
/**
 * The ThreadSafeQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 */
#pragma once

#ifndef THREADSAFEQUEUE_HPP
#define THREADSAFEQUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace MyNamespace
{
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        /**
         * Destructor.
         */
        ~ThreadSafeQueue(void)
        {
            invalidate();
        }

        /**
         * Attempt to get the first value in the queue.
         * Returns true if a value was successfully written to the out parameter, false otherwise.
         */
        bool tryPop(T& out)
        {
            std::lock_guard<std::mutex> std::lock{m_mutex};
            if(m_queue.empty() || !m_valid)
            {
                return false;
            }
            out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        /**
         * Get the first value in the queue.
         * Will block until a value is available unless clear is called or the instance is destructed.
         * Returns true if a value was successfully written to the out parameter, false otherwise.
         */
        bool waitPop(T& out)
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_condition.wait(lock, [this]()
            {
                return !m_queue.empty() || !m_valid;
            });
            /*
             * Using the condition in the predicate ensures that spurious wakeups with a valid
             * but empty queue will not proceed, so only need to check for validity before proceeding.
             */
            if(!m_valid)
            {
                return false;
            }
            out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        /**
         * Push a new value onto the queue.
         */
        void push(T value)
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_queue.push(std::move(value));
            m_condition.notify_one();
        }

        /**
         * Check whether or not the queue is empty.
         */
        bool empty(void) const
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            return m_queue.empty();
        }

        /**
         * Clear all items from the queue.
         */
        void clear(void)
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            while(!m_queue.empty())
            {
                m_queue.pop();
            }
            m_condition.notify_all();
        }

        /**
         * Invalidate the queue.
         * Used to ensure no conditions are being waited on in waitPop when
         * a thread or the application is trying to exit.
         * The queue is invalid after calling this method and it is an error
         * to continue using a queue after this method has been called.
         */
        void invalidate(void)
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_valid = false;
            m_condition.notify_all();
        }

        /**
         * Returns whether or not this queue is valid.
         */
        bool isValid(void) const
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            return m_valid;
        }

    private:
        std::atomic_bool m_valid{true};
        mutable std::mutex m_mutex;
        std::queue<T> m_queue;
        std::condition_variable m_condition;
    };
}

#endif
