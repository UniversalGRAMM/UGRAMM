#!/bin/bash

#Default architecture set to RIKEN in this helper script
#Seed is default to 0
# $1 = Seed
# $2 = NR = 8
# $3 = NC = 8
# $4 = Application graph
# $5 = UGRAMM config JSON file

# Check if the first argument is --help or no arguments provided at all 
if [ "$1" == "--help" ] || [ -z "$1" ]; then
  echo "This helper script will generate a specific CGRA device model file based on user input; Compile, and run UGRAMM with the generated device model file as input. It will also convert the output of UGRAMM into PNG format for easier debugging and visualization."
  echo "Usage: ./run_ugramm.sh [SEED] [NR] [NC] [APPLICATION_GRAPH] [CONFIG_FILE]"
  echo "Example: ./run_ugramm.sh 15 8 8 Kernels/Conv_Balance/conv_nounroll_Balance_Specific.dot config.json"
  exit 0
fi

# Assign default seed if not provided
SEED=${1:-0}
NR=${2}
NC=${3}
Arch=RIKEN #Harcoded, can be changed to ADRES
afile=${4}
cfile=${5}
verbose_level=0 #Hardcoded, can be run with 0 -> info : 1-> debug : 2-> trace
#drc_verbose_level=1 #Optional, can be run with 0 -> err [default] : 1-> warn : 2-> info 

# To disable DRC, add the following flag "--drc_disable"
# To set DRC verbose level, add the following flag "--drc_verbose_level [vebosity level]"

# Generating device model:
echo "---------------------Generating device model using external script:---------------------"
echo "cd scripts && ./device_model_gen.py -NR $NR -NC $NC -Arch $Arch && cd .."
python_version=python3	#replace with python if that's the default path
cd scripts && $python_version device_model_gen.py -NR $NR -NC $NC -Arch $Arch && cd ..

# Executes UGRAMM and producing mapping result in mapping_output.dot
echo " "
echo " "
echo "---------------------Executing UGRAMM and producing mapping result in ordered_dot_output.dot & unpositioned_dot_output---------------------"
dfile="scripts/riken_${NR}_${NC}.dot"
echo "make && ./UGRAMM --drc_disable --seed $SEED --verbose_level $verbose_level --dfile $dfile --afile $afile --config $cfile" 
make && ./UGRAMM --drc_disable --seed $1 --verbose_level $verbose_level --dfile $dfile --afile $afile --config $cfile



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
