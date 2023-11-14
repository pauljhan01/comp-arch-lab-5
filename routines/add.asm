.ORIG x3000
LEA R0, Initial
LDW R0, R0, #0 ;R0 = x4000
ADD R1, R1, #1
STW R1, R0, #0 ;Mem[x4000] = 1

LEA R0, Begin
LDW R0, R0, #0 ;R0 = xC000
LEA R1, Repetition
LDW R1, R1, #0 ;R1 = #20

;Sum 20 bytes beginning at xC000
Repeat LDB R2, R0, #0 ;Load value at byte into R2
ADD R3, R3, R2 ;Add loaded value with previous N values in R3
ADD R0, R0, #1 ;Increment pointer by 1
ADD R1, R1, #-1 ;Decrease counter by 1
BRP Repeat ;Repeat if counter is still positive

LEA R0, Complete
LDW R0, R0, #0
STW R3, R0, #0 ;Mem[xC014] = Sum of the 20 bytes

LEA R0, Unaligned
LDW R0, R0, #0
STW R3, R0, #0 

Initial .FILL x4000
Begin .FILL xC000
Complete .FILL xC014
Protection .FILL x0000 ;For readability
Unaligned .FILL xC017
Repetition .FILL #20

.END