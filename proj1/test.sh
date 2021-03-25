 #!/usr/bin/env bash

NUM=16
PROC=$(echo 'l(16)/l(2)' | bc -l)
PROC=${PROC%.*}
PROC=$(( $PROC + 1 ))
NAME="pms"
SOURCE="$NAME.cpp"
NUMBERS_FILE="numbers"
BUILD_OPT=""
RUN_OPT=""

#merlin 
# BUILD_OPT="--prefix /usr/local/share/OpenMPI"
# RUN_OPT="--prefix /usr/local/share/OpenMPI" ### num

mpic++ ${BUILD_OPT} -o "$NAME" "$SOURCE"
CODE="$?"

if [[ "$CODE" -ne 0 ]]; then
  exit "$CODE"
fi

############# GENERATE RANDOM NUMBERS #############
dd if=/dev/random bs=1 count=$NUM of=$NUMBERS_FILE

############ RUN ############
mpirun ${RUN_OPT} -np $PROC ./$NAME

############ DELETE FILES ############
#rm -f $NAME $NUMBERS_FILE
