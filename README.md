# Introduction to UGRAMM:

[![Documentation](https://img.shields.io/badge/Documentation-UGRAMM-blue)](https://universalgramm.github.io/)
[![Doxygen](https://img.shields.io/badge/Doxygen-UGRAMM-orange)](https://universalgramm.github.io/doxygen/)

<div style="text-align: justify;">
<br> GRAMM (GRAph Minor Mapping) is a novel CGRA mapping technique that has demonstrated an 80x CAD runtime speedup over other mapping methods <a href="https://ieeexplore.ieee.org/document/10296406">[Zhou et al.(2023)]</a>. GRAMM is based on the graph minoring concept from graph theory, which states that if we can transform graph G into H by removing its nodes and/or edges and as well by contracting edges in G, then graph H is a minor of graph G. In the graph minor example in Fig.1, red edges in graph G are removed and edges within blue circles are contracted, resulting in graph H as a minor of graph G. <br> <br>
</div>

<div style="text-align: center;">
    <img src="assets/images/minor_example.png" alt="Fig 1. Graph Minor example" style="border: 1px solid black; width: 400px;">
    <figcaption style="font-size: 14px; color: #555;">Fig 1. Graph Minor example</figcaption>
</div>

<div style="text-align: justify;">
This repository contains a more general version of GRAMM known as UGRAMM (Universal GRAph Minor Mapper), which can not only be used in CGRA mapping but as well could be easily incorporated in any other applications such as ASIC/FPGA packing, etc. <br> <br>
</div>

- <b> If referred, kindly cite the research using: </b> 
> G. Zhou, M. Stojilović and J. H. Anderson, "GRAMM: Fast CGRA Application Mapping Based on A Heuristic for Finding Graph Minors," 2023 33rd International Conference on Field-Programmable Logic and Applications (FPL), Gothenburg, Sweden, 2023, pp. 305-310, doi: 10.1109/FPL60245.2023.00052.

# License:

- The UGRAMM toolchain is licensed under this [License](https://github.com/UniversalGRAMM/UGRAMM/blob/master/LICENSE). Please review it before using UGRAMM.