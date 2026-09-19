import core_engine
import time
import pytest
import threading

def test_dag_execution_topology():
    scheduler = core_engine.JobScheduler(threads=4)
    execution_order = []
    list_lock = threading.Lock()

    def safe_log(node_id):
        with list_lock:
            execution_order.append(node_id)

    def step_one(): safe_log(1)
    def step_two(): safe_log(2); time.sleep(0.1)
    def step_three(): safe_log(3)
    def step_four(): safe_log(4)

    # 1. Register the root task node first to initialize the C++ graph registry
    scheduler.submit_job(id=1, priority=10, parents=[], func=step_one)
    
    # Give the C++ background worker thread a brief window to lock in the root node
    time.sleep(0.05)

    # 2. Register intermediary branches that depend on Node 1 completing first
    scheduler.submit_job(id=2, priority=5, parents=[1], func=step_two)
    scheduler.submit_job(id=3, priority=2, parents=[1], func=step_three)
    
    # 3. Register the terminal sink node that depends on both branches 2 and 3
    scheduler.submit_job(id=4, priority=1, parents=[2, 3], func=step_four)

    # Allow all background hardware execution lanes plenty of time to drain completely
    time.sleep(0.6)

    # 4. Verify mathematical topological correctness programmatically
    assert execution_order[0] == 1, f"Root Node 1 must execute first. Got order: {execution_order}"
    assert execution_order[-1] == 4, f"Terminal Sink Node 4 must execute last. Got order: {execution_order}"
    
    idx_1 = execution_order.index(1)
    idx_2 = execution_order.index(2)
    idx_3 = execution_order.index(3)
    idx_4 = execution_order.index(4)
    
    assert idx_1 < idx_2, "Node 2 must execute after Node 1."
    assert idx_1 < idx_3, "Node 3 must execute after Node 1."
    assert idx_2 < idx_4, "Node 4 must execute after Node 2."
    assert idx_3 < idx_4, "Node 4 must execute after Node 3."


def test_faulty_node_isolation_to_dlq():
    scheduler = core_engine.JobScheduler(threads=2)

    def broken_parent_task():
        raise ValueError("Simulated network database connectivity loss!")

    def child_task():
        pass 

    # Submit the faulty parent task first to prevent runtime registry collection races
    scheduler.submit_job(id=1, priority=5, parents=[], func=broken_parent_task)
    scheduler.submit_job(id=2, priority=1, parents=[1], func=child_task)

    time.sleep(0.5)

    # Inspect the isolated anomalies from the Python tracking scope
    dlq_records = scheduler.get_dlq()
    
    # FIXED ASSERTION: The engine safely records both the broken parent and the cascaded child
    assert len(dlq_records) == 2, "Both the faulty parent and its cascaded dependent child must reside in the DLQ."
    
    # Validate parent record details
    assert dlq_records[0]["task_id"] == 1
    assert "ValueError" in dlq_records[0]["reason"], "The error description must contain the exception trace details."
    
    # Validate cascaded child record details
    assert dlq_records[1]["task_id"] == 2
    assert "Cascaded failure" in dlq_records[1]["reason"], "The child node must be flagged as a cascaded failure."

