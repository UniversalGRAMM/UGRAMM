## GRAMM helper scripts:

1. **device_model_gen.py**
    - **Usage:** `device_model_gen.py [-h] [-NR NR] [-NC NC] [-Arch ARCH]`
    - **Example:** `./device_model_gen.py -NR 8 -NC 8 -Arch RIKEN`
    - Generates a device model for the specified CGRA architecture.
    - When the script is executed, the output will be saved as <Arch_Name>_<NR>_<NC>.dot.
    - **Architecture supported:**
        - RIKEN
2. **benchmark_modification.py**
    - **Usage:** `./benchmark_modification.py [-Benchmark Path_To_Benchmark_File] [-OutputDir Path_To_Output_Kernal_Directory]`
    - **Example:** `./benchmark_modification.py -Benchmark ../Kernels/Conv_Balance.dot -OutputDir ../Kernels_Modified`
    - Reformat the application kernels provided by CGRA-ME into dot files format that is supported by UGRAMM
    - CGRA-ME kernels are formatted using DOT graphs. However, these kernels include both node and edge attributes that are not supported by UGRAMM. A detailed list of these differences is provided below:
        - Operand Attribute: CGRA-ME kernels define node pins as PinA, PinC, or Any2Pins, stored within the Operand attribute. However, UGRAMM requires more specificity in distinguishing between load and driver pins. Thus, the Operand attribute is replaced by Driver and Load attributes. Additionally, UGRAMM does not support the concept of Any2Pins, so each Driver and Load attribute must be assigned specific supported pins or a vector of supported pins.
        - Shape Attribute: CGRA-ME kernels use shape attributes primarily to assist in generating graphical outputs. This is unnecessary for UGRAMM and is therefore removed.
        - Type Attribute: CGRA-ME kernels use the type attribute to distinguish operation nodes in the application graph. In UGRAMM, this is handled through the pragma and opcode attributes, so the type attribute is not needed and is removed.
    Additionally, the current CGRA-ME kernels do not include the pragma support required by UGRAMM. Therefore, this script reformats the kernels into DOT graphs compatible with UGRAMM.
    - Do note that this will be the **primary script used** when converting CGRA-ME supported kernels
    - When the script is executed, the output dot file will be saved as <Benchmark_File_Name>.dot (ex. `Conv_Balance.dot`)
3. **constant_duplication.py**
    - **Usage:** `./constant_duplication.py [-Benchmark Path_To_Benchmark_File] [-OutputDir  Path_To_Output_Kernal_Directory]`
    - **Example:** `./constant_duplication.py -Benchmark ../Kernels/Conv_Balance.dot -OutputDir ../Kernels_Modified`
    - Duplicate fanout constant in the application graphs. 
    - In some architectures (ex. Riken CGRA) does not support fanout constantants as they can not route a signal back to the switch block. Hence, constant duplication is required to successfully map application graph onto the device model graph.
    - When the script is executed, the output dot file will be saved as <Benchmark_File_Name>.dot (ex. `Conv_Balance.dot`)
