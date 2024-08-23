## GRAMM

- GRAph Minor Mapper (GRAMM) is an novel technique which uses Graph Minor method for CGRA Mapping which significantly speed up the Mapping process. 
- More details could be find in this publication: [here](https://ieeexplore.ieee.org/document/10296406)
- If referred, kindly cite the research using:
``` 
G. Zhou, M. Stojilović and J. H. Anderson, "GRAMM: Fast CGRA Application Mapping Based on A Heuristic for Finding Graph Minors," 2023 33rd International Conference on Field-Programmable Logic and Applications (FPL), Gothenburg, Sweden, 2023, pp. 305-310, doi: 10.1109/FPL60245.2023.00052.
```


## Helper Script usage:

The following command runs the maps the DFG on the Riken Device model that does not contains any pins
> ./run_gramm.sh Kernels/Conv_Balance/conv_nounroll_Balance.dot 8 8 RIKEN 

The following command runs the maps the DFG on the Riken Device model that does contains any pins. This will be the main command used for the GRAMM mapping.
> ./run_gramm.sh Kernels/Conv_Balance/conv_nounroll_Balance.dot 8 8 RIKEN_with_pins

- run_gramm.sh script:
    - Arugment info:
        - $1 [Kernel]: Kernels/Conv_Balance/conv_nounroll_Balance.dot 
        - $2 [NR] = 8
        - $3 [NC] = 8
        - 4 [Arch_Type for device model] = RIKEN_with_pins or RIKEN
            - "RIKEN" Argument produces device model without pins meanwhile "RIKEN_with_pins" produce architecture for RIKEN with pins
    - Generating device model using external script
        - `cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch RIKEN && cd ..`
    - Executes GRAMM and produces mapping result in mapping_output.dot
        - `make && ./GRAMM $1 $device_model_output $2 $3 0`
    - Finally converts the mapped output dot file into png
        - `neato -Tpng mapping_output.dot -o mapping_output.png`
    - Successful mapping result will be in `mapping_output.png`

## Hardcoded mapping version:


<img src="assets/images/mapping_output_hardcoded.png" alt="mapping_output_hardcoded" width="600"/>

## External script-based version [Identical to hardcoded version]:

<img src="assets/images/mapping_output_dot_input.png" alt="mapping_output_dot_input" width="600"/>


## Benchmarks conversion script
There are two scripts used to covert the current benchmarks. There scripts are 
- benchmark_modification.py 
- application_graph_modification.py

Most of the benchmark currently has the "Operand" property defined at the edges of the DFG graph. This property defines the load pin the DFG. Unfortunately, this convention will not be used in the GRAMM mapper. Instead, the edges will be defined through "driver" and "load" properties. These new properties will be used to define the input and output pins in the DFG path. 

The main script that everyone will be using is the benchmark_modification.py. Since most benchmarks in the kernel suite has edges with "Operand" property, the benchmark_modification.py will convert the edge properties to the new convention mentioned above. In addition, some of the "Operand" property can be set as "Any2Pins", meaning that the edge can be mapped to any load pins of the PEs. This creates some confusion, so this script will automatically assign a pins while adhearing to the pins that need to be mapped to certain pins in the PE.  Also, if the user wishes, we can have multiple pins assigned to the operand property of the input dot file. For instance, the load property in the input dot file will look as the following: "operand=inPinA.inPinB". It is important to have a "." between each pins to help script distinguish between the pins

During testing, we created some benchmarks that has the extension "_Tester", for example "conv_nounroll_Balance_Tester". These benchmarks are already formarted their edges to have the "driver" and "load" properties. The only thing is that the "load" proporty can be set to "AnyPins, meaning that the edge can be mapped to any load pins of the PEs. This creates some confusion, so the application_graph_modification.py script will automatically assign a pins while adhearing to the pins that need to be mapped to certain pins in the PE. Similar to the other script, if the user wishes, we can have multiple pins assigned to the load property of the input dot file. For instance, the load property in the input dot file will look as the following: "load=inPinA.inPinB". It is important to have a "." between each pins to help script distinguish between the pins


### benchmark_modification.py
> ./benchmark_modification.py -Benchmark <Path_To_Benchmark_File> -OutputDir <Path_To_Output_Kernal_Directory>
> ./benchmark_modification.py -Benchmark ../Kernels/Conv_Balance/conv_unroll3.dot -OutputDir ../Kernels_Modified

### application_graph_modification.py
> ./application_graph_modification.py -Benchmark <Path_To_Benchmark_File> -OutputDir <Path_To_Output_Kernal_Directory>
> ./application_graph_modification.py -Benchmark ../Kernels/Conv_Balance/conv_nounroll_Balance_Tester_Fanout.dot -OutputDir ../Kernels_Modified

For Riken architecture, constantant can not have a fan out as they can not be routed back to the switch block. In any typical mapping, we would say that the mapping failed as the device model graph does not allow this application DFG feature. However, we have created a helper script for Riken arhcitecture that duplicated the constant nodes to give the illusion of constant fanout. One important thing to mention is that the output dot file will be called "modified_{filename}.dot". This will be fixed in the future. 
> ./constant_duplication.py -Benchmark <Path_To_Benchmark_File> -OutputDir  <Path_To_Output_Kernal_Directory>
> ./constant_duplication.py -Benchmark ../Kernels_Modified/Conv_Balance/conv_nounroll_Balance_Tester_Fanout_Constant.dot -OutputDir ../Kernels_Modified