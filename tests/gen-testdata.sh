#!/usr/bin/env bash
set -euox pipefail

as_all=( \
	"tests/data/adds.s"
	"tests/data/subs.s"
	"tests/data/b.s"
	);
as_new="tests/data/b.s"

# create expected data
echo "creating ${as_new%.*}.txt"
filename=${as_new##*/}
gdb -nx -q -batch -x ./tests/gdbscript.txt --args ./emu-testgen ${filename%.*} | grep QQQ > ${as_new%.*}.txt

# create test assembler
echo "creating ${as_new%.*}.s"
./emu-testgen ${filename%.*}

for as in ${as_all[@]}; do
	# create test rawbin
	echo "creating ${as%.*}.bin"
	./tests/gen-rawbin.sh ${as} ${as%.*}.bin
done