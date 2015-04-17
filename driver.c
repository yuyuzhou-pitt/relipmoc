#include "proj2.h"
#include "proj3.h"

#include <stdio.h>

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
  GenAll();
  generate_code(codeName);
  STPrint();

  printtree(SyntaxTree, 0);
}


