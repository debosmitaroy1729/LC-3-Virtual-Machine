#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; //memory storage

//Registers
enum
{
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,//General Purpose Registers (holds numbers, addresses, results of operations)
	R_PC, //program counter, holds the address of the next instruction to execute
	R_COND, //Condition Flags, stores comparison results 
	R_COUNT
};

uint16_t reg[R_COUNT]; //Register Storage

//OPCodes
enum
{
	OP_BR = 0, //branch
	OP_ADD, //add
	OP_LD, //load
	OP_ST, //store
	OP_JSR, //jump register
	OP_AND, //bitwise AND
	OP_LDR, //load register
	OP_STR, //store register
	OP_RTI, //unused
	OP_NOT, //bitwise NOT
	OP_LDI, //load indirect
	OP_STI, //store indirect 
	OP_JMP, //jump
	OP_RES, //reserved (unused)
	OP_LEA, //load effective address
	OP_TRAP //execute trap
};

//Condition Flags
enum
{
	FL_POS = 1 << 0, //P
	FL_ZRO = 1 << 1, //Z
	FL_NEG = 1 << 2 //N
};

enum
{
	TRAP_GETC = 0x20, //gets character from keyboard, not reflected in terminal
	TRAP_OUT = 0x21, //output a character
	TRAP_PUTS = 0x22, //output a word string
	TRAP_IN = 0x23, //gets character from keyboard, reflected in terminal
	TRAP_PUTSP = 0x24, //output byte string
	TRAP_HALT = 0x25 //halt program
};

//Sign extend function
uint16_t signExtend(uint16_t x, int bit_count)
{
	if((x >> (bit_count -1)) & 1) //check if the first bit is signed 
	{
		x|= (0xFFFF << bit_count); //set all higher bits to 1
	}
	return x;
}

//Flag update function 
void updateFlags(uint16_t r)
{
	if (reg[r] == 0)
	{
		reg[R_COND] = FL_ZRO;
	}

	else if (reg[r] >> 15)
	{
		reg[R_COND] = FL_NEG;
	}

	else 
	{
		reg[R_COND] = FL_POS;
	}
}

enum
{
	MR_KBSR = 0xFE00, //keyboard status
	MR_KBDR = 0xFE02 //keyboard data
};

void memWrite(uint16_t address, uint16_t val)
{
	memory[address] = val;
}

struct termios original_tio;

void disable_input_buffering()
{
	tcgetattr(STDIN_FILENO, &original_tio);
	struct termios new_tio = original_tio;
	new_tio.c_lflag &= ~ICANON & ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

uint16_t check_key()
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

uint16_t memRead(uint16_t address)
{
	if (address == MR_KBSR)
	{
		if (check_key())
		{
			memory[MR_KBSR] = (1 << 15);
			memory[MR_KBDR] = getchar();
		}

		else
		{
			memory[MR_KBSR] = 0;
		}

	}

	return memory[address];
}

uint16_t swap16(uint16_t x)
{
	return ((x >> 8)|(x << 8));
}

void readImageFile(FILE* file)
{
	uint16_t origin;
	fread(&origin, sizeof(origin), 1, file);
	origin = swap16(origin);

	uint16_t max_read = MEMORY_MAX - origin;
	uint16_t* p = memory + origin;
	size_t read = fread(p, sizeof(uint16_t), max_read, file);

	while(read-- > 0)
	{
		*p = swap16(*p);
		p++;
	}
}

int readImage(const char* imagePath)
{
	FILE* file = fopen(imagePath, "rb");
	if (!file) return 0;
	readImageFile(file);
	fclose(file);
	return 1;
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\nInterrupted\n");
    exit(1);
}


int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		printf("LC-3 [image-file]\n");
		exit(2);
	}

	if (!readImage(argv[1]))
	{
		printf("Failed to load image file\n");
		exit(1);
	}

	disable_input_buffering();
	atexit(restore_input_buffering);


	reg[R_PC] = 0x3000; //counter set to starting location of program

	int running = 1;

	signal(SIGINT, handle_interrupt);

	while (running)
	{
		uint16_t instr = memRead(reg[R_PC]++); //Fetching instruction

		uint16_t opcode = instr >> 12; //Decoding Opcode by extracting top 4 bits of instruction

		switch(opcode)
		{
			case OP_ADD:
				{
					uint16_t r0 = (instr >> 9) & 0x7; //destination register
					uint16_t r1 = (instr >> 6) & 0x7; //first operand

					uint16_t imm_flag = (instr >> 5) & 0x1; //Immeediate mode check

					if (imm_flag)
					{
						uint16_t imm5 = signExtend(instr & 0x1F, 5);
						reg[r0] = reg[r1] + imm5;
					}

					else
					{
						uint16_t r2 = instr & 0x7;
						reg[r0] = reg[r1] + reg[r2];
					}

					updateFlags(r0);
				}
				break;
			
			case OP_AND:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t r1 = (instr >> 6) & 0x7;
					uint16_t imm_flag = (instr >> 5) & 0x1;

					if (imm_flag)
					{
						uint16_t imm5 = signExtend(instr & 0x1F, 5);
						reg[r0] = reg[r1] & imm5;
					}

					else
					{
						uint16_t r2 = instr & 0x7;
						reg[r0] = reg[r1] & reg[r2];
					}
				}
				break;

			case OP_NOT:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t r1 = (instr >> 6) & 0x7;

					reg[r0] = ~reg[r1];
					updateFlags(r0);
				}
				break;

			case OP_BR:
				{
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9);
					uint16_t cond_flag = (instr >> 9) & 0x7;
					
					if (cond_flag & reg[R_COND])
					{
						reg[R_PC] += pc_offset;
					}
				}
				break;

			case OP_JMP:
				{
					uint16_t r1 = (instr >> 6) & 0x7;
					reg[R_PC] = reg[r1];
				}
				break;

			case OP_JSR:
				{
					uint16_t longFlag = (instr >> 11) & 1;
					reg[R_R7] = reg[R_PC];

					if (longFlag)
					{
						uint16_t long_pc_offset  = signExtend(instr & 0x7FF, 11);
						reg[R_PC] += long_pc_offset; //jump to subroutine
					}

					else
					{
						uint16_t r1 = (instr >> 6) & 0x7;
						reg[R_PC] = reg[r1]; //jump register to subroutine
					}
				}
				break;

			case OP_LD:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9);
					reg[r0] = memRead(reg[R_PC] + pc_offset);
					updateFlags(r0);
				}
				break;

			case OP_LDI:
				{
					uint16_t r0 = (instr >> 9) & 0x7; //destination register
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9); //PC Offset

					reg[r0] = memRead(memRead(reg[R_PC] + pc_offset));
					updateFlags(r0);
				}
				break;

			case OP_LDR:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t r1 = (instr >> 6) & 0x7;

					uint16_t offset = signExtend(instr & 0x3F, 6);

					reg[r0] = memRead(reg[r1] + offset);

					updateFlags(r0);
				}
				break;

			case OP_LEA:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9);
					reg[r0] = reg[R_PC] + pc_offset;
					updateFlags(r0);
				}
				break;

			case OP_ST:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9);

					memWrite(reg[R_PC] + pc_offset, reg[r0]);
				}
				break;

			case OP_STI: 
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t pc_offset = signExtend(instr & 0x1FF, 9);

					memWrite(memRead(reg[R_PC] + pc_offset), reg[r0]);
				}
				break;

			case OP_STR:
				{
					uint16_t r0 = (instr >> 9) & 0x7;
					uint16_t r1 = (instr >> 6) & 0x7;
					uint16_t offset = signExtend(instr & 0x3F, 6);
					memWrite(reg[r1] + offset, reg[r0]);
				}
				break;

			case OP_TRAP:
				{
					reg[R_R7] = reg[R_PC];

					switch (instr & 0xFF)
					{
						case TRAP_GETC:
							{
								reg[R_R0] = (uint16_t)getchar();
								updateFlags(R_R0);
							}	
							break;

						case TRAP_OUT:
							{
								putc((char)reg[R_R0], stdout);
								fflush(stdout);
							}
							break;

						case TRAP_PUTS:
							{
								uint16_t* c = memory + reg[R_R0];
								while (*c)
								{
									putc((char)* c, stdout);
									++c;
								}
								fflush(stdout);
							}
							break;

						case TRAP_IN:
							{
								printf("Enter a character: ");
								char c = getchar();
								putc(c, stdout);
								fflush(stdout);
								reg[R_R0] = (uint16_t)c;
								updateFlags(R_R0);
							}
							break;

						case TRAP_PUTSP:
							{
								uint16_t* c = memory + reg[R_R0];
								while (*c)
								{
									char c1 = (*c) & 0xFF;
									putc(c1, stdout);
									char c2 = (*c) >> 8;
									if (c2) putc(c2, stdout);
									++c;
								}
								fflush(stdout);
							}
							break;

						case TRAP_HALT:
							{
								puts("HALT");
								fflush(stdout);
								running = 0;
							}
							break;
					}
				}
				break;

			case OP_RES:
				abort();
			case OP_RTI:
				abort();
			default: 
				printf("BAD OPCODE: %d\n", opcode);
				running = 0;
				break;
		}
	}

	return 0;
}
