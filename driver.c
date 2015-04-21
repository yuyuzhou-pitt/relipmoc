#include "proj2.h"
#include "proj3.h"

#include <stdio.h>

int outtree = 1;          /* flag -- print out syntax tree or not? */
int outtable = 1;         /* flag -- print out symbol table or not? */
int trace = 1;            /* flag -- trace parser */
int traceST = 1;          /* flag -- trace semantic analysis */
int traceGen = 1;         /* flag -- trace semantic analysis */
                          /* name of the generated asm file */
extern void init_table(); /* hash table initialization, imported from table.c */
extern int error_count;   /* # of semantic errors found */

extern int loc_str();
extern void yyparse();
extern int DataOffset(); /* code generation routines, imported from gen.c */
extern int GenAll();
extern int dump();	/*output code to a file, imported from proj4.c */

extern int st_top;

/* Root of the syntax tree */
extern tree  SyntaxTree;
/* printtree output direction */
FILE *treelst;	/* file used to save the syntx tree */
FILE *tablelst;           /* file used to save the symbol table */

char codeName[20] = {"code.s"}; /* name of the gereated asm file*/

int main()
{
  SyntaxTree = NULL;
  yyparse(); /* make syntax tree */

  if ( SyntaxTree == NULL )
    {
      printf( "Syntax Tree not created, exiting...\n" );
      exit(1);
    }
  treelst = stdout;

  STInit();
  MkST(SyntaxTree);
  DataOffset();
  STPrint();
  GenAll();
  //GenAsm(SyntaxTree, 0);
  generate_code(codeName);
  STPrint();

  printtree(SyntaxTree, 0);
}


