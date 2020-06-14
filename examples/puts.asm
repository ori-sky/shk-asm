puts:
puts.loop:
  LOD $10, $0
  CMP $11, $10, #0
  BRA puts.end, !EQ $11
  STO #1:#0, $10
  ADD $0, $0, #1
  BRA puts.loop
puts.end:
  RET
