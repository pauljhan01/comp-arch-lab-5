.ORIG x1200
;Push R0 R1 R2 R3 onto supervisor stack
;Assumes R6 is already initialized as supervisor stack pointer
ADD R6, R6 #-2
STW R0, R6, #0
ADD R6, R6, #-2
STW R1, R6, #0
ADD R6, R6, #-2
STW R2, R6, #0
ADD R6, R6, #-2
STW R3, R6, #0


;Begin execution of ISR
LEA R0, PAGE_TABLE
LDW R0, R0, #0 ; Load address of page table
LEA R1, NUM_PTE
LDW R1, R1, #0 ;Load counter of PTE to traverse
LEA R2, MASK 
LDW R2, R2, #0 ;Load mask to clear reference bit

REPEAT LDW R3, R0, #0 ;Load PTE
AND R3, R3, R2 ;Clear reference bit
STW R3, R0, #0
ADD R0, R0, #2 ;Increment pointer to next PTE
ADD R1, R1, #-1 ;Decrement counter
BRP REPEAT
BRNZ DONE



;Pop R0, R1, R2 R3 from supervisor stack
DONE LDW R3, R6, #0
ADD R6, R6, #2
LDW R2, R6, #0
ADD R6, R6, #2
LDW R1, R6, #0
ADD R6, R6, #2
LDW R0, R6, #0
ADD R6, R6, #2
RTI
PAGE_TABLE .FILL x1000
NUM_PTE .FILL #128
MASK .FILL xFFFE
.END