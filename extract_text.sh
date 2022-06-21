#!/bin/bash

SUF=aarch64-linux-gnu-
${SUF}as $1 -o tmp.o
RET=$(${SUF}objdump -h tmp.o | awk '/\.text/ { printf("0x%s 0x%s\n", $3, $6)}')
VALS=(${RET})
SIZE=$((${VALS[0]}))
OFFSET=$((${VALS[1]}))
dd if=tmp.o of=out.bin ibs=1 skip=${OFFSET} count=${SIZE} status=none
