.global	sum_test_enter
sum_test:
	bl testfn_sum
	movz x5, #1
	ret
sum_test_enter:
	str  lr, [sp, #-32]! // push LR
	bl testfn_sum
	movz x5, #1
	ldr  lr, [sp], #32    // pop LR
	ret
testfn_sum:
	sub	sp, sp, #32
	str	w0, [sp, 12]
	str	wzr, [sp, 24]
	ldr	w0, [sp, 12]
	str	w0, [sp, 28]
	b	.L2
.L3:
	ldr	w1, [sp, 24]
	ldr	w0, [sp, 28]
	add	w0, w1, w0
	str	w0, [sp, 24]
	ldr	w0, [sp, 28]
	sub	w0, w0, #1
	str	w0, [sp, 28]
.L2:
	ldr	w0, [sp, 28]
	cmp	w0, 0
	bgt	.L3
	ldr	w0, [sp, 24]
	add	sp, sp, 32
	ret
