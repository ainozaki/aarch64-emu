.global test_ldxr
test_ldxr:
	mov x0, #0
	ldxr w0, [x19]
	stxr w2, w1, [x19]
	mov x0, #1
	ret
