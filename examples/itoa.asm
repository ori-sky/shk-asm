itoa:
  MOV $20, $1
  MOV $10, $0
itoa.loop:
  MOD $11, $10, #10
  ADD $11, #48, $11
  STO $1, $11
  ADD $1, $1, #1
  DIV $10, $10, #10
  BRA itoa.loop, !NE $10
itoa.end:
  STO $1, #0
  MOV $0, $20
  CAL strrev
  RET
