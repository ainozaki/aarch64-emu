init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
test_as:
	movz x1, #1
	b test3_as
	ret
test2_as:
	movz x2, #2
	ret
test3_as:
	movz x3, #3
	ret
