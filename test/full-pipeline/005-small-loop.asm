ldi $1, 2
ldi $2, 100
ldi $3, 0
loop:
	add $3, $3, $1
	bnz loop
