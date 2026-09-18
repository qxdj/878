import core_engine
import time
import pytest

def test_dag_execution_topology():
    scheduler = core_engine.JobScheduler(threads=4)
    execution_order = []

    # 1. Define execution steps that log their identity when fired
    def step_one():
        execution_order.append(1)

    def step_two():
        time.sleep(0.1)  # Simulate small workload
        execution_order.append(2)

    def step_three():
        execution_order.append(3)

    def step_four():
        execution_order.append(4)

    # 2. Submit jobs out of logical order with explicit parent dependencies
    # Node 4 requires 2 and 3 to finish
    scheduler.submit_job(id=4, priority=1, parents=[2, 3], func=step_four)
    # Node 2 and 3 require Node 1 to finish
    scheduler.submit_job(id=2, priority=5, parents=[1], func=step_two)
    # Node 3 has higher priority than 2, so it should finish before 2 once 1 is done
    scheduler.submit_job(id=3, priority=2, parents=[1], func=step_three)
    # Node 1 has no parents and kicks off everything
    scheduler.submit_job(id=1, priority=10, parents=[], func=step_one)

    # 3. Wait for workers to drain the graph execution lanes
    time.sleep(0.6)

    # 4. Verify mathematical topological correctness
    assert execution_order[0] == 1, "Root Node 1 must always execute first."
    assert execution_order[-1] == 4, "Terminal Sink Node 4 must always execute last."
    assert 2 in execution_order and 3 in execution_order, "Parallel branch components failed to run."

