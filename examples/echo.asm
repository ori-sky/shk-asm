main:
main.loop:
  CAL getc
  CMP $20, $0, #10
  CAL putc
  BRA main.loop, !NE $20
main.end:
  RET
