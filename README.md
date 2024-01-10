## GRAMM

- GRAph Minor Mapper (GRAMM) is an novel technique which uses Graph Minor method for CGRA Mapping which significantly speed up the Mapping process. 
- More details could be find in this publication: [here](https://ieeexplore.ieee.org/document/10296406)
- If referred, kindly cite the research using:
``` 
G. Zhou, M. Stojilović and J. H. Anderson, "GRAMM: Fast CGRA Application Mapping Based on A Heuristic for Finding Graph Minors," 2023 33rd International Conference on Field-Programmable Logic and Applications (FPL), Gothenburg, Sweden, 2023, pp. 305-310, doi: 10.1109/FPL60245.2023.00052.
```

## How to Use:

### Compile:

```
make #Generates a binary called GRAMM in the root directory
```

### Running a circuit on GRAMM:
```
[bhilareo@p181 GRAMM]$ ./GRAMM --help
Arguments are <filename.dot> <numRows NR> <numCols NC> <seed>

# Argument 1: Dot file of the circuit which needs to be mapped (some microbenchmark circuits are provided under microbench/)
# Argument 2 & 3: Size of the CGRA (NR  & NC)
# Argument 4: Seed for the run.
```

### Mapping `accmulate` circuit on 8X8 ADRES CGRA using GRAMM
> ./GRAMM microbench/accumulate/graph_loop.dot 8 8 0
