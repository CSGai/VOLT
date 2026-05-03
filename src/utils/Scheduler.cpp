#pragma once
#include <algorithm>
#include <chrono>
#include <functional>
#include <stop_token>
#include <thread>
#include <vector>

class Scheduler {

    public:
        // callback function, interval in minutes
        void add_task(std::function<void()> callback, int interval) { tasks_.push_back({callback, std::chrono::minutes(interval)}); }

        // spawns threads for each task which sleep based on interval between activations
        void start() {
            sort_tasks();
            for (const auto& task : tasks_) {
                threads_.emplace_back([task](std::stop_token st) {
                    while (!st.stop_requested()) {
                        task.func();
                        std::this_thread::sleep_for(task.interval);
                    }
                });
            }
        }
        // raises stop flag for every thread in scheduler scope in order
        void stop() {
            for (auto& t : threads_)
                t.request_stop();
        }

        ~Scheduler() { stop(); }

    private:
        struct Task {
                std::function<void()> func;
                std::chrono::steady_clock::duration interval;
        };
        std::vector<Task> tasks_;
        std::vector<std::jthread> threads_;

        // sorts tasks from shortest intervals to longest
        void sort_tasks() {
            std::sort(tasks_.begin(), tasks_.end(), [](const Task& a, const Task& b) { return a.interval < b.interval; });
        }
};