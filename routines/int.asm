.ORIG x1200
;Push R0 and R1 onto supervisor stack
;Assumes R6 is already initialized as supervisor stack pointer
ADD R6, R6 #-2
STW R0, R6, #0
ADD R6, R6, #-2
STW R1, R6, #0

;Begin execution of ISR
LEA R0, INCREMENT
LDW R0, R0, #0
LDW R1, R0, #0
ADD R1, R1, #1
STW R1, R0, #0

;Pop R0 and R1 from supervisor stack
LDW R1, R6, #0
ADD R6, R6, #2
LDW R0, R6, #0
ADD R6, R6, #2
RTI
INCREMENT .FILL x4000
.END