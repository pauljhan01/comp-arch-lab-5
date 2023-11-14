.ORIG x0200
.FILL x0000 ;Empty
.FILL x1200 ;ISR, vector x01
.FILL x1600 ;Protection exception handler, vector x02
.FILL x1A00 ;Unaligned access exception handler, vector x03
.FILL x1C00 ;Unknown opcode exception handler, vector x04
.END