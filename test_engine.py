import core_engine
import time
import os

# Define a Python workload to be scheduled by the C++ engine
def heavy_math_workload(job_id):
    print(f"[Python] Job {job_id} running concurrently on Process PID: {os.getpid()}")
    time.sleep(1) # Simulate complex arithmetic blocking
    print(f"[Python] Job {job_id} successfully completed.")

if __name__ == "__main__":
    print("Initializing C++ Concurrency Engine...")
    # Spin up the scheduler with 4 underlying C++ worker threads
    scheduler = core_engine.JobScheduler(threads=4)

    print("Submitting 6 Python workloads to the C++ priority queues...")
    for i in range(6):
        # Pass lambda tasks into the compiled C++ framework
        scheduler.submit_job(lambda idx=i: heavy_math_workload(idx))

    print("Main program thread continuing execution asynchronously...")
    # Give the background worker threads time to finish processing
    time.sleep(3)
    print("Execution complete.")

