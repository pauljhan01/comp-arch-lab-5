/*
    Name 1: Paul Han
    UTEID 1: pjh2235
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N - Lab 5                                           */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         pagetable    page table in LC-3b machine language   */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    LD_VECTOR,
    LD_PSR,
    LD_SP,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    GATE_VECTOR,
    GATE_PSR,
    GATE_SP,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    SW_SSP,
    SW_USP,
    SP_MUX,
    CLR_PSR,
/* MODIFY: you have to add all your new control signals */
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetLD_VECTOR(int *x)     { return(x[LD_VECTOR]);}
int GetLD_PSR(int *x)        { return(x[LD_PSR]);}
int GetLD_SP(int *x)         { return(x[LD_SP]);}
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetGATE_VECTOR(int *x)   { return(x[GATE_VECTOR]);}
int GetGATE_PSR(int *x)      { return(x[GATE_PSR]);}
int GetGATE_SP(int *x)       { return(x[GATE_SP]);}
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }
int GetSW_SSP(int *x)        { return(x[SW_SSP]);}
int GetSW_USP(int *x)        { return(x[SW_USP]);}
int GetSP_MUX(int *x)        { return(x[SP_MUX]);}
int GetCLR_PSR(int *x)       { return(x[CLR_PSR]);}
/* MODIFY: you can add more Get functions for your new control signals */

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x2000 /* 32 frames */ 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 

/* For lab 4 */
int INTV; /* Interrupt vector register */
int EXCV; /* Exception vector register */
int SSP; /* Initial value of system stack pointer */
int USP; /* Initial value of user stack pointer */
int PSR; /* Processor Status Register */
/* MODIFY: you should add here any other registers you need to implement interrupts and exceptions */

/* For lab 5 */
int PTBR; /* This is initialized when we load the page table */
int VA;   /* Temporary VA register */
/* MODIFY: you should add here any other registers you need to implement virtual memory */

} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/* For lab 5 */
#define PAGE_NUM_BITS 9
#define PTE_PFN_MASK 0x3E00
#define PTE_VALID_MASK 0x0004
#define PAGE_OFFSET_MASK 0x1FF

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                
    if(CYCLE_COUNT == 300){
        NEXT_LATCHES.INTV = 0x01;
    }
    eval_micro_sequencer();   
    cycle_memory();
    eval_bus_drivers();
    drive_bus();
    latch_datapath_values();

    CURRENT_LATCHES = NEXT_LATCHES;

    CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%0.4x\n", BUS);
    printf("MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%0.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename, int is_virtual_base) {                   
    FILE * prog;
    int ii, word, program_base, pte, virtual_pc;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    if (is_virtual_base) {
      if (CURRENT_LATCHES.PTBR == 0) {
	printf("Error: Page table base not loaded %s\n", program_filename);
	exit(-1);
      }

      /* convert virtual_base to physical_base */
      virtual_pc = program_base << 1;
      pte = (MEMORY[(CURRENT_LATCHES.PTBR + (((program_base << 1) >> PAGE_NUM_BITS) << 1)) >> 1][1] << 8) | 
	     MEMORY[(CURRENT_LATCHES.PTBR + (((program_base << 1) >> PAGE_NUM_BITS) << 1)) >> 1][0];

      printf("virtual base of program: %04x\npte: %04x\n", program_base << 1, pte);
		if ((pte & PTE_VALID_MASK) == PTE_VALID_MASK) {
	      program_base = (pte & PTE_PFN_MASK) | ((program_base << 1) & PAGE_OFFSET_MASK);
   	   printf("physical base of program: %x\n\n", program_base);
	      program_base = program_base >> 1; 
		} else {
   	   printf("attempting to load a program into an invalid (non-resident) page\n\n");
			exit(-1);
		}
    }
    else {
      /* is page table */
     CURRENT_LATCHES.PTBR = program_base << 1;
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0 && is_virtual_base) 
      CURRENT_LATCHES.PC = virtual_pc;

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine         */
/*                                                             */
/***************************************************************/
void initialize(char *argv[], int num_prog_files) { 
    int i;
    init_control_store(argv[1]);

    init_memory();
    load_program(argv[2],0);
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(argv[i + 3],1);
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);
    CURRENT_LATCHES.SSP = 0x3000; /* Initial value of system stack pointer */

/* MODIFY: you can add more initialization code HERE */

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 4) {
	printf("Error: usage: %s <micro_code_file> <page table file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv, argc - 3);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* Do not modify the above code, except for the places indicated 
   with a "MODIFY:" comment.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/
int interrupts = FALSE;
int exceptions = FALSE;
int exception_or_interrupt_skip = FALSE;
#define READ 0
#define WRITE 1
int load_signals[7] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
void reset_load_signals();
enum load_signal_name{
    ldmar,
    ldmdr,
    ldir,
    ldben,
    ldreg,
    ldcc,
    ldpc
} load_signal_name;
void reset_load_signals(){
    for(int i = 0; i < 7; i++){
        load_signals[i] = FALSE;
    }
}
int sign_extend(int immediate, int starting_bit);
int sign_extend(int immediate, int starting_bit){
    int mask = 1;
    mask = mask << starting_bit - 1;
    if(((immediate & mask) >> (starting_bit - 1)) == 1){
        for(int i = 0; i < 32; i++){
            immediate |= mask;
            mask = mask << 1;
        }
    }
    return immediate;
}
void copy_microinstruction();
void copy_microinstruction(){
    for(int i = 0; i < CONTROL_STORE_BITS; i++){
        NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER][i];
    }
}
  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */
void eval_micro_sequencer() {
    //IRD[0]
    if(GetIRD(CURRENT_LATCHES.MICROINSTRUCTION)==0){
        switch(GetCOND(CURRENT_LATCHES.MICROINSTRUCTION)){
            //COND[0], COND[1]
            case 0b01:{
                if(CURRENT_LATCHES.READY == TRUE){
                    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION) | 0x2;
                    copy_microinstruction();
                }else{
                    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
                    copy_microinstruction();
                }
                break;
            }
            case 0b10:{
                if(CURRENT_LATCHES.BEN == TRUE){
                NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION) | 0x4; 
                copy_microinstruction();
                }
                else{
                    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
                    copy_microinstruction();
                }
                break;
            }
            case 0b11:{
                if((CURRENT_LATCHES.IR & 0x0800) == 0x0800){
                    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION) | 0x1;
                    copy_microinstruction();
                }
                else{
                    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
                    copy_microinstruction();
                }
                break;
            }
            default:{ 
                NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
                copy_microinstruction();
                break;
            }
        }
    }
    else{
        NEXT_LATCHES.STATE_NUMBER = (CURRENT_LATCHES.IR & 0x0000F000) >> 12;
        copy_microinstruction();
    }
}

  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */
void cycle_memory() {
    //track current memory cycle
    static int mem_cycle = 0;
    if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == TRUE && mem_cycle == 4){
        mem_cycle = 0;
        NEXT_LATCHES.READY = FALSE;
        load_signals[ldmdr] = TRUE;
    }
    else if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == TRUE){
        if(mem_cycle == 3){
            NEXT_LATCHES.READY = TRUE;
            int read_write = GetR_W(CURRENT_LATCHES.MICROINSTRUCTION);
            switch(read_write){
                case READ:{
                    if((CURRENT_LATCHES.MAR & 0x1) == 1){
                        
                        NEXT_LATCHES.MDR = Low16bits((MEMORY[(CURRENT_LATCHES.MAR & 0xFFFE) >> 1][1] << 8));
                        NEXT_LATCHES.MDR = NEXT_LATCHES.MDR + (MEMORY[(CURRENT_LATCHES.MAR & 0xFFFE) >> 1][0] & 0x00FF);
                        load_signals[ldmdr] = TRUE;
                    }else{
                        NEXT_LATCHES.MDR = Low16bits((MEMORY[CURRENT_LATCHES.MAR >> 1][1] << 8));
                        NEXT_LATCHES.MDR = NEXT_LATCHES.MDR + (MEMORY[CURRENT_LATCHES.MAR >> 1][0] & 0x00FF);
                        load_signals[ldmdr] = TRUE;
                    }
                    break;
                }
                case WRITE:{
                    int write_size = GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
                    if(write_size == 1){
                        MEMORY[CURRENT_LATCHES.MAR >> 1][0] = Low16bits(CURRENT_LATCHES.MDR & 0x00FF);
                        //Left shift 8 bits since byte addressable
                        MEMORY[CURRENT_LATCHES.MAR >> 1][1] = Low16bits((CURRENT_LATCHES.MDR & 0x0000FF00) >> 8);
                    }
                    //write size is one byte and MAR[0] so low byte of word of memory
                    else if((CURRENT_LATCHES.MAR & 0x0001) == 0){
                        MEMORY[CURRENT_LATCHES.MAR >> 1][0] = Low16bits(CURRENT_LATCHES.MDR & 0x00FF);
                    }
                    //write size is one byte and MAR[1] so high byte of word of memory
                    else{
                        MEMORY[CURRENT_LATCHES.MAR >> 1][1] = Low16bits((CURRENT_LATCHES.MDR & 0x0000FF00) >> 8);
                    }
                    break;
                }
            }
        }
        mem_cycle++;
        return;
    }
}


  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */  
void eval_bus_drivers() {
    if(exception_or_interrupt_skip){
        return;
    }
    if(GetGATE_VECTOR(CURRENT_LATCHES.MICROINSTRUCTION)){
        if(interrupts == TRUE){
            BUS = Low16bits(0x0200 | CURRENT_LATCHES.INTV);
            NEXT_LATCHES.INTV = 0;
            interrupts = FALSE;
        }else if(exceptions == TRUE){
            BUS = Low16bits(0x0200 | CURRENT_LATCHES.EXCV);
            NEXT_LATCHES.EXCV = 0;
            exceptions = FALSE;
        }
    }
    if(GetGATE_PSR(CURRENT_LATCHES.MICROINSTRUCTION)){
        if(GetCLR_PSR(CURRENT_LATCHES.MICROINSTRUCTION)){
            if((CURRENT_LATCHES.PSR & 0x8000) == 0x8000){
                NEXT_LATCHES.STATE_NUMBER = 39;
                copy_microinstruction();
            }
            NEXT_LATCHES.PSR = NEXT_LATCHES.PSR & 0x7FFF;
        }
        if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)){
            NEXT_LATCHES.EXCV = CURRENT_LATCHES.EXCV << 1;
            NEXT_LATCHES.INTV = CURRENT_LATCHES.INTV << 1;
        }
        BUS = Low16bits(CURRENT_LATCHES.PSR);
    }
    if(GetGATE_SP(CURRENT_LATCHES.MICROINSTRUCTION)){
        if(GetLD_SP(CURRENT_LATCHES.MICROINSTRUCTION)){
            //SP - 2
            if(GetSP_MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                NEXT_LATCHES.REGS[6] = CURRENT_LATCHES.REGS[6] - 2;
                CURRENT_LATCHES.REGS[6] -= 2;
                BUS = Low16bits(NEXT_LATCHES.REGS[6]);
            }
            //SP + 2
            if(GetSP_MUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                NEXT_LATCHES.REGS[6] = CURRENT_LATCHES.REGS[6] + 2;
                CURRENT_LATCHES.REGS[6] += 2;
                BUS = Low16bits(NEXT_LATCHES.REGS[6]);
            }
        }else{
            BUS = Low16bits(CURRENT_LATCHES.REGS[6]);
        }
    }
    if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)){
        if(GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
            __uint16_t immediate = 0;
            immediate = CURRENT_LATCHES.IR & 0x00FF;
            immediate = immediate << 1;
            BUS = Low16bits(immediate);
        }
        else{
            __int16_t addr2_result = 0;
            __int16_t addr1_result = 0;
            switch(GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                case 0:{
                    addr2_result = 0;
                    break;
                }
                case 1:{
                    addr2_result = CURRENT_LATCHES.IR & 0x003F;
                    addr2_result = sign_extend(addr2_result, 6);
                    addr2_result = Low16bits(addr2_result);
                    break; 
                }
                case 2:{
                    addr2_result = CURRENT_LATCHES.IR & 0x01FF;
                    addr2_result = sign_extend(addr2_result, 9);
                    addr2_result = Low16bits(addr2_result);
                    break;
                }
                case 3:{
                    addr2_result = CURRENT_LATCHES.IR & 0x07FF;
                    addr2_result = sign_extend(addr2_result, 12);
                    addr2_result = Low16bits(addr2_result);
                    break;
                }
            }
            if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==1){
                addr2_result = addr2_result << 1;
            }
            switch(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                case 0:{
                    addr1_result = CURRENT_LATCHES.PC;
                    addr1_result = Low16bits(addr1_result);
                    break;
                }
                case 1:{
                    if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                        int base_reg_idx = CURRENT_LATCHES.IR & 0x01C0;
                        base_reg_idx = base_reg_idx >> 6;
                        addr1_result = CURRENT_LATCHES.REGS[base_reg_idx];
                        addr1_result = Low16bits(addr1_result);
                        break;
                    }
                }
            }
            //protection exception -> needs to be changed for Lab 5
            if((addr1_result + addr2_result) < 0x3000 && (NEXT_LATCHES.PSR & 0x8000) == 0x8000){
                exception_or_interrupt_skip = TRUE;
                exceptions = TRUE;
                NEXT_LATCHES.EXCV = 0x02;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            }
            //unaligned exception
            else if(((addr1_result + addr2_result) & 0x01) == 1 && GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==1){
                exception_or_interrupt_skip = TRUE;
                exceptions = TRUE;
                NEXT_LATCHES.EXCV = 0x03;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            }
            BUS = Low16bits(addr1_result + addr2_result);
        }
    }else if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)){
        int sr1 = 0;
        int sr2 = 0;
        int immediate = 0;
        int aluk = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
        if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
            sr1 = (CURRENT_LATCHES.IR & 0x01C0) >> 6;
            if((CURRENT_LATCHES.IR & 0x0020) == 0){
                sr2 = (CURRENT_LATCHES.IR & 0x0007);
                int result = 0;
                switch(aluk){
                    case 0:{
                        result = CURRENT_LATCHES.REGS[sr1] + CURRENT_LATCHES.REGS[sr2];
                        result = Low16bits(result);
                        BUS = result;
                        break;
                    }
                    case 1:{
                        result = CURRENT_LATCHES.REGS[sr1] & CURRENT_LATCHES.REGS[sr2];
                        result = Low16bits(result);
                        BUS = result;
                        break;
                    }
                    case 2:{
                        result = CURRENT_LATCHES.REGS[sr1] ^ CURRENT_LATCHES.REGS[sr2];
                        result = Low16bits(result);
                        BUS = result;
                        break;
                    }
                    default:{
                        printf("error in control code for ALUK");
                        break;
                    }
            }
            }else{
                immediate = (CURRENT_LATCHES.IR & 0x001F);
                immediate = sign_extend(immediate, 5);
                int result = 0;
                    switch(aluk){
                        case 0:{
                            result = CURRENT_LATCHES.REGS[sr1] + immediate;
                            result = Low16bits(result);
                            BUS = result;
                            break;
                        }
                        case 1:{
                            result = CURRENT_LATCHES.REGS[sr1] & immediate;
                            result = Low16bits(result);
                            BUS = result;
                            break;
                        }
                        case 2:{
                            result = CURRENT_LATCHES.REGS[sr1] ^ immediate;
                            result = Low16bits(result);
                            BUS = result;
                            break;
                        }
                        default:{
                            printf("error in control code for ALUK");
                            break;
                        }
                }
            }

        }
        else{
            sr1 = (CURRENT_LATCHES.IR & 0x0E00) >> 9;
            BUS = CURRENT_LATCHES.REGS[sr1];
        }

    }else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)){
        if((CURRENT_LATCHES.MAR & 0x1) == 1){
            if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int temp = sign_extend((CURRENT_LATCHES.MDR & 0xFF00) >> 8, 8);
                BUS = Low16bits(temp);
            }
        }else{
            if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int temp = sign_extend(CURRENT_LATCHES.MDR & 0x00FF, 8);
                BUS = Low16bits(temp);
            }else{
                if(GetLD_SP(CURRENT_LATCHES.MICROINSTRUCTION)){
                    if(GetSP_MUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                        NEXT_LATCHES.REGS[6] = CURRENT_LATCHES.REGS[6] + 2;
                        CURRENT_LATCHES.REGS[6] += 2;
                    }
                }
                BUS = Low16bits(CURRENT_LATCHES.MDR);
            }
        }
    }else if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)){

        __uint8_t pc_mux = GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
        int addr1_result = 0;
        int addr2_result = 0;
        if(pc_mux == 2 && GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION) != 1){
            switch(GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                case 0:{
                    addr2_result = 0;
                    break;
                }
                case 1:{
                    addr2_result = CURRENT_LATCHES.IR & 0x003F;
                    addr2_result = sign_extend(addr2_result, 6);
                    addr2_result = Low16bits(addr2_result);
                    break; 
                }
                case 2:{
                    addr2_result = CURRENT_LATCHES.IR & 0x01FF;
                    addr2_result = sign_extend(addr2_result, 9);
                    addr2_result = Low16bits(addr2_result);
                    break;
                }
                case 3:{
                    addr2_result = CURRENT_LATCHES.IR & 0x07FF;
                    addr2_result = sign_extend(addr2_result, 11);
                    addr2_result = Low16bits(addr2_result);
                    break;
                }
            }
            if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)==1){
                addr2_result = addr2_result << 1;
            }
            switch(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                case 0:{
                    addr1_result = CURRENT_LATCHES.PC;
                    addr1_result = Low16bits(addr1_result);
                    break;
                }
                case 1:{
                    int base_reg_idx = CURRENT_LATCHES.IR & 0x01C0;
                    base_reg_idx = base_reg_idx >> 6;
                    addr1_result = CURRENT_LATCHES.REGS[base_reg_idx];
                    addr1_result = Low16bits(addr1_result);
                    break;
                }
            }
            BUS = Low16bits(addr1_result + addr2_result);
        }else{
            BUS = Low16bits(CURRENT_LATCHES.PC);
        }
        
    }else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        __uint16_t type_shift = CURRENT_LATCHES.IR & 0x0030;
        type_shift = type_shift >> 4;
        __uint8_t immediate = CURRENT_LATCHES.IR & 0x000F;
        int sr1 = CURRENT_LATCHES.IR & 0x01C0;
        sr1 = sr1 >> 6;
        switch(type_shift){
            case 0:{
                __int16_t val = CURRENT_LATCHES.REGS[sr1] << immediate;
                val = Low16bits(val);
                BUS = val;
                break;
            }
            case 1:{
                __uint16_t val = CURRENT_LATCHES.REGS[sr1] >> immediate;
                val = Low16bits(val);
                BUS = val;
                break;
            }
            case 3:{
                __int16_t val = sign_extend(CURRENT_LATCHES.REGS[sr1],16) >> immediate;
                val = Low16bits(val);
                BUS = val;
                BUS = Low16bits(BUS);
                break;
            }
        }
    };
}

  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */   
void drive_bus() {
    if(exception_or_interrupt_skip){
        return;
    }
    if(GetGATE_VECTOR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)){
            NEXT_LATCHES.MAR = Low16bits(BUS);
        }
    }
    if(GetGATE_PSR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.MDR = Low16bits(BUS);
        }
    }
    if(GetGATE_SP(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.MAR = Low16bits(BUS);
        }
    }
    if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int reg_idx = CURRENT_LATCHES.IR & 0x0E00;
                reg_idx = reg_idx >> 9;
                NEXT_LATCHES.REGS[reg_idx] = Low16bits(BUS);
            }else{
                NEXT_LATCHES.REGS[7] = Low16bits(BUS);
            }
            load_signals[ldreg] = TRUE;
        }
        if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if((CURRENT_LATCHES.MAR & 0x1) == 0){
                NEXT_LATCHES.MDR = Low16bits(BUS);
                load_signals[ldmdr] = TRUE;
            }else{
                NEXT_LATCHES.MDR = Low16bits(BUS & 0x00FF);
                int temp = Low16bits(BUS & 0x00FF);
                temp = temp << 8;
                temp = Low16bits(temp);
                NEXT_LATCHES.MDR += temp;
                load_signals[ldmdr] = TRUE;
            }
        }
    }
    else if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int reg_idx = CURRENT_LATCHES.IR & 0x0E00;
                reg_idx = reg_idx >> 9;
                NEXT_LATCHES.REGS[reg_idx] = Low16bits(BUS);
            }else if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
                NEXT_LATCHES.REGS[7] = Low16bits(BUS);
            }
            load_signals[ldreg] = TRUE;
        }
        if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.MAR = Low16bits(BUS);
            load_signals[ldmar] = TRUE;
            if(NEXT_LATCHES.INTV!=0){
                interrupts = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.STATE_NUMBER = 37;
                copy_microinstruction();
            }
            //protection exception -> needs to be changed for Lab 5
            if((NEXT_LATCHES.PSR & 0x8000)==0x8000 && NEXT_LATCHES.MAR < 0x3000 && (NEXT_LATCHES.IR & 0xFF00) != 0xFF00){
                exceptions = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.EXCV = 0x02;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            }
        }
        if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION) && load_signals[ldmdr] != TRUE){
            NEXT_LATCHES.MDR = Low16bits(BUS);
        }
        
    }
    else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_PSR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.PSR = Low16bits(BUS);
            if((NEXT_LATCHES.PSR & 0x8000) == 0x8000){
                //otherwise go to 18
                NEXT_LATCHES.STATE_NUMBER = 52;
                copy_microinstruction();
            }
        }
        if(GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.IR = Low16bits(BUS);
            load_signals[ldir] = TRUE;
            //unknown opcode exception
            if((NEXT_LATCHES.IR & 0xF000) == 0xA000 || (NEXT_LATCHES.IR & 0xF000) == 0xB000){
                exceptions = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.EXCV = 0x04;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            }
        }
        if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.PC = Low16bits(BUS);
            load_signals[ldpc] = TRUE;
        }
        if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int reg_idx = CURRENT_LATCHES.IR & 0x0E00;
                reg_idx = reg_idx >> 9;
                NEXT_LATCHES.REGS[reg_idx] = Low16bits(BUS);
            }else{
                NEXT_LATCHES.REGS[7] = Low16bits(BUS);
            }
            load_signals[ldreg] = TRUE;
        }
    }
    else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            NEXT_LATCHES.MAR = Low16bits(BUS);
            load_signals[ldmar] = TRUE;
            //protection exception -> needs to be changed for Lab 5
            if((NEXT_LATCHES.PSR & 0x8000)==0x8000 && NEXT_LATCHES.MAR < 0x3000 && (NEXT_LATCHES.IR & 0xFF00)!=0xF000){
                exceptions = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.EXCV = 0x02;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            }
        }
        if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int dr_idx = CURRENT_LATCHES.IR & 0x0E00;
                dr_idx = dr_idx >> 9;
                NEXT_LATCHES.REGS[dr_idx] = Low16bits(BUS);
            }
        }
    }
    else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                int reg_idx = CURRENT_LATCHES.IR & 0x0E00;
                reg_idx = reg_idx >> 9;
                NEXT_LATCHES.REGS[reg_idx] = Low16bits(BUS);
            }
        }
    }
}

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */  
void latch_datapath_values() {
    load_signals[ldmdr] = FALSE;
    if(exception_or_interrupt_skip){
        exception_or_interrupt_skip = FALSE;
        return;
    }
    if(GetSW_SSP(CURRENT_LATCHES.MICROINSTRUCTION)){
        NEXT_LATCHES.USP = Low16bits(CURRENT_LATCHES.REGS[6]);
        NEXT_LATCHES.REGS[6] = Low16bits(CURRENT_LATCHES.SSP);
    }
    if(GetSW_USP(CURRENT_LATCHES.MICROINSTRUCTION)){
        NEXT_LATCHES.SSP = Low16bits(CURRENT_LATCHES.REGS[6]);
        NEXT_LATCHES.REGS[6] = Low16bits(CURRENT_LATCHES.USP);
    }
    if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==0b11){
        NEXT_LATCHES.PC = CURRENT_LATCHES.PC - 2;
        load_signals[ldpc] = TRUE;
    }
    if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)==1 && load_signals[ldpc] == FALSE){
        if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0){
            NEXT_LATCHES.PC += 2;
        }
        if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==1){
            int reg_idx = (CURRENT_LATCHES.IR & 0x01C0) >> 6;
            NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[reg_idx];
            //protection exception -> needs to be changed
            if(NEXT_LATCHES.PC < 0x3000 && (NEXT_LATCHES.PSR & 0x8000) == 0x8000){
                exceptions = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.EXCV = 0x02;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            } 
        }
        if(GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION)==2){ 
            int addr2 = 0;
            int addr1 = 0;

            switch(GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION)){
                case 0:{
                    addr2 = 0;
                    break;
                }
                case 1:{
                    addr2 = sign_extend(CURRENT_LATCHES.IR & 0x003F, 6);
                    break;
                }
                case 2:{
                    addr2 = sign_extend(CURRENT_LATCHES.IR & 0x01FF, 9);
                    break;
                }
                case 3:{
                    addr2 = sign_extend(CURRENT_LATCHES.IR & 0x07FF, 11);
                    break;
                }
            }
            if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)){
                addr2 = addr2 << 1;
            }
            if(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)==0){
                addr1 = CURRENT_LATCHES.PC;
                addr1 = Low16bits(addr1);
            }else if(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)==1){
                int reg_idx = CURRENT_LATCHES.IR & 0x01C0;
                reg_idx = reg_idx >> 6;
                addr1 = CURRENT_LATCHES.REGS[reg_idx];
            }
            NEXT_LATCHES.PC = Low16bits(addr1 + addr2);
            //protection exception
            if(NEXT_LATCHES.PC < 0x3000 && (NEXT_LATCHES.PSR & 0x8000) == 0x8000){
                exceptions = TRUE;
                exception_or_interrupt_skip = TRUE;
                NEXT_LATCHES.EXCV = 0x02;
                NEXT_LATCHES.STATE_NUMBER = 36;
                copy_microinstruction();
            } 
        }
    }
    load_signals[ldpc] = FALSE;
    if(GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        NEXT_LATCHES.BEN = (((CURRENT_LATCHES.IR & 0x0800) >> 11) & CURRENT_LATCHES.N) + (((CURRENT_LATCHES.IR & 0x0400) >> 10) & CURRENT_LATCHES.Z) + (((CURRENT_LATCHES.IR & 0x0200) >> 9) & CURRENT_LATCHES.P);
    }
    if(GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION)==1){
        int reg_idx = CURRENT_LATCHES.IR & 0x0E00;
        reg_idx = reg_idx >> 9;
        if(NEXT_LATCHES.REGS[reg_idx] == 0){
            NEXT_LATCHES.Z = 1;
            NEXT_LATCHES.P = 0;
            NEXT_LATCHES.N = 0;
            NEXT_LATCHES.PSR = (CURRENT_LATCHES.PSR & 0xFFF8) | 0x0002;
        }
        else if(((NEXT_LATCHES.REGS[reg_idx] & 0x00008000) >> 15) == 1){
            NEXT_LATCHES.Z = 0;
            NEXT_LATCHES.P = 0;
            NEXT_LATCHES.N = 1;
            NEXT_LATCHES.PSR = (CURRENT_LATCHES.PSR & 0xFFF8) | 0x0004;
        }
        else if((NEXT_LATCHES.REGS[reg_idx] & 0x00008000) == 0){
            NEXT_LATCHES.Z = 0;
            NEXT_LATCHES.P = 1;
            NEXT_LATCHES.N = 0;
            NEXT_LATCHES.PSR = (CURRENT_LATCHES.PSR & 0xFFF8) | 0x0001;
        }
    }
}
