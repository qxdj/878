# High-Performance Hybrid DAG Concurrency Engine (ID: 1789603920)

A cross-language concurrent execution engine built in **C++17** and bound natively to **Python 3 via pybind11**. The engine maps task orchestration pipelines into a thread-safe, weighted **Directed Acyclic Graph (DAG)** to schedule non-blocking execution sequences with dynamic **Min-Heap priority re-sorting** and runtime **Fault-Tolerance Isolation**.

## 🚀 Key Architectural Innovations & Resilience

*   **Topological Task Orchestration:** Resolves complex graph dependencies at runtime by tracking node in-degrees (O(1) lookup mapping) and cascading unlocks to child nodes upon successful task completion.
*   **Weighted Min-Heap Scheduling:** Tasks whose dependencies hit zero are instantly pushed into a binary priority matrix (`std::priority_queue`) for bounded \(O(\log n)\) scheduling optimizations.
*   **Asynchronous Watchdog & Timeouts:** Tracks task execution durations via non-blocking standard asynchronous primitives (`std::future` / `std::packaged_task`). Hanging or stalled operations are forcefully captured once they exceed their microsecond timeout windows.
*   **Cascaded Fault-Tolerance & DLQ Routing:** Protects the thread pool from starvation or unexpected crashes. If a task raises a runtime exception or times out, the core isolates its metadata, halts downstream execution branches, flags all dependent child elements with structural failure tracking, and routes them into an isolated **Dead-Letter Queue (DLQ)**.
*   **The Primitive Identifier Token Pattern:** Eliminates Python Global Interpreter Lock (GIL) cross-thread corruption and *Small Object Optimization (SOO)* anomalies inside `std::function`. Dynamic Python references are securely locked inside a synchronized, module-level registry (`py::dict`) on the Python stack, passing only primitive `int id` descriptors to the C++ core layers.

---

## 🛠️ System Lifecycle & Deallocation Layout

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
                                         ▲
                                         │ Catch Exceptions / Timeouts
                               ┌─────────┴───────────┐
                               │ Isolate Node & Kids │
                               │ Route to Python DLQ │
                               └─────────────────────┘
```

---

## 📦 Prerequisites & Environment

The automation pipeline utilizes standard system tooling. Ensure your local machine or container has:
*   A C++17 compliant compiler (`g++ >= 9` or `clang`)
*   Python 3.8+ with header packages (`python3-dev`)

Dependencies are managed through `requirements.txt`:
```text
pybind11>=2.10.0
pytest>=7.0.0
```

---

## 🏗️ Compilation & Testing

To compile the C++ binding matrix into a native Python platform extension module, execute the optimized production configuration string:

```bash
g++ -O3 -Wall -shared -std=c++17 -fPIC $(python3 -m pybind11 --includes) bindings.cpp -o core_engine$(python3-config --extension-suffix) -pthread
```

### Running Automated Test Assertions
The repository includes an integrated testing framework managed through `pytest` to verify topological correctness, thread isolation, and cascaded error routing:

```bash
pytest -v test_engine.py
```

---

## 💻 Resilient Programmatic Usage Example

The framework allows you to declare arbitrary graph topologies directly from Python script contexts while the compilation module executes workloads on underlying OS thread infrastructures:

```python
import core_engine
import time

# Spin up the concurrency engine with 2 workers
scheduler = core_engine.JobScheduler(threads=2)

def broken_parent_task():
    raise ValueError("Simulated database connection loss!")

def child_task():
    print("This will never run because the parent failed.")

# Register a graph where Node 2 depends on Node 1
scheduler.submit_job(id=2, priority=1, parents=[1], func=child_task)
scheduler.submit_job(id=1, priority=5, parents=[], func=broken_parent_task)

# Let the background worker threads capture and route the failure
time.sleep(0.5)

# Inspect the isolated anomalies from the Dead-Letter Queue
dlq_records = scheduler.get_dlq()
for record in dlq_records:
    print(f"[DLQ Alert] Task {record['task_id']} isolated. Reason: {record['reason']}")
```

---

## 🛡️ CI/CD Infrastructure

This repository maintains a fully integrated **GitHub Actions pipeline** (`.github/workflows/verify.yml`). On every push to the repository, the system spins up a virtual Linux runner, manages cross-language dependency caching, compiles the binary extension, and runs edge-case tests to guarantee system stability and memory layout integrity.

