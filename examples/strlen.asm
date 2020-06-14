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
  RET

;strlen:
;  MOV $10, $0
;  LOD $11, $0
;  BRA strlen.end, !EQ $11
;strlen.loop:
;  ADD $10, $10, #1
;  LOD $11, $0
;
;  PSH $0
;  MOV $0, #65
;  CAL putc
;  POP $0
;
;  BRA strlen.loop, !NE $11
;strlen.end:
;  CMP $0, $1, $0
;  RET
