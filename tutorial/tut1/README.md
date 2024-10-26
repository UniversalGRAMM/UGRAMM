# UGRAMM Tutorial-1: Mapping Digital Circuits to Hardware Modules

{: .note }
Please refer to the Getting Started Guide (available at: https://universalgramm.github.io/getting-started) of UGRAMM before attempting this tutorial.

# Table of Contents

1. [Description](#description)
2. [Application-dot file](#application-dot-file)
3. [Device-model file](#device-model-file)
4. [Running UGRAMM](#running-ugramm)

## Description:

UGRAMM is a flexibile tool which can be used not only for CGRA mapping but for any application where we need to find embedding of a small graph in a large graph.

In this tutorial we have mapped a digital design input into to Hardware Modules, mapping here corresponds to one-to-one matching of a particular operation from the input digital design to the hardware units available in the device-model.

```mermaid
    graph TD
        AFILE(**AFILE**: Application file describing digital design input) --> **Universal-GRAMM**
        DFILE(**DFILE**: Device-model graph file describing hardware modules circuitry) --> **Universal-GRAMM**
        CFILE(**CFILE**: Config file, optional in case) -.-> **Universal-GRAMM**
```


## Application-dot file:

<div style="text-align: center;">
  <table style="margin: auto;">
    <tr>
        <td align="center">
        <img src="afile_graph.png" alt="Fig 1. Digital Circuit Design" width="240"/>
        <br> <b>Digital Circuit Design</b>
        </td>
        <td align="center">
        <img src="afile_dot.png" alt="Fig 2. Digital Design in Dot Format" width="420"/>
        <br> <b>Digital Circuit Design in Dot Format </b>
        </td>
    </tr>
  </table>
</div>

### Application Dot File Overview 

> **AFile Location:** `UGRAMM/tutorial/tut1/application.dot`
  
As illustrated in Fig 1, the digital circuit design of a two-input AND gate can also be represented in [Graphviz dot format](https://graphviz.org/doc/info/lang.html). The conversion of this circuit into dot format is provided below in the provided code snippet, along with the graph output shown in Fig 2.

In the application dot file, all nodes are classified as **Function Cells (FuncCells)** according to the UGRAMM framework. For more details, refer to the defined cell types of UGRAMM [here](https://universalgramm.github.io/node-type.html).

### PRAGMA Section

The dot file begins with a **PRAGMA** statement, which is followed by the main content of the dot file. The PRAGMA specifies the supported operations for a FuncCell in a format defined by UGRAMM. More information about the supported pragmas can be found [here](https://universalgramm.github.io/Supported-Pragmas.html). 

For instance, a gate is represented as a FuncCell in the application file. A supported operation for this gate is AND, which is defined in the PRAGMA as follows:

> [SupportedOps] = {GATE, AND}; // [Keyword] = {FuncCell, SupportedOp1, SupportedOp2...};

### Application Dot Attributes Overview

| **Type**      | **Attribute** | **Description**                                                      | **Example**                        |
|---------------|---------------|----------------------------------------------------------------------|------------------------------------|
| Node          | **label**     | The display name of the node, shown in graphical representations.   | `label="Input1"`                  |
| Node          | **opcode**    | Specifies the opcode of the FuncCell, indicating its functionality.   | `opcode=INPUT` or `opcode=AND`                   |
| Edge          | **driver**    | Specifies the driver pin to use for the connection  | `driver="outPinA"`                |
| Edge          | **load**      | Specifies the load pin to use for the connection | `load="inPinA"`                   |

- **Example:** `Input1 -> And1  [driver="outPinA", load="inPinA"];`
  - UGRAMM will establish a connection from Input1's `outPinA` to And1's `inPinA`.
  - Both FuncCells must have the required pins defined in the device-model dot file with matching names; otherwise, UGRAMM will throw a FATAL ERROR.


```
 /* ------- Application graph pragma ------- 
[SupportedOps] = {GATE, AND};           //Supported Operation by "GATE" FuncCell is "AND" 
[SupportedOps] = {IO, INPUT, OUTPUT};   //Supported Operations by "IO" FuncCell are "INPUT", "OUTPUT"
*/
strict digraph "tutorialUGRAMM" {

//Node information:
Input1 [label="Input1", opcode=INPUT];  //Input-IO node 
Input2 [label="Input2", opcode=INPUT];  //Input-IO node 
And1 [label="And1", opcode=AND];        //And-gate defined as GATE node
Output [label="Output1", opcode=OUTPUT]; //Output-IO node

//Edge information:
Input1 -> And1  [driver="outPinA", load="inPinA"];
Input2 -> And1  [driver="outPinA", load="inPinB"];
And1 -> Output  [driver="outPinA", load="inPinA"];
}
```

## Device-model file:

<div style="text-align: center;">
  <img src="dfile_graph.png" alt="Fig 3. Device model" width="720"/>
  <br> <b> Fig 3. Device model </b>
</div>

### Device-model Dot File Overview 

> **DFile Location:** `UGRAMM/tutorial/tut1/device-model.dot`

The device-model dot file illustrates the available hardware modules and their interconnections within the system. UGRAMM aims to embed a smaller application graph into this larger device model graph. The circuitry shown in Fig 3 can also be represented in dot format, as shown in the code snippet below or in `tut1/device-model.dot`.

In contrast to the application dot file, the device model includes all three cell types: **FuncCell**, **RouteCell**, and **PinCell**. In UGRAMM, every **FuncCell** must have output and/or input pin interfaces. These **FuncCells** are interconnected using these pins, often with **RouteCells** facilitating the connections in between.

For example, the connection between the **Gate FuncCell** shown in the Fig 3. and the far-right **IO FuncCell** can be described as follows: **{outPin of Gate, Reg (RouteCell), inputPin of IO, IO (FuncCell)}**.

Each cell-type in UGRAMM has its associated opcodes. For example, a **PinCell** may include opcodes such as **IN** and **OUT** describing Input pin cells and Output pin cells respectively. The details are outlined as follows:

```mermaid
graph TD
    A[CELL] --> B[FuncCell]
    A --> C[RouteCell]
    A --> D[PinCell]
    
    B --> E[OpCode: GATE]
    B --> L[OpCode: IO]
    C --> F[OpCode: MUX]
    C --> G[OpCode: REG]
    D --> H[OpCode: IN]
    D --> I[OpCode: OUT]
```

### PRAGMA Section

The device-model dot file begins with a **PRAGMA** statement, similar to the application dot file, which specifies the operations supported by various FuncCells within the device model.

It is important to ensure that the operations defined in the device model align with the requirements specified in the application dot file's PRAGMA. For example, if the application dot file requests a NOR operation from a GATE FuncCell, but the device model only supports an AND operation for GATE FuncCell, then MAPPING will not be feasible.

For example, the PRAGMA section is defined as follows:

```plaintext
/* ------- Device model pragma ------- 
[SupportedOps] = {GATE, AND};  
[SupportedOps] = {IO, INPUT, OUTPUT};
*/
```

### Device-model Dot Attributes Overview

| **Required/Optional**      | **Attribute** | **Description**                                                      | **Example**                        |
|---------------|---------------|----------------------------------------------------------------------|------------------------------------|
| Required          | **G_Name**    | The unique name of the node        | `G_Name="IO1.inPinA"`             |
| Required          | **G_CellType**| Specifies the type of the cell (e.g., PinCell, FuncCell, RouteCell).          | `G_CellType="PinCell"`             |
| Required          | **G_NodeType**| Indicates the opCode of the node in the architecture (e.g., PinCell supports IN, OUT).| `G_NodeType="IN"`                  |
| Optional          | **G_VisualX** | The X-coordinate for visual representation in the layout.          | `G_VisualX="0"`                    |
| Optional          | **G_VisualY** | The Y-coordinate for visual representation in the layout.          | `G_VisualY="3"`                    |

<details>
<summary> <b> Device Model Pragma Code </b> </summary>

```plaintext
/* ------- Device model pragma ------- 
[SupportedOps] = {GATE, AND};  
[SupportedOps] = {IO, INPUT, OUTPUT};
*/
strict digraph  {
0  [G_Name="IO1.inPinA", G_CellType="PinCell", G_NodeType="IN", G_VisualX="0", G_VisualY="3"];
1  [G_Name="IO1", G_CellType="FuncCell", G_NodeType="IO", G_VisualX="0.75", G_VisualY="3"];
2  [G_Name="IO1.outPinA", G_CellType="PinCell", G_NodeType="OUT", G_VisualX="1.5", G_VisualY="3"];

3  [G_Name="IO2.inPinA", G_CellType="PinCell", G_NodeType="IN", G_VisualX="0", G_VisualY="1"];
4  [G_Name="IO2", G_CellType="FuncCell", G_NodeType="IO", G_VisualX="0.75", G_VisualY="1"];
5  [G_Name="IO2.outPinA", G_CellType="PinCell", G_NodeType="OUT", G_VisualX="1.5", G_VisualY="1"];

6 [G_Name="MUX1", G_CellType="RouteCell", G_NodeType="MUX", G_VisualX="2.5", G_VisualY="3"];
7 [G_Name="MUX2", G_CellType="RouteCell", G_NodeType="MUX", G_VisualX="2.5", G_VisualY="1"];

8  [G_Name="Gate1.inPinA", G_CellType="PinCell", G_NodeType="IN", G_VisualX="3", G_VisualY="3"];
9  [G_Name="Gate1.inPinB", G_CellType="PinCell", G_NodeType="IN", G_VisualX="3", G_VisualY="1"];
10  [G_Name="Gate1", G_CellType="FuncCell", G_NodeType="GATE", G_VisualX="3.75", G_VisualY="2"];
11  [G_Name="Gate1.outPinA", G_CellType="PinCell", G_NodeType="OUT", G_VisualX="4.5", G_VisualY="2"];

12 [G_Name="REG1", G_CellType="RouteCell", G_NodeType="REG", G_VisualX="5.5", G_VisualY="2"];

13  [G_Name="IO3.inPinA", G_CellType="PinCell", G_NodeType="IN", G_VisualX="6", G_VisualY="2"];
14  [G_Name="IO3", G_CellType="FuncCell", G_NodeType="IO", G_VisualX="6.75", G_VisualY="2"];
15  [G_Name="IO3.outPinA", G_CellType="PinCell", G_NodeType="OUT", G_VisualX="7.5", G_VisualY="2"];


//1st I/O layer:
0 -> 1; //Connection of input pin to the I/O block
1 -> 2; //Connection of I/O block to the output pin 

3 -> 4; //Connection of input pin to the I/O block
4 -> 5; //Connection of I/O block to the output pin 

//Interconnect layer:
2 -> 6;
5 -> 6;

2 -> 7;
5 -> 7;

//Gate layer:
6 -> 8;
7 -> 9;
8 -> 10;
9 -> 10;
10 -> 11;

//Reg layer:
11 -> 12;

//I/O layer:
12 -> 13;
13 -> 14;
14 -> 15;
}
```
</details>

## Running UGRAMM:

UGRAMM mapping algorithm is described in detailed in this paper: https://ieeexplore.ieee.org/document/10296406

```
cd tutorial/tut1/
../../UGRAMM --afile application.dot --dfile device-model.dot --config config.json --verbose_level 0 --seed 0 
dot -Tpng unpositioned_dot_output.dot -o unpositioned_dot_output.png
neato -Tpng positioned_dot_output.dot -o positioned_dot_output.png 
```
### unpositioned_dot_output 
- This dot-file does not contains user provided co-ordinates of the device-model node cells.
- This file also displays the input application graph for which UGRAMM was executed.
<div style="text-align: center;">
  <img src="unpositioned_dot_output.png" alt="Fig 3. Device model" width="720"/>
  <br> <b> Fig 4. unpositioned_dot_output </b>
</div>

### positioned_dot_output

- This dot-file contains user provided co-ordinates of the device-model node cells.

<div style="text-align: center;">
  <img src="positioned_dot_output.png" alt="Fig 3. Device model" width="720"/>
  <br> <b> Fig 5. positioned_dot_output </b>
</div>