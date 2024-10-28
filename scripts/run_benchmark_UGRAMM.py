#!/usr/bin/python3

import subprocess
import time
import random

# Parameters
num_runs = 25
NR = 8
NC = 8
config_file = "../config.json"  # Update to remove space before `--config`
benchmark_1 = "../Kernels/Conv_Balance/conv_nounroll_Balance.dot"
benchmark_2 = "../Kernels/Conv_Balance/conv_nounroll_Balance_Any.dot"
command_to_run = "./../UGRAMM"

def run_benchmark(seed, NR, NC, benchmark, config_file):
    # Construct the command as a list
    command = [
        command_to_run,
        "--drc_disable",
        "--seed", str(seed),
        "--dfile", f"riken_{NR}_{NC}.dot",
        "--afile", benchmark,
        "--config", config_file,
        "--verbose_level", "0"
    ]
    
    print("Running command:", ' '.join(command))  # Print the command for debugging
    
    # Measure the runtime
    start_time = time.time()
    
    try:
        # Run the command and capture the output
        result = subprocess.run(command, capture_output=True, text=True, timeout=300)
        runtime = time.time() - start_time
        
        # Check if it passed (assuming "pass" in the output indicates success)
        if "UGRAMM_PASSED" in result.stdout.upper():
            return True, runtime
        else:
            return False, runtime
    except subprocess.TimeoutExpired:
        print(f"Benchmark timed out with seed {seed}")
        return False, float('inf')
    except FileNotFoundError as e:
        print(f"Command not found: {e}")
        return False, float('inf')

def benchmark_runner(benchmark, seeds):
    pass_count = 0
    total_runtime = 0.0
    
    for i, seed in enumerate(seeds):
        passed, runtime = run_benchmark(seed, NR, NC, benchmark, config_file)
        
        if passed:
            pass_count += 1
        total_runtime += runtime
        
        # Output the progress for each run
        print(f"Run {i+1}/{len(seeds)}: Seed = {seed}, Passed = {passed}, Time = {runtime:.4f}s")

    pass_rate = pass_count / len(seeds) * 100
    average_runtime = total_runtime / len(seeds)
    
    return pass_rate, average_runtime

# Generate a list of random seeds
seeds = [random.randint(0, 1000000) for _ in range(num_runs)]

# Running benchmark 1
print("Running benchmark 1...")
pass_rate_1, avg_time_1 = benchmark_runner(benchmark_1, seeds)

# Running benchmark 2
print("Running benchmark 2...")
pass_rate_2, avg_time_2 = benchmark_runner(benchmark_2, seeds)

# Print final results
print("\nResults:")
print(f"Benchmark 1 {benchmark_1} | Pass Rate: {pass_rate_1:.2f}% | Average Time: {avg_time_1:.4f}s")
print(f"Benchmark 2 {benchmark_2} | Pass Rate: {pass_rate_2:.2f}% | Average Time: {avg_time_2:.4f}s")
