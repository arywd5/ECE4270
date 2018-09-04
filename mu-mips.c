#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	//Current state/next state

	uint32_t instr, temp, temp2, temp3;
	uint8_t command, flag = 0, rs, rt, rd, sa; 
	uint16_t immed;

	//Get instr value
	instr = mem_read_32(CURRENT_STATE.REGS[28] + CURRENT_STATE.PC);

	command = instr >> 26;
	flag = 0;
	
	if (command == 0){
		command = instr & 63;
		flag = 1;
	}
	
	if (command == 1){
		command = (instr >> 16) & 31;
		flag = 2;
	}

	rs = (instr >> 21) & 31;
	rt = (instr >> 16) & 31;
	rd = (instr >> 11) & 31;
	sa = (instr >> 6) & 31;
	immed = instr & 0x0FFFF;

	//Parse
	switch (command){
		//ADD && LB
		case 32:
			if (flag == 0){	//LB
				temp = mem_read_32(rs + immed);	//Base + offset
				NEXT_STATE.REGS[rt] = temp;  //Put memory into rt
			} else {	//Add
				//temp = mem_read_32(rs + immed);
				temp = rs;
				printf("Temp = %zu\n", temp);
				//temp2 = mem_read_32(rt + immed);
				temp2 = rt;
				printf("Temp2 = %zu\n", temp2);
				temp3 = temp + temp2;
				printf("Temp3 = %zu\n", temp3);
				if ( ((temp3 >> 30) == 1) || ((temp3 >> 30) == 2) ){
					//Overflow
					printf("Overflow\n");
				} else {
					NEXT_STATE.REGS[rd] = temp3;
				}
			}
			NEXT_STATE.PC += 4;
			break;
		//ADDI && JR
		case 8:
			if (flag == 0){ //ADDI
				
			} else {	//JR
				temp = mem_read_32(rs);
				if ((temp && 0x00000003) == 0){
					NEXT_STATE.PC += temp;
				} else {
					//Exception	
				}
			}			
			break;
		//ADDIU && JALR
		case 9:
			if (flag == 1){//JALR
				
			} else {	//ADDIU
				NEXT_STATE.PC += 4;
			}
			break;
		//ADDU && LH
		case 33:
			if (flag == 0){//LH
				
			} else {	//ADDU
				
			}
		//SUB
		case 34:
			
			break;
		//SUBU
		case 35:
			if (flag == 0){//LW
				NEXT_STATE.PC += 4;
			} else {//SUBU
				
			}			
			break;
		//MULT
		case 24:	
			
			break;
		//MULTU
		case 25:
			
			break;
		//DIV
		case 26:
			
			break;
		//DIVU
		case 27:
			
			break;
		//AND
		case 36:
			
			break;
		//ANDI && SYSCALL
		case 12:
			if (flag == 0){	//ANDI
				
			} else {	//SYSCALL
				RUN_FLAG = FALSE;
			}
			break;
		//OR
		case 37:
			
			break;
		//ORI
		case 13:
			
			break;
		//XOR
		case 38:
			
			break;
		//XORI
		case 14:
			
			break;
		//NOR
		case 39:
			
			break;
		//SLT
		case 42:
			
			break;
		//SLTI
		case 10:
			
			break;
		//SLL && BLTZ
		case 0:
			if (flag == 2){//BLTZ
				
			} else {	//SLL		
				
			}
			break;
		//SRL && J
		case 2:
			if (flag == 0){//J
				command = instr & 0x3FFFFFF;
				
			} else {	//SRL
				
			}
			break;
		//SRA && JAL
		case 3:
			if (flag == 0){//JAL
				command = instr & 0x3FFFFFF;
				
			} else {	//SRA
				
			}
			break;
		//LUI
		case 15:
			NEXT_STATE.PC += 4;
			break;
		//SW
		case 43:
			NEXT_STATE.PC += 4;
			break;
		//SB
		case 40:
			
			break;
		//SH
		case 41:
			
			break;
		//MFHI
		case 16:
			
			break;
		//MFLO
		case 18:
			
			break;
		//MTHI
		case 17:
			
			break;
		//MTLO
		case 19:
			
			break;
		//BEQ
		case 4:
			
			break;
		//BNE
		case 5:
			
			break;
		//BLEZ
		case 6:
			
			break;
		//BGEZ
		case 1:
			
			break;
		//BGTZ
		case 7:
			
			break;
		default: 
			
			break;
	}
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	
	uint32_t instr;
	uint8_t command, flag = 0, rs, rt, rd, sa; 
	uint16_t immed;

	//Get instr value
	instr = mem_read_32(addr);

	command = instr >> 26;
	flag = 0;
	
	if (command == 0){
		command = instr & 63;
		flag = 1;
	}
	
	if (command == 1){
		command = (instr >> 16) & 31;
		flag = 2;
	}

	rs = (instr >> 21) & 31;
	rt = (instr >> 16) & 31;
	rd = (instr >> 11) & 31;
	sa = (instr >> 6) & 31;
	immed = instr & 0x0FFFF;

	//Parse
	switch (command){
		//ADD && LB
		case 32:
			if (flag == 0){
				printf("LB \t BASE = %d \t RT = %d\n", rs, rt);
			} else {
				printf("ADD \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			}
			break;
		//ADDI && JR
		case 8:
			if (flag == 0){
				printf("ADDI \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			} else {
				printf("JR \t RS = %d\n", rs);			
			}			
			break;
		//ADDIU && JALR
		case 9:
			if (flag == 1){
				printf("JALR \t RS = %d \t RD = %d\n", rs, rd);
			} else {
				printf("ADDIU \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			}
			break;
		//ADDU && LH
		case 33:
			if (flag == 0){
				printf("LH \t BASE = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			} else {
				printf("ADDU \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			}
		//SUB
		case 34:
			printf("SUB \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//SUBU
		case 35:
			if (flag == 0){
				printf("LW \t BASE = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			} else {
				printf("SUBU \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			}			
			break;
		//MULT
		case 24:
			printf("MULT \t RS = %d \t RT = %d\n", rs, rt);
			break;
		//MULTU
		case 25:
			printf("MULTU \t RS = %d \t RT = %d\n", rs, rt);
			break;
		//DIV
		case 26:
			printf("DIV \t RS = %d \t RT = %d\n", rs, rt);
			break;
		//DIVU
		case 27:
			printf("DIVU \t RS = %d \t RT = %d\n", rs, rt);
			break;
		//AND
		case 36:
			printf("AND \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//ANDI && SYSCALL
		case 12:
			if (flag == 0){
				printf("ANDI \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			} else {
				printf("SYSCALL\n");
			}
			break;
		//OR
		case 37:
			printf("OR \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//ORI
		case 13:
			printf("ORI \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			break;
		//XOR
		case 38:
			printf("XOR \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//XORI
		case 14:
			printf("XORI \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			break;
		//NOR
		case 39:
			printf("NOR \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//SLT
		case 42:
			printf("SLT \t RS = %d \t RT = %d \t RD = %d\n", rs, rt, rd);
			break;
		//SLTI
		case 10:
			printf("SLTI \t RS = %d \t RT = %d \t IMMEDIATE = %d\n", rs, rt, immed);
			break;
		//SLL && BLTZ
		case 0:
			if (flag == 2){
				printf("BLTZ \t RS = %d \t OFFSET = %d\n", rs, immed);
			} else {			
				printf("SLL \t RT = %d \t RD = %d \t SA = %d\n", rt, rd, sa);
			}
			break;
		//SRL && J
		case 2:
			if (flag == 0){
				command = instr & 0x3FFFFFF;
				printf("J\tTarget: %d\n", command);
			} else {
				printf("SRL \t RT = %d \t RD = %d \t SA = %d\n", rt, rd, sa);
			}
			break;
		//SRA && JAL
		case 3:
			if (flag == 0){
				command = instr & 0x3FFFFFF;
				printf("JAL\tTarget: %d\n", command);
			} else {
				printf("SRA \t RT = %d \t RD = %d \t SA = %d\n", rt, rd, sa);
			}
			break;
		//LUI
		case 15:
			printf("LUI \t RT = %d \t IMMEDIATE = %d\n", rt, immed);
			break;
		//SW
		case 43:
			printf("SW \t BASE = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			break;
		//SB
		case 40:
			printf("SB \t BASE = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			break;
		//SH
		case 41:
			printf("SH \t BASE = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			break;
		//MFHI
		case 16:
			printf("MFHI \t RD = %d\n", rd);
			break;
		//MFLO
		case 18:
			printf("MFLO \t RD = %d\n", rd);
			break;
		//MTHI
		case 17:
			printf("MTHI \t RS = %d\n", rs);
			break;
		//MTLO
		case 19:
			printf("MTLO \t RS = %d\n", rs);
			break;
		//BEQ
		case 4:
			printf("BEQ \t RS = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			break;
		//BNE
		case 5:
			printf("BNE \t RS = %d \t RT = %d \t OFFSET = %d\n", rs, rt, immed);
			break;
		//BLEZ
		case 6:
			printf("BLEZ \t RS = %d \t OFFSET = %d\n", rs, immed);
			break;
		//BGEZ
		case 1:
			printf("BGEZ \t RS = %d \t OFFSET = %d\n", rs, immed);
			break;
		//BGTZ
		case 7:
			printf("BGTZ \t RS = %d \t OFFSET = %d\n", rs, immed);
			break;
		default: 
			printf("\n");
			break;
	}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
