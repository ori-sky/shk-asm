LOD $1, fib_digit
CAL fib
STO #1:#0, $0            ; stdout
DIE

; in:  fib digit
; out: result
fib:
  MOV $0,    #0          ; term 1
  MOV $0x10, #1          ; term 2
  MOV $0x11, #0          ; loop counter
.loop:
  STO #1:#0, $0          ; stdout
  ADD $0x12, $0, $0x10
  MOV $0,    $0x10
  MOV $0x10, $0x12
  ADD $0x11, $0x11, #1
  CMP $0x13, $0x11, $1
  BRA .loop, !LT $0x13
.end:
  RET

fib_digit: DAT #10
