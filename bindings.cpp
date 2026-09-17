#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "ThreadPool.hpp"

namespace py = pybind11;

// A wrapper class to adapt the generic C++ ThreadPool for high-level Python functions
class PyJobScheduler {
public:
    explicit PyJobScheduler(size_t threads) : pool(threads) {}

    // Enqueues a pure Python function and executes it inside the C++ worker pool
    void submit_job(const py::function& func) {
        // We capture the Python function inside a standard C++ function block
        pool.enqueue([func]() {
            // Acquire the Python Global Interpreter Lock (GIL) before interacting with Python objects
            py::gil_scoped_acquire acquire;
            try {
                func();
            } catch (py::error_already_set &e) {
                // Safely catch and print Python exceptions inside C++ threads
                py::print("Error executing Python job in C++ thread context:", e.what());
            }
        });
    }

private:
    ThreadPool pool;
};

// Define the compiled Python module structure
PYBIND11_MODULE(core_engine, m) {
    m.doc() = "High-performance C++ execution engine wrapper for Python tasks";

    py::class_<PyJobScheduler>(m, "JobScheduler")
        .def(py::init<size_t>(), py::arg("threads") = std::thread::hardware_concurrency())
        .def("submit_job", &PyJobScheduler::submit_job, "Submit a Python function to the execution queue");
}

