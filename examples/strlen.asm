strlen:
  MOV $10, #0
  LOD $11, $0
  BRA strlen.end, !EQ $11
strlen.loop:
  ADD $10, $10, #1
  ADD $0, $0, #1
  LOD $11, $0
  BRA strlen.loop, !NE $11
strlen.end:
  MOV $0, $10
  RET
