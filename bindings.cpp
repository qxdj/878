#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  
#include "ThreadPool.hpp"

namespace py = pybind11;

// Adăugăm atributul de vizibilitate pentru a alinia clasa cu tipurile interne pybind11
class PYBIND11_EXPORT PyJobScheduler {
public:
    explicit PyJobScheduler(size_t threads) : pool(threads) {
        py::gil_scoped_acquire acquire;
        py_registry = py::dict();
    }

    void submit_dag_job(int id, int priority, const std::vector<int>& parents, const py::object& func) {
        {
            py::gil_scoped_acquire acquire;
            py_registry[py::int_(id)] = func;
        }

        pool.register_task(id, priority, [this, id]() {
            py::gil_scoped_acquire acquire;
            try {
                if (this->py_registry.contains(py::int_(id))) {
                    py::object target_func = this->py_registry[py::int_(id)];
                    target_func();
                }
            } catch (py::error_already_set &e) {
                py::print("DAG node execution fault:", e.what());
            }

            // REZOLVARE EROARE: Ștergem cheia apelând metoda nativă .pop() a dicționarului Python
            if (this->py_registry.contains(py::int_(id))) {
                this->py_registry.attr("pop")(py::int_(id));
            }
        }, parents);
    }

    ~PyJobScheduler() {
        py::gil_scoped_acquire acquire;
        py_registry.clear();
    }

private:
    ThreadPool pool;
    py::dict py_registry; 
};

PYBIND11_MODULE(core_engine, m) {
    m.doc() = "High-performance C++ execution engine with mathematical DAG scheduling rules";

    py::class_<PyJobScheduler>(m, "JobScheduler")
        .def(py::init<size_t>(), py::arg("threads") = std::thread::hardware_concurrency())
        .def("submit_job", &PyJobScheduler::submit_dag_job, 
             py::arg("id"), py::arg("priority"), py::arg("parents"), py::arg("func"),
             "Submit a Python workload into the concurrent DAG pipeline framework");
}
