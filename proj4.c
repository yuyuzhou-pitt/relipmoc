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
  
emit_main()
{
    char s[256];
    new_code();
    sprintf(s, ".text\nmain:\n" \
        "\tla\t\$28\tbase\n" \
        "\tmove\t\$t1\t$28\n" \
        "\tadd\t\$t1\t\$t1\t0\n" \
        "\tli\t\$t2\t0\n" \
        "\tmove\t\$fp\t\$sp\n" \
        "\tsw\t\$ra\t0(\$sp)\n" \
        "\taddi\t\$sp\t\$sp\t-4");
    str_code(s);
    tab_code(1, strlen(s));
}

emit_end()
{
    char s[256];
    new_code();
    sprintf(s, "\tlw\t\$ra\t0(\$fp)\n" \
        "\tmove\t\$sp\t\$fp\n" \
        "\tjr\t\$ra" );
    str_code(s);
    tab_code(1, strlen(s));
}

emit_rodata(label_i, str)
int label_i;
char *str;
{
    char s[256];
    new_code();
    sprintf(s, ".data\n" \
        ".align 2\n" \
        "S_%d:\t.asciiz\t\"%s\"\n" \
        ".text", label_i, str );
    str_code(s);
    tab_code(1, strlen(s));
}

emit_println(label_i)
int label_i;
{
    char s[256];
    new_code();
    sprintf(s, "\tli\t\$11\t0\n" \
        "\tadd\t\$11\t\$11\t0\n" \
        "\tadd\t\$11\t\$11\t\$gp\n" \
        "\tmove\t\$25\t\$11\n" \
        "\tmove\t\$24\t\$t2\n" \
        "\tsw\t\$24\t0(\$sp)\n" \
        "\taddi\t\$sp\t\$sp\t-4\n" \
        "\tsw\t\$25\t0(\$sp)\n" \
        "\taddi\t\$sp\t\$sp\t-4\n" \
        "\tli\t\$v0\t4\n" \
        "\tla\t\$a0\tS_%d\n" \
        "\tsyscall", label_i );
    str_code(s);
    tab_code(1, strlen(s));
}

emit_header()
{
    char s[256];
    new_data();
    sprintf(s, ".data\n" \
        "Enter:\t.asciiz \"\n" \
        "\"\n" \
        "base:\n" \
        ".text\n" \
        ".globl main\n" \
        ".data\n");
    str_data(s);
    tab_data(2, strlen(s));
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
emit_jmp(operator, reg_s, l_num)
	/* emit unconditional and conditional jump instructions */
  int operator, l_num;
  char *reg_s;
{
  char s[32];

  new_code();
  tab_code(1, 1);
  switch (operator)
  {
    case BEQZ:
      str_code("beqz"); break;
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
      str_code("b"); break;
  }
  tab_code(1, 4);
  
  sprintf(s, "%s\tL_%d", reg_s, l_num);
  str_code(s);
  tab_code(3, strlen(s));
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
      str_code("b"); break;
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
  char *name, *type;
  int size;
{
  char temp[10];

  new_data();
  //str_data(name);
  //char_data(':');
  tab_data(1, strlen(name)+1);
  str_data(".");
  str_data(type);
  tab_data(1, 5);
  sprintf(temp, "%d", size);
  str_data(temp);
  tab_data(2, strlen(temp));
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
  str_data(".asciiz");
  tab_data(1, 6);
  strconst(str);
  tab_data(2, strlen(str));
}

emit_readfp(s)
  char *s;
{
  char s_addon[256] = "";
  sprintf(s_addon, "\tsub\t\$11\t\$fp\t\$11\n" \
      "\tlw\t%s\t(\$11)", s);
  new_code();
  str_code(s_addon);
  tab_code(1, strlen(s_addon));
}

emit_pushstack(s)
  char *s;
{
  char s_addon[256] = "";
  sprintf(s_addon, "\tsw\t%s\t0(\$sp)\n" \
      "\taddi\t$sp\t$sp\t-4", s);
  new_code();
  str_code(s_addon);
  tab_code(1, strlen(s_addon));
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
	str_code("add"); break;
    case SUB:
	str_code("sub"); break;
    case MUL:
	str_code("mul"); break;
    case DIV:
	str_code("div"); break;
    case AND:
	str_code("bic"); break;
    case OR:
	str_code("bis"); break;
    case MOV:
	str_code("mov"); break;
    case MOVA:
	str_code("mova"); break;
    case PUSH:
	str_code("push"); break;
    case PUSHA:
	str_code("pusha"); break;
    case CMP:
	str_code("cmp"); break;
    case TST:
	str_code("tst"); break;
    case LI:
	str_code("li"); break;
  }
  if (operator <= OR)
  {
    sprintf(s, "%d", num_op);
    char_code(*s);
  }
  tab_code(1, 6);

  len = print_op(op1);
  if (num_op > 1)
  {
    tab_code(1, len+1);
    len = print_op(op2);
    if (num_op > 2)
    {
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
    sprintf(s, "\$%d", reg_num);
    str_code(s);
    return strlen(s);
  }
  switch (reg_num)
  {
    case AP:
	str_code("ap");  break;
    case FP:
	str_code("fp");  break;
    case SP:
	str_code("sp");  break;
    case PC:
	str_code("pc");  break;
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

/* Check data exists or not */
int data_lookup(char *s)
{
  int i;
  int __start = 0;
  int __end = 0;
  for (i=0;i<DATA_MAX;i++) {
      if(*s == data[i]){
          int j;
          for(j=i+1;i<DATA_MAX;j++){
              s++;
              if (data[j] == ':'){
                  return i;
              }
              if (*s != data[j]){
                  break;
              }
          }
      }
  }
  return -1;
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
  //fprintf(stderr, "c=%d.\n", c);
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

