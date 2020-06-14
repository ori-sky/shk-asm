MOV $0, #0
MOV $1, #1

MOV $2, #0
LOD $3, fib_digit

loop: b: c:
d:
  ADD $4, $0, $1
  MOV $0, $1
  MOV $1, $4
  ADD $2, $2, #1
  CMP $5, $2, $3
BRA loop, !LT $5

e: DBG
DIE

fib_digit: DAT #10
