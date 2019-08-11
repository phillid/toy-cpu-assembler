ldi $1, 2
ldi $2, 20
ldi $3, 0
loop:
	add $3, $3, $1
	subi $2, $2, 1
	bnz loop
