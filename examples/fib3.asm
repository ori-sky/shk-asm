fib:
  CMP $10, $0, #1
  BRA fib.end, !LE $10
fib.main:
  MOV $0, $10
  CMP $10, $0, #1
  PSH $10
  CAL fib
  MOV $10, $0
  POP $0
  PSH $10
  CAL fib
  POP $10
  ADD $0, $0, $10
fib.end:
  RET

fib_str: DAT #0, #0, #0, #0, #0, #0

in_str:  DAT #105, #110, #32, #61, #32, #0

out_str: DAT #111, #117, #116, #32, #61, #32, #0

main:
main.loop:
  MOV $0, in_str
  CAL puts

  CAL getc
  CMP $0, $0, #48
  CAL fib

  LOD $100, #1:#0   ; skip newline

  MOV $1, fib_str
  CAL itoa

  MOV $0, out_str
  CAL puts
  MOV $0, fib_str
  CAL puts

  MOV $0, #10
  CAL putc

  ;CAL getc   ; skip newline
  BRA main.loop
main.end:
  RET
