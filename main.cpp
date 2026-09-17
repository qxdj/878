#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"

// A dummy computational task simulating mathematical load
int simulate_computation(int id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return id * id;
}

int main() {
    // Spin up an optimized pool based on available CPU cores
    ThreadPool pool;
    std::vector<std::future<int>> results;

    // Enqueue 8 distinct algorithmic tasks
    for (int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(simulate_computation, i)
        );
    }

    // Safely resolve the futures asynchronously as they finish execution
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Task Result [" << i << "]: " << results[i].get() << std::endl;
    }

    return 0;
}

