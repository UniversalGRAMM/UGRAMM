#!/bin/bash

#Default architecture set to RIKEN in this helper script
#Seed is default to 0
# $1 : Kernels/Conv_Balance/conv_nounroll_Balance.dot 
# $2 = NR = 8
# $3 = NC = 8
# $4 = Arch Type = RIKEN

# Generating device model:
echo "---------------------Generating device model using external script:---------------------"
echo "cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch $4 && cd .."
cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch $4 && cd ..

# Executes GRAMM and producing mapping result in mapping_output.dot
echo " "
echo " "
echo "---------------------Executing GRAMM and producing mapping result in ordered_dot_output.dot & unpositioned_dot_output---------------------"
device_model_output="scripts/riken_$2_$3.dot"
echo "make && ./GRAMM $1 $device_model_output $2 $3 0"
make && ./GRAMM $1 $device_model_output $2 $3 0

# Converting the mapped output dot file into png:
echo " "
echo " "
echo "---------------------Converting the ordered-mapped  positioned_dot_output.dot file into ordered_dot_output.png:---------------------"
echo "neato -Tpng positioned_dot_output.dot -o positioned_dot_output.png"
neato -Tpng positioned_dot_output.dot -o positioned_dot_output.png

# Converting the mapped output dot file into png:
echo " "
echo " "
echo "---------------------Converting the unordered-mapped  unpositioned_dot_output.dot file into unpositioned_dot_output.png:---------------------"
echo "neato -Tpng unpositioned_dot_output.dot -o unpositioned_dot_output.png"
dot -Tpng unpositioned_dot_output.dot -o unpositioned_dot_output.png