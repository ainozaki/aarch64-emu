import sys

class Log:
    def __init__(self, gdb_file, emu_file):
        self.f_gdb = open(gdb_file)
        self.f_emu = open(emu_file)
        self.loglist_gdb = [s.strip() for s in self.f_gdb.readlines()]
        self.loglist_emu = [s.strip() for s in self.f_emu.readlines()]
        self.len_gdb = len(self.loglist_gdb)
        self.len_emu = len(self.loglist_emu)
        self.idx_gdb = 0
        self.idx_emu = 0
        self.idx_inst = 0
        self.num_insts = int(self.loglist_emu[0], 10)
        self.pc = 0

    def compare_item(self, name):
        tmp1 = self.loglist_gdb[self.idx_gdb].split()
        tmp2 = self.loglist_emu[self.idx_emu].split()
        self.idx_gdb += 1
        self.idx_emu += 1
        assert(tmp1[0] == name)
        assert(tmp2[0] == name)
        
        if (name == "sp"):
            return
        if (name == "pc"):
            self.pc = int(tmp1[1], 16)
        if tmp1[1] != tmp2[1]:
            print("[{}][{}] gdb:{} vs emu:{}".format(self.idx_inst, name, format(int(tmp1[1], 16), "x"), format(int(tmp2[1], 16), "x")))
            exit()
        

    def compare_files(self):
        while (self.idx_gdb < self.len_gdb) and (self.idx_emu < self.len_emu):
            tmp1 = self.loglist_gdb[self.idx_gdb].split()
            tmp2 = self.loglist_emu[self.idx_emu].split()
            self.idx_gdb += 1
            self.idx_emu += 1
            
            # sync
            while tmp1[0] != "===":
                tmp1 = self.loglist_gdb[self.idx_gdb].split()
                self.idx_gdb += 1
            while tmp2[0] != "===":
                tmp2 = self.loglist_emu[self.idx_emu].split()
                self.idx_emu += 1
            
            # index
            assert(tmp1[1] == tmp2[1])

            # pc
            self.compare_item("pc")
            
            # sp
            self.compare_item("sp")
            
            # cpsr
            tmp1 = self.loglist_gdb[self.idx_gdb].split()
            self.idx_gdb += 1
            #tmp2 = loglist_emu[idx_emu].split()        
            #idx_emu += 1
            
            # general registers
            self.compare_item("x0")
            self.compare_item("x1")
            self.compare_item("x2")
            self.compare_item("x3")
            self.compare_item("x19")
            self.compare_item("x29")
            self.compare_item("x30")
            
            # disas
            tmp2 = self.loglist_emu[self.idx_emu]        
            self.idx_emu += 1
            print("[{}] {} {}".format(self.idx_inst, format(self.pc, "x"), tmp2))

            # loop up to `num_insts`
            self.idx_inst += 1
            if self.idx_inst == self.num_insts:
                break

        self.f_gdb.close()
        self.f_emu.close()
    
def main():
    """
    argv = sys.argv
    if len(argv) < 3:
        print("usage:", argv[0], "<gdb-file> <emu-file>")
        return
    gdb_path = argv[1]
    emu_path = argv[2]
    """
    gdb_path = "gdb-log.txt"
    emu_path = "emu-log.txt"
    log = Log(gdb_path, emu_path)
    log.compare_files()

if __name__ == "__main__":
    main()
