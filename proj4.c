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
 
   emiting functions: emit_call, _label, _goto, _most, _data, _str
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
  char s[10];
  int len;
 
  new_code();
  str_code("\tCALLS");
  tab_code(1, 5);
  sprintf(s, "#%d,", num_arg);
  str_code(s);
  tab_code(1, strlen(s));
  str_code(func_name);
  tab_code(2, strlen(func_name));
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
  
emit_entry(name)
char *name;
{
    char s[256];

    new_code();
    sprintf(s, "\t.ENTRY %s, 0", name);
    str_code(s);
    tab_code(1, strlen(s));
}

emit_idiot(s)
char *s;
{
    new_code();
    str_code(s);
    tab_code(1, strlen(s));
}


/*********************************************************************/
emit_goto(operator, l_num)
	/* emit unconditional and conditional jump instructions */
  int operator, l_num;
{
  char s[10];

  new_code();
  tab_code(1, 1);
  switch (operator)
  {
    case BEQL:
      str_code("BEQL"); break;
    case BGEQ:
      str_code("BGEQ"); break;
    case BGTR:
      str_code("BGTR"); break;
    case BLEQ:
      str_code("BLEQ"); break;
    case BLSS:
      str_code("BLSS"); break;
    case BNEQ:
      str_code("BNEQ"); break;
    case JMP:
      str_code("JMP"); break;
  }
  tab_code(1, 4);
  
  sprintf(s, "L_%d", l_num);
  str_code(s);
  tab_code(3, strlen(s));
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
  char temp[10];

  new_data();
  str_data(name);
  char_data(':');
  //tab_data(1, strlen(name)+1);
  str_data(".BLK");
  char_data(toupper(type));
  //tab_data(1, 5);
  sprintf(temp, "%d", size);
  str_data(temp);
  //tab_data(2, strlen(temp));
}

/**************************************************************/
emit_str(name, str)
  char *name, *str;
{
  char *s;

  new_data();
  str_data(name);
  char_data(':');
  tab_data(1, strlen(name)+1);
  str_data(".ASCII");
  tab_data(1, 6);
  strconst(str);
  tab_data(2, strlen(str));
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
  char s[20];
  int len;

  new_code();
  char_code('\t');
  switch (operator)
  {
    case ADD:
	str_code("ADD"); break;
    case SUB:
	str_code("SUB"); break;
    case MUL:
	str_code("MUL"); break;
    case DIV:
	str_code("DIV"); break;
    case AND:
	str_code("BIC"); break;
    case OR:
	str_code("BIS"); break;
    case MOV:
	str_code("MOV"); break;
    case MOVA:
	str_code("MOVA"); break;
    case PUSH:
	str_code("PUSH"); break;
    case PUSHA:
	str_code("PUSHA"); break;
    case CMP:
	str_code("CMP"); break;
    case TST:
	str_code("TST"); break;
  }
  char_code(toupper(type));
  if (operator <= OR)
  {
    sprintf(s, "%d", num_op);
    char_code(*s);
  }
  tab_code(1, 6);

  len = print_op(op1);
  if (num_op > 1)
  {
    char_code(',');
    tab_code(1, len+1);
    len = print_op(op2);
    if (num_op > 2)
    {
      char_code(',');
      tab_code(1, len+1);
      len = print_op(op3);
    }
  }
  tab_code(3-num_op, len);
}
      
/****************************************************************/
int print_op(op)
  	/* print an operand */
  OPERAND op;
{
  int len;
  char s[20];

  switch (op.mode)
  {
    case NUM_CONST:
	sprintf(s, "#%d", op.num_const);
   	str_code(s);
	return strlen(s);
    case CHAR_CONST:
	str_code("#^A/");
	char_code(op.char_const);
	char_code('/');
	return 6;
    case IDENTIFIER:
	str_code(op.ident);
	return strlen(op.ident);
    case REGISTER:
	len = print_reg(op.reg);
	return len;
    case REG_DEFER:
	char_code('(');
	len = print_reg(op.reg);
	char_code(')');
	return len+2;
    case REG_DISPL:
	sprintf(s, "%d(", op.num_const); 
        str_code(s);
	len = strlen(s);
  	len = len + print_reg(op.reg);
	char_code(')');
	return len+1;
  }
}

/**************************************************************/
int print_reg(reg_num)
  int reg_num;
	/* print register to code array */
{
  char s[10];

  if (reg_num <= 11)
  {
    sprintf(s, "R%d", reg_num);
    str_code(s);
    return strlen(s);
  }
  switch (reg_num)
  {
    case AP:
	str_code("AP");  break;
    case FP:
	str_code("FP");  break;
    case SP:
	str_code("SP");  break;
    case PC:
	str_code("PC");  break;
  }
  return 2;
}

/**********************************************************/
new_code()
	/* start a new line of code */
{
  if (cl >= CODE_LINE_MAX) dump("Too many lines of code!\n");
  if (c >= CODE_MAX) dump("new_code: Too many chars of code!\n");
  code[c++] = '\n';
	/* put a <return> at the end of the last line */
  code_line[cl++] = c;
}

generate_code(name)
char *name;
{
  FILE *fp;

  if ((fp = fopen(name, "w")) == NULL) {
    printf("Can't open file %s to output generated asm code\n", name);
    exit(5);
  }
  data[d] = '\0';
  code[c] = '\0';
  fprintf(fp, "%s\n%s\n", data, code);
  fclose(fp);
  printf("Assembly code is generated in file \"%s\"\n", name);
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
  exit(0);
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
  if (len + c + 1 > CODE_MAX) 
      dump("str_code: Too many chars of code!\n");
  strcpy(&code[c], s);
  c = c + len;
  fprintf(stderr, "c=%d.\n", c);
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
  if (c >= CODE_MAX) dump("char_code: Too many chars of code!\n");
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
  if (t + c > CODE_MAX) dump("tab_code: Too many chars of code!\n");
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
  str_data(" ; ");
  str_data(s);
}

/**************************************************************/
comment_code(s)
	/* put comment into code */
  char *s;
{
  str_code(" ; ");
  str_code(s);
}

/**************************************************************/
strconst(s)
  char *s;
{
  char c;
  char flag=0;
  static char ret [100];
  char *ptr = ret;

  for (c = '/'; isprint (c); c++) /* determine the delimiting character */
  {
    if (!index (s, c) && c != '=' && c != ';' && c != '<') break;
  }

  if (isprint (*s))
  {
    *ptr++ = c;			/* Delimit start 			*/
    flag = 1;
  }

  while (*s)
  {
    if (isprint (*s))		/* If character,...			*/
    {
      if (!flag)
      {
	*ptr++ = c;
	flag = 1;
      }
      *ptr++ = *s++ ;		/* print it.				*/
    }
    else
    {
      if (flag)
      {
	*ptr++ = c;
	flag = 0;
      }
      sprintf (ptr, "<%d>", *s++);	/* ..as <number>	*/
      while (*ptr)
	ptr++;
    }
  }
  if (flag)
    *ptr++ = c;
  *ptr  = 0;
  str_data(ret);
  tab_data(2, strlen(ret));
}

