strrev:
  PSH $0
  CAL strlen
  POP $10
  CMP $11, $0, #2
  BRA strrev.end, !LT $11
  MOV $11, #0
  DIV $12, $0, #2
strrev.loop:
  ADD $13, $10, $11
  LOD $14, $13
  ADD $15, $10, $0
  CMP $15, $15, #1
  CMP $15, $15, $11
  LOD $16, $15

  STO $14, $15
  STO $16, $13

  ADD $11, $11, #1
  CMP $14, $11, $12
  BRA strrev.loop, !LT $14
strrev.end:
  RET
