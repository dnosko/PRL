#!/usr/bin/env bash


NUM=16
NAME="pms"
SOURCE="$NAME.c"
NUMBERS_FILE="numbers"
BUILD_OPT=""
RUN_OPT=""

#merlin 
# BUILD_OPT="--prefix /usr/local/share/OpenMPI"
# RUN_OPT="--prefix /usr/local/share/OpenMPI" ### num

mpicc ${BUILD_OPT} -o "$NAME" "$SOURCE"
CODE="$?"

if [[ "$CODE" -ne 0 ]]; then
  exit "$CODE"
fi

############# GENERATE RANDOM NUMBERS #############

dd if=/dev/random bs=1 count=$NUM of=$NUMBERS_FILE


############ RUN ############

mpirun ${RUN_OPT} -np $NUM ./$NAME

############ DELETE FILES ############
rm -f $NAME $NUMBERS_FILE
