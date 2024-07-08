## GRAMM

- GRAph Minor Mapper (GRAMM) is an novel technique which uses Graph Minor method for CGRA Mapping which significantly speed up the Mapping process. 
- More details could be find in this publication: [here](https://ieeexplore.ieee.org/document/10296406)
- If referred, kindly cite the research using:
``` 
G. Zhou, M. Stojilović and J. H. Anderson, "GRAMM: Fast CGRA Application Mapping Based on A Heuristic for Finding Graph Minors," 2023 33rd International Conference on Field-Programmable Logic and Applications (FPL), Gothenburg, Sweden, 2023, pp. 305-310, doi: 10.1109/FPL60245.2023.00052.
```

## How to Use:

### Config according to the need:

```
#define RIKEN 1                 //Defining the architecture type [TODO: get rid of this in future, making GRAMM universal!!]
#define DEBUG 0                 //For enbaling the print-statements (mapped-DFG)
#define HARDCODE_DEVICE_MODEL 1 //Controls hardcoding of device model
```
### Compile:

```
make #Generates a binary called GRAMM in the root directory
```

### Running a circuit on GRAMM:
```
[bhilareo@p181 GRAMM]$ ./GRAMM --help
Arguments are <application.dot> <device_model.dot> <numRows NR> <numCols NC> <seed>

# Argument 1 & 2: Dot file for application and device model
# Argument 3 & 4: Size of the CGRA (NR  & NC)
# Argument 5: Seed for the run.
```

### Mapping `Conv_Balance` circuit on 8X8 ADRES CGRA using GRAMM
> ./GRAMM Kernels/Conv_Balance/conv_nounroll_Balance.dot 2 2 0

---

## Helper Script usage:

> ./run_gramm.sh Kernels/Conv_Balance/conv_nounroll_Balance.dot 8 8   

- run_gramm.sh script:
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