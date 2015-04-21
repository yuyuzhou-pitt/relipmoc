/*************************************************************/
/*  proj4.c

   This file consists of two parts.  
    a. data structures of code storage
    b. emit functions
*/

/**************** data structures *******************/

#define	DATA_LINE_MAX	500
	/* # of lines in data definition */
#define CODE_LINE_MAX	8000
	/* # of lines in code */
#define DATA_MAX	10000
	/* # of chars in data definition */
#define CODE_MAX	100000
	/* # of chars in code */

typedef struct 
{
  int mode;
	/* operand mode, see definitions below */
  int num_const;
	/* number constant or the offset in register displayment mode */
  char char_const;
	/* char constant */
  char *ident;
	/* identifier */
  int reg;
	/* register number */
} OPERAND;	

/* operand mode */
#define NUM_CONST	50
	/* operand is a number constant */
#define CHAR_CONST	60
	/* char constant */
#define IDENTIFIER	70
	/* operand is an identifier */
#define REGISTER	80
	/* operand is in a register */
#define REG_DEFER	90
	/* the address of operand is the content of a reg */
#define REG_DISPL	100
	/* the address of operand is the content of a reg + a number */

/* register definition */
#define ZERO	0	//Constant 0
#define AT	1	//Reserved for assembler
#define V0	2	//Expression evaluation and result of a function
#define V1	3	//Result of a function
#define A0	4	//Arugment 1
#define A1	5	//Arugment 2
#define A2	6	//Argument 3
#define A3	7	//Argument 4
#define T0	8	//Temporary ( not preserved acress call)
#define T1	9	//Temporary ( not preserved acress call)
#define T2	10	//Temporary ( not preserved acress call)
#define T3	11	//Temporary ( not preserved acress call)
#define T4	12	//Temporary ( not preserved acress call)
#define T5	13	//Temporary ( not preserved acress call)
#define T6	14	//Temporary ( not preserved acress call)
#define T7	15	//Temporary ( not preserved acress call)
#define S0	16	//Saved Temporary ( preserved acress call)
#define S1	17	//Saved Temporary ( preserved acress call)
#define S2	18	//Saved Temporary ( preserved acress call)
#define S3	19	//Saved Temporary ( preserved acress call)
#define S4	20	//Saved Temporary ( preserved acress call)
#define S5	21	//Saved Temporary ( preserved acress call)
#define S6	22	//Saved Temporary ( preserved acress call)
#define S7	23	//Saved Temporary (preserved acress call)
#define T8	24	//Temporary ( not preserved acress call)
#define T9	25	//Temporary ( not preserved acress call)
#define K0	26	//Reserved for OS kernel
#define K1	27	//Reserved for OS kernel
#define GP	28	//Pointer to global area
#define SP	29	//Stack pointer
#define FP	30	//Frame pointer
#define RA	31	//Return address (used by function call)


/* most frequent used instruction definition */

/* Arithmetic and Logic Instructions */
#define ABS	abs	//abs Rdest, Rsrc		--Absolute Value

#define ADD	add	//add Rdest, Rsrc1, Src2 	--Addition (with overflow)
#define ADDI	addi	//addi Rdest, Rsrc1, Imm	--Addition Immediate (with overflow)
#define ADDU	addu	//addu Rdest, Rsrc1, Src2	--Addition (without overflow)
#define ADDIU	addiu	//addiu Rdest, Rsrc1, Imm	--Addition Immediate (without overflow)

#define AND	and	//and Rdest, Rsrc1, Src2	--AND
#define ANDI	andi	//and Rdest, Rsrc1, Imm		--And Immediate

#define DIV	div	//div Rsrc, Rsrc2		--Divide(singed)
#define	DIVU	divu	//divu Rsrc1, Rsrc2		--Divide(unsigned)

#define MUL	mul	//mul Rdest, Rsrc1, Src2	--Multiply(without overflow)
#define	MULO	mulo	//mulo Rdest, Rsrc1, Src2	--Multiply (with overflow)
#define MULOU	mulou	//mulou Rdest, Rsrc1, Src2	--Unsigned Multiply (with overflow)
#define MULT	mult	//mult Rsrc1, Rsrc2		--Multiply
#define MULTU	multu	//multu Rsrc1, Rsrc2		--Unsigned Multiply

#define NEG	neg	//neg Rdest, Rsrc		--Negate Value(with overflow)
#define	NEGU	negu	//negu Rdest, Rsrc		--Negate Value(without overflow)

#define NOT	not	//not Rdest, Rsrc		--NOT

#define OR	or	//or Rdest, Rsrc1, Rsrc2	--OR
#define ORI	ori	//ori Rdest, Rsrc1, Imm		--OR Immediate

#define SUB	sub	//sub Rdest, Rsrc1, Rsrc2	--Substract (with overflow)
#define SUBU	subu	//subu Rdest, Rsrc1, Rsrc2	--Substract (without overflow)

/* Comparision Instructions */
#define	SEQ	seq	//seq Rdest, Rsrc1, Src2	--Set Equal

#define SGE	sge	//sge Rdest, Rsrc1, Src2        --Set Greater Than Equal
#define SGEU	sgeu	//sgeu Rdest, Rsrc1, Src2       --Set Greater Than Equal Unsigned

#define SGT	sgt	//sgt Rdest, Rsrc1, Src2        --Set Greater Than
#define SGTU	sgtu	//sgtu Rdest, Rsrc1, Src2       --Set Greater Than Unsigned

#define SLE	sle	//sle Rdest, Rsrc1, Src2        --Set Less Than Equal
#define SLEU	sleu	//sleu Rdest, Rsrc1, Src2       --Set Less Than Equal Unsigned

#define SLT	slt	//slt Rdest, Rsrc1, Src2        --Set Less Than
#define SLTU	sltu	//sltu Rdest, Rsrc1, Src2       --Set Less Than Unsigned

#define SNE	sne	//sne Rdest, Rsrc1, Src2        --Set Not Equal

/* Branch and Jump Instructions */
#define B	b	//b label1			--Branch instruction

#define BEQ	beq	//beq Rsrc1, Src2, Label	--Branch on equal ==
#define BEQZ	beqz	//beqz Rsrc, label		--Branch on Equal Zero

#define BGE	bge	//bge Rsrc1, Src2, label	--Branch on Greater Than Equal >=
#define BGEU	bgeu	//bgeu Rsrc1, Src2, label	--Branch on GTE Unsigned

#define BGT	bgt	//bgt Rsrc1, Src2, label	--Branch on Greater Than >
#define BGTU	bgtu	//bgtu Rsrc1, Src2, label	--Branch on Greater Than  Unsigned

#define BLE	ble	//ble Rsrc1, Src2, label	--Branch on Less Than Equal <=
#define BLEU	bleu	//bleu Rsrc1, Src2, label	--Branch on LTE Unsigned

#define BLT	blt	//blt Rsrc1, Src2, label	--Branch on Less Than Equal <
#define BLTU	bltu	//bltu Rsrc1, Src2, label	--Branch on Less Than  Unsigned

#define BNE	bne	//bne Rsrc1, Src2, label	--Branch on Not Equal

#define J	j	//j label			-- Jump
#define JAL	jal	//jal label			-- Jump and Link, save the address of the next instruction in regiester 31, $ra
#define JALR	jalr	//jalr Rsrc			-- Jump and Link Register
#define JR 	jr	//jr Rsrc			-- Jump Register 

/* Load Instructions */
#define LI	li	//li Rdest, imm			--Load immediate
#define LA	la	//la Rdest, address		--Load Address
#define	LW	lw	//lw Rdest, address		--Load Word

/* Store Instructions */
#define SW	sw	//sw Rsrc address		--Sotre Word

/* Data Movement Instructions */
#define MOVE	move	//move Rest, Rst		--Move


/* Floating Pointer Instructions, not need for this project */

#define R0     0
#define R1     1
#define R2     2
#define R3     3
#define R4     4
#define R5     5
#define R6     6
#define R7     7
#define R8     8
#define R9     9
#define R10    10
#define R11    11
#define AP     12
#define FP     13
#define SP     14
#define PC     15

/* instruction definition */

/* general group */
#define ADD    21
#define SUB    22
#define MUL    23
#define DIV    24
#define AND    25
#define OR     26
#define MOV    27
#define MOVA   28
#define PUSH   29
#define PUSHA  30
#define CMP    31
#define TST    32

/* brach and jump group */
#define BEQL   40
#define BGEQ   41
#define BGTR   42
#define BLEQ   43
#define BLSS   44
#define BNEQ   45
#define JMP    46
