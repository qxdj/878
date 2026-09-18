# 878

# High-Performance Hybrid DAG Concurrency Engine

A cross-language concurrent execution engine built in **C++17** and bound natively to **Python 3 via pybind11**. The engine maps task orchestration pipelines into a thread-safe, weighted **Directed Acyclic Graph (DAG)** to schedule non-blocking execution sequences with dynamic **Min-Heap priority re-sorting**.

## 🚀 Key Architectural Innovations

*   **Topological Task Orchestration:** Resolves complex graph dependencies at runtime by tracking node in-degrees (O(1) lookup mapping) and cascading unlocks to child nodes upon task completion.
*   **Weighted Min-Heap Scheduling:** Tasks whose dependencies hit zero are instantly pushed into a priority matrix (`std::priority_queue`) for bounded \(O(\log n)\) scheduling optimizations.
*   **Thread-Safe Core Processing:** Leverages a custom thread-pool paradigm using atomic signaling, mutual exclusion primitives (`std::mutex`), and thread synchronization blockades (`std::condition_variable`).
*   **Bulletproof GIL Isolation:** Eliminates Python Global Interpreter Lock (GIL) cross-thread corruption by maintaining function object lifecycles within an opaque tracking registry inside the Python runtime environment, entirely bypassing unsafe implicit deallocations on native C++ threads.

---

## 🛠️ System Architecture

```text
  Python Layer                  pybind11 Layer               C++ Core Engine
┌──────────────────────┐      ┌──────────────────────┐     ┌────────────────────────┐
│  Register Tasks &    │      │  Lock Python GIL     │     │ ThreadPool / Mutex     │
│  Define Dependencies ├─────►│  (gil_scoped_acquire)├───► │ Task Node Registry     │
└──────────────────────┘      └──────────┬───────────┘     └───────────┬────────────┘
                                         ▲                             │
                                         │ Unlocks Child               │ Evaluates In-Degree
                                         │ Nodes                       ▼ ($O(1)$)
                               ┌─────────┴───────────┐     ┌────────────────────────┐
                               │ Deallocate Context  │◄────┤ Execution Priority Heap│
                               │ Via Object .pop()   │     │ (Min-Heap $O(\log n)$) │
                               └─────────────────────┘     └────────────────────────┘
```

---

## 📦 Prerequisites & Environment

The automation pipeline utilizes standard system tooling. Ensure your local machine or container has:
*   A C++17 compliant compiler (`g++ >= 9` or `clang`)
*   Python 3.8+ with header packages (`python3-dev`)

Dependencies are listed in `requirements.txt`:
```text
pybind11>=2.10.0
pytest>=7.0.0
```

---

## 🏗️ Compilation & Testing

To compile the C++ binding matrix into a native Python platform extension module, execute:

```bash
g++ -O3 -Wall -shared -std=c++17 -fPIC $(python3 -m pybind11 --includes) bindings.cpp -o core_engine$(python3-config --extension-suffix) -pthread
```

### Running Automated Test Assertions
The repository includes a topological validation engine managed through `pytest`:

```bash
pytest -v test_engine.py
```

---

## 💻 Programmatic Usage Example

The framework allows you to declare arbitrary graph topologies directly from Python script contexts while the compilation module executes workloads on underlying OS thread infrastructures:

```python
import core_engine
import time

# Spin up the concurrency engine with 4 workers
scheduler = core_engine.JobScheduler(threads=4)

def workload(node_label):
    print(f"[Worker Thread] Processing Node: {node_label}")
    time.sleep(0.1)

# Register a terminal sink node (ID: 4) depending on components 2 and 3
scheduler.submit_job(id=4, priority=1, parents=[2, 3], func=lambda: workload("Terminal Sink"))

# Register parallel branch nodes depending on root ingestion (ID: 1)
scheduler.submit_job(id=2, priority=5, parents=[1], func=lambda: workload("Branch Alpha"))
scheduler.submit_job(id=3, priority=2, parents=[1], func=lambda: workload("Branch Beta"))

# Register the root task node with no parent blockades
scheduler.submit_job(id=1, priority=10, parents=[], func=lambda: workload("Root Ingestion"))

# Background C++ threads instantly schedule and process nodes in topological alignment
time.sleep(1.0)
```

---

## 🛡️ CI/CD Infrastructure

This repository maintains a fully integrated **GitHub Actions pipeline** (`.github/workflows/verify.yml`). On every push to the repository, the system spins up a virtual Linux runner, manages cross-language dependency caching, compiles the binary extension, and runs edge-case tests to guarantee system stability and memory layout integrity.

