.global test_csel
test_csel:
	mov x1, #0x10
	mov x2, #0x20
	mov x3, #0x20
	csel	x4, x1, x2, eq
	csel	x5, x1, x2, ne
	csel	x6, x3, x2, eq
	csel	x7, x3, x2, ne
	ret
