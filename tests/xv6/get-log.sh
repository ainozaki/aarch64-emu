gdb(){
	scp sslab-remote:~/source/xv6-aarch64/gdb-log.txt ./
}

emu(){
	scp mp:~/aarch64-emu/emu-log.txt ./
}

if [ $# -eq 0 ]; then
	gdb
	emu
fi
eval "$1"
