#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "proj4.h"

/******************************************************************/
/* emit functions in project 4.
   Data definitions implemented:
	--data block definition (name, type, size)
	--string constant definition
   Instructions implemented:
 	add, sub, mul, div, and, or, mov, mova, push, pusha, cmp, tst
	beql, bgeq, bgtr, bleq, blss, bneq, jmp, calls
   Addressing modes implemented:
	relative(identifier), number constant, char constant, register
	register deferred, register display
 
   emiting functions: emit_call, emit_label, emit_goto, emit_most, emit_data, emit_str
*/
	
int data_line [DATA_LINE_MAX], code_line [CODE_LINE_MAX];
	/* arrays to store the beginning of lines */
char data [DATA_MAX], code [CODE_MAX];
	/* arrays to store data definition and code */

int dl=0, cl=0, d=0, c=0;
	/* counters of lines and chars */

/*********************************************************************/
emit_call(func_name, num_arg)
 	/* emit a calls instruction */
  char *func_name;
  int num_arg;
{

}

/*********************************************************************/
emit_label(l_num)
	/* emit a definition of label.
	  l_num = label number.
	  example: l_num = 102, code = "L_102"
	*/
  int l_num;
{
  char s[10];
  
  new_code();
  sprintf(s, "L_%d:", l_num);
  str_code(s);
  tab_code(1, strlen(s));
}

/* this is my function emit entry */

emit_entry(name)
char *name;
{


}

emit_idiot(s)
char *s;
{

}
  
/*********************************************************************/
emit_goto(operator, l_num)
	/* emit unconditional and conditional jump instructions */
  int operator, l_num;
{

}

/**************************************************************/
emit_data(name, type, size)
	/* emit one data line, name = data object name
				type = L/l, W/w or B/b
				size = # of elements
	 */
  char *name, type;
  int size;
{

}

/**************************************************************/
emit_str(name, str)
  char *name, *str;
{
}

/*****************************************************************/
emit_most(operator, type, num_op, op1, op2, op3)
	/* operator: one of the instructions in the general group 
	  type = L/l, W/w or B/b
	  num_op = 1, 2 or 3
	  op1..op3: operands, op2 and/or op3 can be omitted according to num_op
	*/
  int operator, num_op;
  char type;
  OPERAND op1, op2, op3;
{

}
      
/****************************************************************/
int print_op(op)
  	/* print an operand */
  OPERAND op;
{

}

/**************************************************************/
int print_reg(reg_num)
  int reg_num;
	/* print register to code array */
{

}

/**********************************************************/
new_code()
	/* start a new line of code */
{

}

/**************************************************************/
generate_code(name)
char *name;
{

}    

/**************************************************************/
dump(error)
	/* print error message and dump data and code when an error occurs */
  char *error;
{
  FILE *fp;

  printf("%s", error);
  fp = fopen("codedump", "w");
  data[d] = '\0';
  code[c] = '\0';
  fprintf(fp, "%s\n%s\n", data, code);
  fclose(fp);
  exit();
}    

/**************************************************************/
new_data()
	/* start a new line of data */
{
  if (dl >= DATA_LINE_MAX) dump("Too many lines of data!\n");
  if (d >= DATA_MAX) dump("Too many chars of data!\n");
  data[d++] = '\n';
        /* put a <return> at the end of the last line */
  data_line[dl++] = d;
}

/**************************************************************/
str_data(s)
	/* copy a string into data line */
  char *s;
{
  int len;
  
  len = strlen(s);
  if (len + d + 1 > DATA_MAX) dump("Too many chars of data!\n");
  strcpy(&data[d], s);
  d = d + len;
}

/**************************************************************/
str_code(s)
	/* copy a string into code line */
  char *s;
{
  int len;
  
  len = strlen(s);
  if (len + c + 1 > CODE_MAX) dump("Too many chars of code!\n");
  strcpy(&code[c], s);
  c = c + len;
}

/**************************************************************/
char_data(c)
	/* copy a string into data line */
  char c;
{
  if (d >= DATA_MAX) dump("Too many chars of data!\n");
  data[d++] = c;
}

/**************************************************************/
char_code(ch)
	/* copy a string into code line */
  char ch;
{
  if (c >= CODE_MAX) dump("Too many chars of code!\n");
  code[c++] = ch;
}

/**************************************************************/
tab_code(n_tab, n_char)
	/* feed proper number of tabs to justify the line 
				n_tab = expected width 
				n_char = actual # of chars already there
	*/
  int n_tab, n_char;
{
  int i, t;

  if (n_char >= 8 * n_tab) return;
  t = n_tab - n_char / 8;
  if (t + c > CODE_MAX) dump("Too many chars of code!\n");
  for (i=0; i<t; i++) code[c++] = '\t';
} 

/**************************************************************/
tab_data(n_tab, n_char)
	/* feed proper number of tabs to justify the line 
				n_tab = expected width 
				n_char = actual # of chars already there
	*/
  int n_tab, n_char;
{
  int i, t;

  if (n_char >= 8 * n_tab) return;
  t = n_tab - n_char / 8;
  if (t + d > DATA_MAX) dump("Too many chars of data!\n");
  for (i=0; i<t; i++) data[d++] = '\t';
} 

/**************************************************************/
comment_data(s)
	/* put comment into data def arrray */
  char *s;
{
  str_data(" # ");
  str_data(s);
}

/**************************************************************/
comment_code(s)
	/* put comment into code */
  char *s;
{
  str_code(" ; ");
  str_code(#);
}

