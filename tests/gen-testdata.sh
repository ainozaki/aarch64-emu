#!/bin/bash

TEST_AS=( \
	"tests/data/adds.s"
	);

for as in ${TEST_AS[@]}; do
	echo "creating ${as%.*}.bin"
	./tests/gen-rawbin.sh ${as} ${as%.*}.bin
	echo "creating ${as%.*}.txt"
	gdb -nx -q -batch -x ./tests/gdbscript.txt ./emu-testgen | grep QQQ > ${as%.*}.txt
done
