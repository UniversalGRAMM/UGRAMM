#!/usr/bin/python3

import subprocess
import time
import random
import glob
import argparse
import re
from tabulate import tabulate  # Imp ort tabulate for formatting tables
from statistics import geometric_mean
import statistics

# Parameters
num_runs = 20
NR = 10
NC = 10
config_file = "../config.json"  # Update to remove space before `--config`
command_to_run = "./../UGRAMM"


def run_benchmark(args, seed, NR, NC, benchmark, config_file):
    # Construct the command as a list
    command = [
        command_to_run,
        "--drc_disable",
        "--pfac_mul", str(args.pfac_mul),
        "--hfac_mul", str(args.hfac_mul),
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
        
        log_output = result.stdout
        match = re.search(r"Routing resources percentage usage\s*:\s*(\d+(\.\d+)?)\s*%", log_output)
        
        if match:
            rrused = float(match.group(1))

        # Check if it passed (assuming "UGRAMM_PASSED" in the output indicates success)
        if "UGRAMM_PASSED" in result.stdout.upper():
            return True, runtime, rrused 
        else:
            return False, runtime, float('inf')

    except subprocess.TimeoutExpired:
        print(f"Benchmark timed out with seed {seed}")
        return False, float('inf'), float('inf')
    except FileNotFoundError as e:
        print(f"Command not found: {e}")
        return False, float('inf'), float('inf')

def benchmark_runner(args, benchmark, seeds):
    pass_count = 0
    total_runtime = 0.0
    total_routing_resources = 0.0
    rt = []
    rr = [] 

    for i, seed in enumerate(seeds):
        passed, runtime, rrused = run_benchmark(args, seed, NR, NC, benchmark, config_file)
        
        if passed:
            pass_count += 1
            total_runtime += runtime  # Only add runtime if the benchmark passed
            rt.append(runtime)
            total_routing_resources += rrused 
            rr.append(rrused)
            
        # Output the progress for each run
        print(f"Run {i+1}/{len(seeds)}: Seed = {seed}, Passed = {passed}, Time = {runtime:.4f}s, Routing resources used = {rrused} %")

    pass_rate = (pass_count / len(seeds)) * 100
    average_runtime = total_runtime / pass_count if pass_count > 0 else 0  # Avoid division by zero
    average_rrused  = total_routing_resources / pass_count if pass_count > 0 else 0 
    
    if len(rt) == 0:
        stddev_rt = 0
    else:
        stddev_rt = statistics.stdev(rt)

    if len(rr) == 0:
        stddev_rr = 0
    else:
        stddev_rr = statistics.stdev(rr)

    return pass_rate, average_runtime, average_rrused, stddev_rt, stddev_rr

def main():
    # Setup argument parser
    parser = argparse.ArgumentParser(description="Run benchmarks with UGRAMM.")
    parser.add_argument("benchmark_type", type=int, choices=[0, 1],
                        help="Enter 0 for Any benchmarks or 1 for Specific benchmarks.")
    parser.add_argument("pfac_mul", type=float, 
                        help="Enter multiplying value for PFAC (greater than 1)")
    parser.add_argument("hfac_mul", type=float, 
                        help="Enter  multiplying value for HFAC (Value inbetween 0.2 to 1)")
    
    args = parser.parse_args()
    benchmark_choice = args.benchmark_type

    # Generate a list of random seeds
    seeds = [random.randint(0, 1000000) for _ in range(num_runs)]

    # Collect benchmark files based on user choice
    if benchmark_choice == 0:
        benchmark_files = glob.glob("../Kernels/**/*_Any.dot", recursive=True)
    elif benchmark_choice == 1:
        benchmark_files = glob.glob("../Kernels/**/*_Specific.dot", recursive=True)

    # Filter out benchmarks containing specified substrings
    excluded_substrings = ["unroll3", "unroll5", "unroll4"]
    new_benchmark_files = []
    for b in benchmark_files:
        if not any(sub in b for sub in excluded_substrings):
            if  ("_Balance_" in b) or ("_nonBalance_" in b) or ("fft_radix4_Specific" in b ) or ("fft_radix5_Specific" in b ):
                new_benchmark_files.append(b)

    # Print the found benchmark files
    print("Found benchmark files:")
    for benchmark in new_benchmark_files:
        print(f" - {benchmark}")

    # List to hold results for tabulation
    results = []
    
    # Generate the device-model based on set arguments:
    subprocess.run(f"python3 device_model_gen.py -NR {NR} -NC {NC} -Arch RIKEN", shell=True)


    # Running benchmarks for all found files
    for benchmark in new_benchmark_files:
        benchmark_type = "Any" if "_Any.dot" in benchmark else "Specific"
        print(f"\nRunning benchmark for {benchmark_type}: {benchmark}...")
        pass_rate, avg_time, average_rrused, std_dev_runtime, std_dev_routing_used = benchmark_runner(args, benchmark, seeds)

        # Append the results for this benchmark
        results.append([benchmark, benchmark_type, f"{pass_rate:.2f}%", f"{avg_time:.4f}s", f"{average_rrused:.2f}%", f'{std_dev_runtime:.2f}',  f'{std_dev_routing_used:.2f}'])

    # Print final results in tabular format
    print("\nFinal Results:")
    print(tabulate(results, headers=["Benchmark", "Type", "Pass Rate", "Average Time", "Average Routing Usage", "std_dev_runtime" , "std_dev_routing_used"], tablefmt="pretty"))

    # Calculate geometric mean::
    avg_times = [float(result[3][:-1]) for result in results]
    runtime_geomean = geometric_mean(avg_times)
    print(f" [SUMMARY] runtime geomean : {runtime_geomean:.2f} seconds ")

    avg_rr = [float(result[4][:-1]) for result in results]
    rr_geomean = geometric_mean(avg_rr)
    print(f" [SUMMARY] routing resource usage geomean : {rr_geomean:.2f} %")

if __name__ == "__main__":
    main()
