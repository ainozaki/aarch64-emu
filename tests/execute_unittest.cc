#include <gtest/gtest.h>

#include "system.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "log.h"

TEST(DataProcessingImm, ADDS) {
  std::string qqq, insttype;
  uint64_t w0, w1;
  uint32_t inst;
  char c;
  int cpsr[4];

  core::System sys("tests/data/adds.bin", /*initaddr=*/0);
  sys.Init();

  std::ifstream f("tests/data/adds.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }

  std::string s;

  while (getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    // inst
    std::istringstream ssinst(s);
    ssinst >> qqq >> qqq >> qqq >> insttype;
    LOG_DEBUG("%s", s.c_str());

    LOG_DEBUG("[expected]\n");
    // w1, w2, w3
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssw(s);
    ssw >> qqq >> std::hex >> w0 >> w1;
    LOG_DEBUG("w0=0x%016lx, w1=0x%016lx\n", w0, w1);

    // flag
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssflag(s);
    ssflag >> qqq;
    for (int i = 0; i < 4; i++) {
      ssflag >> c;
      cpsr[i] = (c != '-');
    }
    LOG_DEBUG("n=%d z=%d c=%d v=%d\n", cpsr[0], cpsr[1], cpsr[2], cpsr[3]);

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.decode_start(inst);

    if (insttype == "adds") {
      EXPECT_EQ(sys.cpu().xregs[0], w0);
      EXPECT_EQ(sys.cpu().xregs[1], w1);
      EXPECT_EQ(cpsr[0], sys.cpu().cpsr.N);
      EXPECT_EQ(cpsr[1], sys.cpu().cpsr.Z);
      EXPECT_EQ(cpsr[2], sys.cpu().cpsr.C);
      EXPECT_EQ(cpsr[3], sys.cpu().cpsr.V);
    }
  }
}

/*
TEST(DataProcessingImm, SUBS) {
  std::string qqq;
  uint64_t w0, ans, imm;
  uint32_t inst;
  char c;
  int cpsr[4];

  core::System sys("tests/data/subs.bin", //initaddr=/0);
  sys.Init();

  std::ifstream f("tests/data/subs.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }

  std::string s;
  while (getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    LOG_DEBUG("[expected]\n");
    // imm
    std::istringstream ssimm(s);
    ssimm >> qqq >> std::hex >> imm;
    LOG_DEBUG("imm = 0x%016lx\n", imm);

    // input
    getline(f, s);
    std::istringstream ssin(s);
    ssin >> qqq >> std::hex >> w0;
    LOG_DEBUG("w0  = 0x%016lx\n", w0);

    // ans
    getline(f, s);
    std::istringstream ssans(s);
    ssans >> qqq >> std::hex >> ans;
    LOG_DEBUG("ans = 0x%016lx\n", ans);

    // flag
    getline(f, s);
    std::istringstream ssflag(s);
    ssflag >> qqq;
    for (int i = 0; i < 4; i++) {
      ssflag >> c;
      cpsr[i] = (c != '-');
    }
    LOG_DEBUG("n=%d z=%d c=%d v=%d\n", cpsr[0], cpsr[1], cpsr[2], cpsr[3]);

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.cpu().xregs[0] = w0;
    sys.decode_start(inst);
    sys.cpu().increment_pc();

    EXPECT_EQ(sys.cpu().xregs[1], ans);
    EXPECT_EQ(cpsr[0], sys.cpu().cpsr.N);
    EXPECT_EQ(cpsr[1], sys.cpu().cpsr.Z);
    EXPECT_EQ(cpsr[2], sys.cpu().cpsr.C);
    EXPECT_EQ(cpsr[3], sys.cpu().cpsr.V);
  }
}
*/

TEST(Branch, b) {
  std::string qqq;
  uint64_t w1, w2, w3, w4;
  uint32_t inst;

  core::System sys("tests/data/b.bin", /*initaddr=*/0x10);
  sys.Init();

  std::ifstream f("tests/data/b.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }

  std::string s;

  while (std::getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    // inst
    LOG_DEBUG("%s", s.c_str());

    LOG_DEBUG("[expected]\n");
    // w1, w2, w3
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssw(s);
    ssw >> qqq >> std::hex >> w1 >> w2 >> w3 >> w4;
    LOG_DEBUG("w1=0x%016lx, w2=0x%016lx, w3=0x%016lx, w4=0x%16lx\n", w1, w2, w3,
              w4);

    // flag
    if (!std::getline(f, s)) {
      break;
    }

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.decode_start(inst);
  }
  EXPECT_EQ(sys.cpu().xregs[1], w1);
  EXPECT_EQ(sys.cpu().xregs[2], w2);
  EXPECT_EQ(sys.cpu().xregs[3], w3);
  EXPECT_EQ(sys.cpu().xregs[4], w4);
}

TEST(Branch, ret) {
  std::string qqq;
  uint64_t w1, w2, w3, w4;
  uint32_t inst;

  core::System sys("tests/data/ret.bin", /*initaddr=*/0x8);
  sys.Init();

  std::ifstream f("tests/data/ret.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }

  std::string s;

  while (std::getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    // inst
    LOG_DEBUG("%s", s.c_str());

    LOG_DEBUG("[expected]\n");
    // w1, w2, w3
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssw(s);
    ssw >> qqq >> std::hex >> w1 >> w2 >> w3 >> w4;
    LOG_DEBUG("w1=0x%016lx, w2=0x%016lx, w3=0x%016lx, w4=0x%016lx\n", w1, w2,
              w3, w4);

    // flag
    if (!std::getline(f, s)) {
      break;
    }

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.decode_start(inst);
  }
  EXPECT_EQ(sys.cpu().xregs[1], w1);
  EXPECT_EQ(sys.cpu().xregs[2], w2);
  EXPECT_EQ(sys.cpu().xregs[3], w3);
  EXPECT_EQ(sys.cpu().xregs[4], w4);
}

TEST(DataProcessingImm, MoveWide) {
  core::System sys("", 0);
  sys.Init();

  sys.decode_start(0x91404042); /* ADD X2, X2, 0x10, LSL #12 */
  EXPECT_EQ(0x10000, sys.cpu().xregs[2]);

  sys.decode_start(0x72800002); /* MOVK W2, #0 */
  EXPECT_EQ(0x10000, sys.cpu().xregs[2]);

  sys.decode_start(0x52800002); /* MOVZ W2, #0 */
  EXPECT_EQ(0, sys.cpu().xregs[2]);
}

TEST(DataProcessingImm, Logical) {
  core::System sys("", 0);
  sys.Init();

  sys.decode_start(0xd2802201); /* MOV X1, #0x110 */
  EXPECT_EQ(0x110, sys.cpu().xregs[1]);

  sys.decode_start(0x92780023); /* AND X3, X1, #0x100 */
  EXPECT_EQ(0x100, sys.cpu().xregs[3]);

  sys.decode_start(0xb2780024); /* ORR X4, X1, #0x100 */
  EXPECT_EQ(0x110, sys.cpu().xregs[4]);

  sys.decode_start(0xd2780025); /* EOR X5, X1, #0x100 */
  EXPECT_EQ(0x010, sys.cpu().xregs[5]);

  sys.decode_start(0xf2400026); /* ANDS X6, X1, #0x1 */
  EXPECT_EQ(0x0, sys.cpu().xregs[6]);
  EXPECT_EQ(0, sys.cpu().cpsr.N);
  EXPECT_EQ(1, sys.cpu().cpsr.Z);
}

TEST(DataProcessingImm, Bitfield) {
  core::System sys("", 0);
  sys.Init();

  sys.decode_start(0xd2801e22); /* MOV X2, #0x00f1 */
  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  EXPECT_EQ(0xf1, sys.cpu().xregs[2]);
  EXPECT_EQ(0x1010, sys.cpu().xregs[3]);

  sys.decode_start(0x93781c43); /* SBFI X3, X2, #8, #8 */
  EXPECT_EQ(0xfffffffffffff100, sys.cpu().xregs[3]);

  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  sys.decode_start(0xb3781c43); /* BFI X3, X2, #8, #8 */
  EXPECT_EQ(0xf110, sys.cpu().xregs[3]);

  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  sys.decode_start(0xd3781c43); /* BFI X3, X2, #8, #8 */
  EXPECT_EQ(0xf100, sys.cpu().xregs[3]);
}

/*
TEST(Branch, BranchImm) {
  core::System sys("tests/test_branch.bin", 0);
  sys.Init();
  sys.execute_loop();

  // B, BL
  EXPECT_EQ(0x1, sys.cpu().xregs[1]);
  EXPECT_NE(0x2, sys.cpu().xregs[2]);
  EXPECT_EQ(0x3, sys.cpu().xregs[3]);
  EXPECT_EQ(0x4, sys.cpu().xregs[4]);
  // CBZ, CBNZ
  EXPECT_NE(0x5, sys.cpu().xregs[5]);
  EXPECT_EQ(0x6, sys.cpu().xregs[6]);
}
*/

TEST(Func, sum) {
  std::string qqq;
  uint64_t w0, w1;
  uint32_t inst;
  std::string s;

  core::System sys("tests/data/fun_sum.bin", /*initaddr=*/0x0);
  sys.Init();
  std::ifstream f("tests/data/fun_sum.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }
  sys.cpu().xregs[0] = 10;

  while (std::getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    // inst
    LOG_DEBUG("%s", s.c_str());

    LOG_DEBUG("[expected]\n");
    // w0, w1
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssw(s);
    ssw >> qqq >> std::hex >> w0 >> w1;
    LOG_DEBUG("w0=0x%016lx, w1=0x%016lx\n", w0, w1);

    // flag
    if (!std::getline(f, s)) {
      break;
    }

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.decode_start(inst);
    LOG_DEBUG("inst: 0x%x\n", inst);
    LOG_DEBUG("w0=0x%016lx, w1=0x%016lx\n", sys.cpu().xregs[0],
              sys.cpu().xregs[1]);
    LOG_DEBUG("\tpc=0x%016lx, sp=0x%016lx\n", sys.cpu().pc,
              sys.cpu().xregs[31]);
    LOG_DEBUG("\t24(SP):0x%lx\n", sys.mem().read(core::mem::MemAccess::Size64,
                                                 sys.cpu().xregs[31] + 24));
    LOG_DEBUG("\t28(SP):0x%lx\n", sys.mem().read(core::mem::MemAccess::Size64,
                                                 sys.cpu().xregs[31] + 28));
    EXPECT_EQ(sys.cpu().xregs[0], w0);
    EXPECT_EQ(sys.cpu().xregs[1], w1);
  }
}

TEST(Func, fibonacci) {
  std::string qqq;
  uint64_t w0, w1, w19;
  uint32_t inst;
  std::string s;
  int index = 0;

  core::System sys("tests/data/fun_fibonacci.bin", /*initaddr=*/0x0);
  sys.Init();
  std::ifstream f("tests/data/fun_fibonacci.txt");
  if (f.fail()) {
    fprintf(stderr, "ifstream\n");
    return;
  }
  sys.cpu().xregs[0] = 15;

  while (std::getline(f, s)) {
    LOG_DEBUG("-----------------------\n");
    // inst
    LOG_DEBUG("%s", s.c_str());

    LOG_DEBUG("\n[expected]\n");
    // w0, w1
    if (!std::getline(f, s)) {
      break;
    }
    std::istringstream ssw(s);
    ssw >> qqq >> std::hex >> w0 >> w1 >> w19;
    LOG_DEBUG("\tw0=0x%016lx, w1=0x%016lx, w19=0x%016lx\n", w0, w1, w19);

    // flag
    if (!std::getline(f, s)) {
      break;
    }

    // execute
    LOG_DEBUG("[actual]\n");
    inst = sys.fetch();
    sys.decode_start(inst);
    LOG_DEBUG("\tinst: 0x%x\n", inst);
    LOG_DEBUG("\tw0=0x%016lx, w1=0x%016lx, w19=0x%016lx\n", sys.cpu().xregs[0],
              sys.cpu().xregs[1], sys.cpu().xregs[19]);
    LOG_DEBUG("\tw29=0x%016lx, w30=0x%016lx\n", sys.cpu().xregs[29],
              sys.cpu().xregs[30]);
    LOG_DEBUG("\tpc=0x%016lx, sp=0x%016lx\n", sys.cpu().pc,
              sys.cpu().xregs[31]);
    // LOG_DEBUG("\t28(SP):0x%lx\n", sys.mem().read(3, sys.cpu().xregs[31]
    // + 28)); EXPECT_EQ(sys.cpu().xregs[0], w0); EXPECT_EQ(sys.cpu().xregs[1],
    // w1);
    EXPECT_EQ(sys.cpu().xregs[0], w0);
    EXPECT_EQ(sys.cpu().xregs[1], w1);
    index++;
  }
}
