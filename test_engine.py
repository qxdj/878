import core_engine
import time

def execute_step(node_name):
    print(f"[DAG Activity] Processing: {node_name}")
    time.sleep(0.4)

if __name__ == "__main__":
    # Initialize with multiple workers to track concurrency
    scheduler = core_engine.JobScheduler(threads=4)

    print("Populating Directed Acyclic Graph structure asynchronously...")

    # Node 4 requires both 2 and 3 to exit first. It has high priority (0) but must wait.
    scheduler.submit_job(id=4, priority=0, parents=[2, 3], func=lambda: execute_step("Node 4 (Terminal Sink)"))

    # Nodes 2 and 3 require Node 1 to exit first.
    scheduler.submit_job(id=2, priority=5, parents=[1], func=lambda: execute_step("Node 2 (Branch Alpha)"))
    scheduler.submit_job(id=3, priority=2, parents=[1], func=lambda: execute_step("Node 3 (Branch Beta)"))

    # Node 1 has no parents. It triggers the entire system cascade.
    scheduler.submit_job(id=1, priority=10, parents=[], func=lambda: execute_step("Node 1 (Root Ingestion)"))

    print("Graph topology registered. Evaluating thread-safe node resolution...")
    time.sleep(2.0)

