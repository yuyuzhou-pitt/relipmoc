#include <stdio.h>
#include "proj4.h"
#include "proj2.h"
#include "proj3.h"

#define CHAR_TYPE 0
#define INTEGER_TYPE 1
#define STRING_TYPE  2
#define LABELSTACKSIZE 100

extern tree Root;
int offset = 0;
int LabelNum = 0;
int GlobalDataSize;

char VarOffset[200];
char ExpValue[200];
OPERAND op1, op2, op3;
int CurrentTemp=0;
int MaxTemp=(-1);
int StringNum;
int LabelStack[LABELSTACKSIZE];
int LabelStackTop= 0;

extern tree NullExp();
extern tree intTypeT;
extern tree charTypeT;
extern tree booleanTypeT;
extern tree stringTypeT;
extern tree LeftChild();
extern tree RightChild();
extern char *getname(); 
extern int traceGen;		/* trace execution or not */

/* label stack management */
PushLabel(label)
int label;
{

}

int PopLabel()
{
}

int TopLabel()
{
}

/* calculate array upper bound */
UpperBound(T)
tree T;
{
}

/* calculate array lower bound */
LowerBound(T)
tree T;
{
}


/* address calculation */
TypeSize(T)
tree T;
{  
}

/* type offset used for record type; use in doubt */
TypeOffset(T)
tree T;
{
}

    

/* argument and local variable offset */
ArgVarOffset(T)
tree T;  /* T pointing to proceop or funcop */
{
}

/* calculate variable offset */
DataOffset()
{
}
  

/* code generation */

tree GetVarType(T)
tree T;               /* pointing to formal parameter */
{

}

/* find out type for the parameter */
int GetType(T)
tree T;
{
}

GenBlockAssign(dest, source, size)
char *dest, *source;
int size;
{
}

GenBlockPush(source, size)
char *source;
int size;
{
} 
     
int GenPushArg(T, type)
tree T, type;
{
}

GenSingleStmt(T)
tree T;
{
}

GenStmts(T)
{

}

GenFun(T)
tree T;
{

}

GenAll()   /* generate codes */
{

}

int GetConstValue(s)
char *s;
{
}

GenValueExp(T)
tree T;
{

}


/* get the lower bound for bound checking and code gen */
int GetLowerBound(T, index)
tree T;
int index;
{
}

/* get upper bound for a certain index */
int GetUpperBound(T, index)
tree T;
int index;
{
}
    
GenVarAddr(T)
tree T;
{
}


/* gen if statement */
int GenIfStmt(T)
tree T;
{
}

/* generated code for loop statement */
GenLoopStmt(T)
tree T;
{
}
