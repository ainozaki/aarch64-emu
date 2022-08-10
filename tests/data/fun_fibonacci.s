.global	fibonacci_test_enter
fibonacci_test:
	bl fibonacci
	movz x5, #1
	ret
fibonacci_test_enter:
	str  lr, [sp, #-64]! // push LR
	bl fibonacci
	movz x5, #1
	ldr  lr, [sp], #64    // pop LR
	ret
fibonacci:
.LFB0:
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	x19, [sp, 16]
	str	w0, [sp, 44]
	ldr	w0, [sp, 44]
	cmp	w0, 0
	bne	.L2
	mov	w0, 0
	b	.L3
.L2:
	ldr	w0, [sp, 44]
	cmp	w0, 1
	bne	.L4
	mov	w0, 1
	b	.L3
.L4:
	ldr	w0, [sp, 44]
	sub	w0, w0, #1
	bl	fibonacci
	mov	w19, w0
	ldr	w0, [sp, 44]
	sub	w0, w0, #2
	bl	fibonacci
	add	w0, w19, w0
.L3:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 48
	ret
