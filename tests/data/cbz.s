.global bcond_test_enter
.global bcond_test
bcond_test:
bcond_test_enter:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
	B.EQ test_eq
test_eq: #Z=1
	addi x1, x1, 1
	B.NE test_ne
test_ne: #Z=0
	addi x1, x1, 1
	B.CS test_cs_hs
test_cs_hs: #C=1
	addi x1, x1, 1
	B.CC test_cc_lo
test_cc_lo #C=0
	addi x2, x2, 2
	B.MI test_mi
test_mi: #N=1
	addi x2, x2, 2
	B.PL test_pl
test_pl: #N=0
	addi x2, x2, 2
	B.VS test_vs
test_vs: #V=1
	addi x3, x3, 3
	B.VC test_vc
test_vc: #V=0
	addi x3, x3, 3
	B.HI test_hi
test_hi: #(C=1)and(Z=0)
	addi x3, x3, 3
	B.LS test_ls
test_ls: #(C=0)or(Z=1)
	addi x4, x4, 4
	B.GE test_ge
test_ge: #N=V
	addi x4, x4, 4
	B.LT test_lt
test_lt: #N!=V
	addi x4, x4, 4
	B.GT test_gt
test_gt:#(Z=0)and(N=V)
	addi x5, x5, 5 
	B.LE test_le
test_le: #(Z=1)or(N!=V)
	addi x5, x5, 5 
	B.AL test_al
test_al:#Any
	addi x5, x5, 5 
	B.NV test_nv
test_nv: #Any
	addi x5, x5, 5 
	movz x5, #1
	ldr  lr, [sp], #16    // pop LR
	ret
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
	movz x4, #0
	movz x5, #0
	ret
