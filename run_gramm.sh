#!/bin/bash

#Default architecture set to RIKEN in this helper script
#Seed is default to 0
# $1 = Seed
# $2 = NR = 8
# $3 = NC = 8
# $4 = Application graph
# $5 = GRAMM config JSON file

# Generating device model:
echo "---------------------Generating device model using external script:---------------------"
echo "cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch RIKEN && cd .."
cd scripts && ./device_model_gen.py -NR $2 -NC $3 -Arch RIKEN && cd ..

# Executes GRAMM and producing mapping result in mapping_output.dot
echo " "
echo " "
echo "---------------------Executing GRAMM and producing mapping result in ordered_dot_output.dot & unpositioned_dot_output---------------------"
device_model_output="scripts/riken_$2_$3.dot"
echo "make && ./GRAMM --seed $1 --verbose_level 0 --dfile $device_model_output --afile $4 --config $5"
make && ./GRAMM --seed $1 --verbose_level 0 --dfile $device_model_output --afile $4 --config $5



# Converting the mapped output dot file into png:
echo " "
echo " "
echo "---------------------Converting the ordered-mapped  positioned_dot_output.dot file into positioned_dot_output.png:---------------------"
echo "neato -Tpng positioned_dot_output.dot -o positioned_dot_output.png"
neato -Tpng positioned_dot_output.dot -o positioned_dot_output.png

# Converting the mapped output dot file into png:
echo " "
echo " "
echo "---------------------Converting the unordered-mapped  unpositioned_dot_output.dot file into unpositioned_dot_output.png:---------------------"
echo "neato -Tpng unpositioned_dot_output.dot -o unpositioned_dot_output.png"
dot -Tpng unpositioned_dot_output.dot -o unpositioned_dot_output.png