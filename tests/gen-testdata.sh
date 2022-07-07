#!/usr/bin/env bash
set -euox pipefail

TEST_AS=( \
	"tests/data/adds.s"
#	"tests/data/subs.s"
	);

for as in ${TEST_AS[@]}; do
	# create expected data
	echo "creating ${as%.*}.txt"
	filename=${as##*/}
	gdb -nx -q -batch -x ./tests/gdbscript.txt --args ./emu-testgen ${filename%.*} | grep QQQ > ${as%.*}.txt

	# create test assembler
	echo "creating ${as%.*}.s"
	./emu-testgen ${filename%.*}

	# create test rawbin
	echo "creating ${as%.*}.bin"
	./tests/gen-rawbin.sh ${as} ${as%.*}.bin
done
