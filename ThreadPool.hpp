#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class ThreadPool {
private:
    struct TaskNode {
        int id;
        int priority;
        std::function<void()> execute;
        
        int dependencies_remaining = 0;
        std::vector<int> dependent_children;

        bool operator<(const TaskNode& other) const {
            return priority > other.priority;
        }
    };

public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task_exec;
                    int completed_id = -1;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->ready_heap.empty(); 
                        });
                        
                        if (this->stop && this->ready_heap.empty()) return;
                        
                        auto node = std::move(this->ready_heap.top());
                        this->ready_heap.pop();
                        
                        task_exec = std::move(node.execute);
                        completed_id = node.id;
                    }
                    
                    // Executăm funcția. Orice protecție GIL necesară va fi gestionată în interiorul ei.
                    task_exec();
                    
                    // Goliți referințele C++ std::function IMEDIAT înainte de faza de deblocare a grafului
                    task_exec = nullptr;
                    
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        auto it = this->dag_registry.find(completed_id);
                        if (it != this->dag_registry.end()) {
                            for (int child_id : it->second->dependent_children) {
                                auto child_node = this->dag_registry[child_id];
                                child_node->dependencies_remaining--;
                                
                                if (child_node->dependencies_remaining == 0) {
                                    this->ready_heap.push(*child_node);
                                }
                            }
                            this->dag_registry.erase(it);
                        }
                    }
                    this->condition.notify_all();
                }
            });
        }
    }

    void register_task(int id, int priority, std::function<void()> f, const std::vector<int>& parent_ids) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        auto new_node = std::make_shared<TaskNode>();
        new_node->id = id;
        new_node->priority = priority;
        new_node->execute = std::move(f);
        
        for (int p_id : parent_ids) {
            if (dag_registry.find(p_id) != dag_registry.end()) {
                new_node->dependencies_remaining++;
                dag_registry[p_id]->dependent_children.push_back(id);
            }
        }
        
        dag_registry[id] = new_node;
        
        if (new_node->dependencies_remaining == 0) {
            ready_heap.push(*new_node);
            condition.notify_one();
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) worker.join();
        }
        dag_registry.clear();
    }

private:
    std::vector<std::thread> workers;
    std::priority_queue<TaskNode> ready_heap; 
    std::unordered_map<int, std::shared_ptr<TaskNode>> dag_registry;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

#endif // THREAD_POOL_HPP

