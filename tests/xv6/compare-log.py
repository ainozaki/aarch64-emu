import sys

def compare_files(gdb_path, emu_path):
    f_gdb = open(gdb_path)
    f_emu = open(emu_path)

    loglist_gdb = [s.strip() for s in f_gdb.readlines()]
    loglist_emu = [s.strip() for s in f_emu.readlines()]
    len_gdb = len(loglist_gdb)
    len_emu = len(loglist_emu)
    idx_gdb = 0
    idx_emu = 0
    inst_idx = 0

    # Number of insts
    num_insts = int(loglist_emu[0], 10)

    while (idx_gdb < len_gdb) and (idx_emu < len_emu):
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()
        idx_emu += 1
        
        # sync
        while tmp1[0] != "===":
            tmp1 = loglist_gdb[idx_gdb].split()
            idx_gdb += 1
        while tmp2[0] != "===":
            tmp2 = loglist_emu[idx_emu].split()
            idx_emu += 1
        
        # index
        assert(tmp1[1] == tmp2[1])

        # pc
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()        
        idx_emu += 1
        assert(tmp1[0] == "pc")
        assert(tmp2[0] == "pc")
        if tmp1[1] != tmp2[1]:
            print("pc ng", format(int(tmp1[1], 16), "x"), "vs", format(int(tmp2[1], 16), "x"))
            return 1
        pc = int(tmp1[1], 16)
        
        # sp
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()        
        idx_emu += 1
        
        # cpsr
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        #tmp2 = loglist_emu[idx_emu].split()        
        #idx_emu += 1
        
        # w0
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()        
        idx_emu += 1
        assert(tmp1[0] == "w0")
        assert(tmp2[0] == "w0")
        if tmp1[1] != tmp2[1]:
            print("failed: w0", format(int(tmp1[1], 16), "x"), "vs", format(int(tmp2[1], 16), "x"))
            return 1
        
        # w1
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()        
        idx_emu += 1
        assert(tmp1[0] == "w1")
        assert(tmp2[0] == "w1")
        if tmp1[1] != tmp2[1]:
            print("w1 ng", format(int(tmp1[1], 16), "x"), "vs", format(int(tmp2[1], 16), "x"))
            return 1
        
        # w2
        tmp1 = loglist_gdb[idx_gdb].split()
        idx_gdb += 1
        tmp2 = loglist_emu[idx_emu].split()        
        idx_emu += 1
        assert(tmp1[0] == "w2")
        assert(tmp2[0] == "w2")
        if tmp1[1] != tmp2[1]:
            print("w2 ng", format(int(tmp1[1], 16), "x"), "vs", format(int(tmp2[1], 16), "x"))
            return 1
       
        # disas
        tmp2 = loglist_emu[idx_emu]        
        idx_emu += 1
        print("[", inst_idx, "]", format(pc, "x"), tmp2)

        # loop up to `num_insts`
        inst_idx += 1
        if inst_idx == num_insts:
            break

    f_gdb.close()
    f_emu.close()
    
def main():
    argv = sys.argv
    if len(argv) < 3:
        print("usage:", argv[0], "<gdb-file> <emu-file>")
        return
    gdb_path = argv[1]
    emu_path = argv[2]
    print("compare", gdb_path, "and", emu_path)
    compare_files(gdb_path, emu_path)

if __name__ == "__main__":
    main()