test1:
	MOVZ X1, #1
	B test3
test2:
	MOVZ X2, #2
test3:
	MOVZ X3, #3
	BL sub
subdummy:
	MOVZ X4, #3
sub:
	MOVZ X4, #4
