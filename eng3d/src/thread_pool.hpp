// Eng3D - General purpouse game engine
// Copyright (C) 2021, Eng3D contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
// Name:
//      thread_pool.hpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Eng3D {
    class ThreadPool {
        // An atomic all idling threads use
        std::atomic<bool> running;
        // While std::thread is not copy-able, the std::vector STL container
        // allows for movable elements instead. keep this in mind when a bug happens
        std::vector<std::thread> threads;
        // Queues are suited for the job here, since first-requested-jobs
        // should have higher priority than the last one, this allows sequential
        // completion of tasks
        std::queue<std::function<void()>> jobs;
        std::mutex job_mutex; // Our mutex for preventing race conditions on the queue above
        std::condition_variable cv_task; // Called when job is added
        std::condition_variable cv_finished; // Called when job is finished
        size_t busy = 0; // Amount of threads currently working
    public:
        /// @brief Construct a new Thread Pool object, this initializes threads for the pool
        ThreadPool() {
            // It's a good idea to use as many threads as the hardware implementation
            // supports. Otherwise we can run into performance hits.
            const size_t n_threads = (size_t)std::thread::hardware_concurrency();
            this->running = true;

            //this->threads = std::vector<std::thread>(n_threads, th);
            for(size_t i = 0; i < n_threads; i++)
                this->threads.push_back(std::thread(&ThreadPool::thread_loop, this));

            // This vector is not going to expand anymore
            this->threads.shrink_to_fit();
            // We now have our threads running at this point - waiting for jobs to take
        }

        /// @brief Destroy the Thread Pool, closing all the pending jobs
        ~ThreadPool() {
            // We are going to signal all threads to shutdown
            std::unique_lock<std::mutex> latch(job_mutex);
            running = false;
            cv_task.notify_all();
            latch.unlock();

            // After that we will start joining all threads - if a thread is executing
            // by a prolonged time, it will block the entire process
            for(auto job = threads.begin(); job != threads.end(); job++)
                (*job).join();
        }

        /// @brief Adds a job to the list of pending jobs
        /// @param job Job to add to the queue
        void add_job(std::function<void()> job) {
            const std::scoped_lock lock(job_mutex);
            jobs.push(job);
            cv_task.notify_one();
        }

        /// @brief This loop is executed on each thread on the thread list, what this basically does
        /// is to check in the list of available jobs for jobs we can take
        void thread_loop() {
            while(true) {
                std::unique_lock<std::mutex> latch(job_mutex);
                cv_task.wait(latch, [this]() {
                    return (!running || !jobs.empty());
                });

                if(!running) break;

                // pull from queue
                std::function<void()> fn = jobs.front();
                jobs.pop();

                busy++;
                // release lock. run async
                latch.unlock();

                fn();

                latch.lock();
                cv_finished.notify_one();
            }
        }

        /// @brief Waits until the job list is empty
        void wait_finished() {
            std::unique_lock<std::mutex> lock(job_mutex);
            cv_finished.wait(lock, [this]() {
                return (jobs.empty() && (busy == 0));
            });
        }

        template<typename I, typename F>
        static void for_each(I first, I last, F func) {
            const size_t n_threads = (size_t)std::thread::hardware_concurrency();
            if(!std::distance<I>(first, last))
                return;

            const size_t iterators_per_thread = std::distance<I>(first, last) / n_threads;
            for(; first != last; first++) {
                func(*first);
            }
        }
    };
};
