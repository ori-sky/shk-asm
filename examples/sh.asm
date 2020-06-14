main:
  CAL getc
main.loop:
  CAL putc
  CAL getc
  CMP $20, $0, #113        ; lowercase q
  BRA main.loop, !NE $20
main.end:
  RET
