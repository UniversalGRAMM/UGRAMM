## GRAMM helper scripts:

1. **device_model_gen.py**
    - **Usage:** `device_model_gen.py [-h] [-NR NR] [-NC NC] [-Arch ARCH]`
    - **Example:** `./device_model_gen.py -NR 8 -NC 8 -Arch RIKEN`
    - Generates a device model for the specified CGRA architecture.
    - When the script is executed, the output will be saved as <Arch_Name>_<NR>_<NC>.dot.
    - **Architecture supported:**
        - RIKEN
2. **application_graph_modification.py**
    - **Usage:** 
    - **Example:** 
    - @HamasWaqar can you please add a small description here.
3. **benchmark_modification.py**
    - **Usage:** 
    - **Example:** 
    - @HamasWaqar can you please add a small description here.
4. **constant_duplication.py**
    - **Usage:** 
    - **Example:** 
    - @HamasWaqar can you please add a small description here.

---

## Benchmarks conversion script
There are two scripts used to covert the current benchmarks. There scripts are 
- benchmark_modification.py 
- application_graph_modification.py

Most of the benchmark currently has the "Operand" property defined at the edges of the DFG graph. This property defines the load pin the DFG. Unfortunately, this convention will not be used in the GRAMM mapper. Instead, the edges will be defined through "driver" and "load" properties. These new properties will be used to define the input and output pins in the DFG path. 

The main script that everyone will be using is the benchmark_modification.py. Since most benchmarks in the kernel suite has edges with "Operand" property, the benchmark_modification.py will convert the edge properties to the new convention mentioned above. In addition, some of the "Operand" property can be set as "Any2Pins", meaning that the edge can be mapped to any load pins of the PEs. This creates some confusion, so this script will automatically assign a pins while adhearing to the pins that need to be mapped to certain pins in the PE. 

During testing, we created some benchmarks that has the extension "_Tester", for example "conv_nounroll_Balance_Tester". These benchmarks are already formarted their edges to have the "driver" and "load" properties. The only thing is that the "load" proporty can be set to "AnyPins, meaning that the edge can be mapped to any load pins of the PEs. This creates some confusion, so the application_graph_modification.py script will automatically assign a pins while adhearing to the pins that need to be mapped to certain pins in the PE.

### benchmark_modification.py
> ./benchmark_modification.py -Benchmark <Path_To_Benchmark_File> -OutputDir <Path_To_Output_Kernal_Directory>
> ./benchmark_modification.py -Benchmark ../Kernels/Conv_Balance/conv_unroll3.dot -OutputDir ../Kernels_Modified

### benchmark_modification.py
> ./application_graph_modification.py -Benchmark <Path_To_Benchmark_File> -OutputDir <Path_To_Output_Kernal_Directory>
> ./application_graph_modification.py -Benchmark ../Kernels/Conv_Balance/conv_nounroll_Balance_Tester_Fanout.dot -OutputDir ../Kernels_Modified

For Riken architecture, constantant can not have a fan out as they can not be routed back to the switch block. In any typical mapping, we would say that the mapping failed as the device model graph does not allow this application DFG feature. However, we have created a helper script for Riken arhcitecture that duplicated the constant nodes to give the illusion of constant fanout. One important thing to mention is that the output dot file will be called "modified_{filename}.dot". This will be fixed in the future. 
> ./constant_duplication.py -Benchmark <Path_To_Benchmark_File> -OutputDir  <Path_To_Output_Kernal_Directory>
> ./constant_duplication.py -Benchmark ../Kernels_Modified/Conv_Balance/conv_nounroll_Balance_Tester_Fanout_Constant.dot -OutputDir ../Kernels_Modified