#!/bin/bash

#Default architecture set to RIKEN in this helper script
#Seed is default to 0
# $1 : Kernels/Conv_Balance/conv_nounroll_Balance.dot 
# device model will be for NR=NC=8 :: scripts/riken_8_8.dot

# Generating device model:
echo "---------------------Generating device model using external script:---------------------"
cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch RIKEN && cd ..

# Executes GRAMM and producing mapping result in mapping_output.dot
echo "---------------------Executing GRAMM and producing mapping result in mapping_output.dot---------------------"
device_model_output="scripts/riken_$2_$3.dot"
make && ./GRAMM $1 $device_model_output $2 $3 0

# Converting the mapped output dot file into png:
echo "---------------------Converting the mapped  mapping_output.dot file into mapping_output.png:---------------------"
neato -Tpng mapping_output.dot -o mapping_output.png