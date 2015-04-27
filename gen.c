#include <stdio.h>
#include "proj4.h"
#include "proj2.h"
#include "proj3.h"

#define CHAR_TYPE 0
#define INTEGER_TYPE 1
#define STRING_TYPE  2
#define LABELSTACKSIZE 100

extern tree SyntaxTree;
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
{  if (traceGen) printf("Entering PushLabel\n");
    if (LabelStackTop >= LABELSTACKSIZE) {
        printf("label stack overflow\n");
        exit(0);
    }
    LabelStack[LabelStackTop++] = label;
  if (traceGen) printf("Leaving PushLabel\n");
}

int PopLabel()
{  if (traceGen) printf("Entering PopLabel\n");
    if (LabelStackTop <= 0) {
        printf("label stack underflow\n");
        exit(0);
    }
    LabelStackTop--;
if (traceGen) printf("Leaving PopLabel\n"); 
    return(LabelStack[LabelStackTop]);
}

int TopLabel()
{  if (traceGen) printf("Entering TopLabel\n");
if (traceGen) printf("Leaving TopLabel\n"); 
    return(LabelStack[LabelStackTop-1]);
}

/* calculate array upper bound */
UpperBound(T)
tree T;
{  if (traceGen) printf("Entering UpperBound\n");
    if (NodeKind(RightChild(T)) == NUMNode) {
if (traceGen) printf("Leaving UpperBound\n");
         return(IntVal(RightChild(T)));
    } else {
if (traceGen) printf("Leaving UpperBound\n");
    return(GetAttr(IntVal(RightChild(T)), VALUE_ATTR));
    }
}

/* calculate array lower bound */
LowerBound(T)
tree T;
{  if (traceGen) printf("Entering LowerBound\n");
    if (NodeKind(LeftChild(T)) == NUMNode) {
if (traceGen) printf("Leaving LowerBound\n");
        return(IntVal(LeftChild(T)));
    } else {
if (traceGen) printf("Leaving LowerBound\n");
    return(GetAttr(IntVal(LeftChild(T)), VALUE_ATTR));
    }
}


/* address calculation
  | |   |     | | +-[DUMMYnode]
  | |   |     | +-[TypeIdOp]
  | |   |     |   +-[INTEGERTNode]
*/
TypeSize(T)
tree T;
{  
    int i,j,k;
    tree p, p1;
    int size, kind, op;
if (traceGen) printf("Entering TypeSize\n");
    kind = NodeKind(LeftChild(T));
    op = NodeOp(LeftChild(T));
    switch (kind) {
      case INTEGERTNode:
	return 4;
	break;
      case CHARTNode:
	return 1;
	break;
      case BOOLEANTNode:
	return 1;
	break;
      case EXPRNode:
        switch (op) {
          case SubrangeOp:
	    return 4;
	    break;
          case ArrayTypeOp:
            size = 1;
	    p = LeftChild(T);
            while (p!= NullExp()) {
                size = size * (UpperBound(RightChild(p)) -
                               LowerBound(RightChild(p)) +1);
                p = LeftChild(p);
	    }
            return(size * TypeSize(RightChild(T)));
	    break;
          case RecompOp:
            size = 0;
            p = T;
            while (p!=NullExp()) {
                int temp;
                temp = TypeSize(RightChild(RightChild(p)));
                if (temp >1) 
                   if (size %4 != 0) size = size /4 *4 + 4;
                size = size + temp;
                p = LeftChild(p);
	    }
            return(size);
	    break;
	  default:
            printf("DEBUG -- wrong NodeOp %d in TypeSize()\n", op);
            exit(0);
        }
	break;
      default:
        printf("DEBUG -- wrong NodeKind %d in TypeSize()\n", kind);
        exit(0);
    }
if (traceGen) printf("Leaving TypeSize\n"); }

/* type offset used for record type; use in doubt */
TypeOffset(T)
tree T;
{
    tree p,p1;
    int offset;
  if (traceGen) printf("Entering TypeOffset\n");
    if (NodeKind(T) != EXPRNode) return(0);
    else if (NodeOp(T) == ArrayTypeOp) 
          TypeOffset(RightChild(T));
    else if (NodeOp(T) == RecompOp) {
        p = T;
        offset = 0;
        while (p != NullExp()) {
            int temp;
            temp = TypeSize(RightChild(RightChild(p)));
            if (temp > 1)
               if (offset % 4 != 0) offset = offset /4*4+4;
            if (!IsAttr(IntVal(LeftChild(RightChild(p))), OFFSET_ATTR)) 
                SetAttr(IntVal(LeftChild(RightChild(p))), OFFSET_ATTR, offset);
            TypeOffset(RightChild(RightChild(p)));
            offset += temp;
            p = LeftChild(p);
	}
    }
if (traceGen) printf("Leaving TypeOffset\n"); }

    

/* argument and local variable offset */
ArgVarOffset(T)
tree T;  /* T pointing to proceop or funcop */
{

    tree specs, body;
    tree p,p1;
    int offset;
    int lastsize;
   if (traceGen) printf("Entering ArgVarOffset\n");
    /* calculate argument offset */

    specs = RightChild(LeftChild(T));
    p = LeftChild(specs);
    offset = 4;
    while (p !=NullExp()) {
        int temp;
        if (NodeOp(p) == VArgTypeOp) {
            temp = TypeSize(GetAttr(IntVal(LeftChild(LeftChild(p))), 
                                   TYPE_ATTR));
            if (temp < 4) temp = 4;
	}
        else temp = 4;

        if ((offset % 4) != 0) offset = (offset / 4) * 4 +4;
        if (NodeOp(p) == VArgTypeOp)
          SetAttr(IntVal(LeftChild(LeftChild(p))), PLACE_ATTR, VARGUE);
        else 
          SetAttr(IntVal(LeftChild(LeftChild(p))), PLACE_ATTR, RARGUE);
        SetAttr(IntVal(LeftChild(LeftChild(p))), OFFSET_ATTR, offset);
        TypeOffset(GetAttr(IntVal(LeftChild(LeftChild(p))), TYPE_ATTR));
        offset += temp; 
        p = RightChild(p);
    }
    
    if (offset % 4 == 0) offset = offset - 4;
    SetAttr(IntVal(LeftChild(LeftChild(T))), VALUE_ATTR, offset / 4);
    offset = 0;
    /* offset equal to local */
    if (RightChild(T) == NullExp()) return(0);
    body = LeftChild(RightChild(T));
    while (body != NullExp()) {
        p = RightChild(body);
        if (NodeOp(p) == DeclOp) {
            while (p!=NullExp()) {
                int temp;
                temp = TypeSize(GetAttr(IntVal(LeftChild(RightChild(p))), 
                                   TYPE_ATTR));
                if (temp > 1) 
                    if ((offset % 4) != 0) offset = (offset /4) *4 +4;
                offset += temp; 
                SetAttr(IntVal(LeftChild(RightChild(p))), PLACE_ATTR, 
                        LOCAL);
                SetAttr(IntVal(LeftChild(RightChild(p))), OFFSET_ATTR, 
                                                          -offset);
                TypeOffset(GetAttr(IntVal(LeftChild(RightChild(p))), 
                           TYPE_ATTR));
                p = LeftChild(p);
	    }
	}
        body = LeftChild(body);
    }
    SetAttr(IntVal(LeftChild(LeftChild(T))), OFFSET_ATTR, offset);
if (traceGen) printf("Leaving ArgVarOffset\n"); }

/* The DelcOp node
  | |   |         +-[DUMMYnode]
  | |   |       +-[UnaryNegOp]
  | |   |       | +-[NUMNode,1]
  | |   |     +-[CommaOp]
  | |   |     | | +-[DUMMYnode]
  | |   |     | +-[TypeIdOp]
  | |   |     |   +-[INTEGERTNode]
  | |   |   +-[CommaOp]
  | |   |   | +-[STNode,5,"x"]
  | |   | +-[DeclOp]
  | |   | | +-[DUMMYnode]
*/
int CalcDataOffset(T, offset)
tree T;
int offset;
{  

    tree p,p1;
   if (traceGen) printf("Entering CalcDataOffset\n");
    if (T==NullExp()) return;


    CalcDataOffset(LeftChild(T), offset);
    p = LeftChild(T);
    while (p != NullExp()) {
        p1 = RightChild(p);
        if (NodeOp(p1) == DeclOp) {
            while (p1!=NullExp()) {
                int temp;
                int intval = IntVal(LeftChild(RightChild(p1)));
                temp = TypeSize(GetAttr(intval, TYPE_ATTR));
                if (temp > 1)
                   if ((offset % 4) != 0) offset = (offset / 4) *4 +4;
                SetAttr(intval, OFFSET_ATTR, offset);
                SetAttr(intval, PLACE_ATTR, GLOBAL);
                TypeOffset(GetAttr(intval, TYPE_ATTR));
                offset += temp;
                p1 = LeftChild(p1);
            }
        } else if ((NodeOp(p1) == ProceOp) || (NodeOp(p1) == FuncOp))
            ArgVarOffset(p1);
        p = LeftChild(p);

    }

    CalcDataOffset(RightChild (T), offset);

if (traceGen) printf("Leaving CalcDataOffset\n"); 
    return offset;
}


/* calculate variable offset */
DataOffset()
{
    int offset;
    tree p,p1;
   if (traceGen) printf("Entering DataOffset\n");
    offset = 0;
    GlobalDataSize = CalcDataOffset(SyntaxTree, offset);

if (traceGen) printf("Leaving DataOffset\n"); }
  

/* code generation */

tree GetVarType(T)
tree T;               /* pointing to formal parameter */
{
    tree p;
    tree pp1,pp2,pp3, pp4,pp5;
  if (traceGen) printf("Entering GetVarType\n");
    if (NodeKind(T) == EXPRNode) {
       if (NodeOp(T) == VarOp) {
          p = LeftChild(T);
          if (!IsAttr(IntVal(p), TYPE_ATTR)) {
                printf("not a type\n");return(false);}
          pp1 = RightChild(T);
          pp3 = (tree)GetAttr(IntVal(p), TYPE_ATTR);
          while ( pp1 != NullExp()) {
              if (NodeKind(pp3) != EXPRNode) {
                  return(pp3);
                  //printf("DEBUG!!!!!\n");
                  //exit(0);
	      }
              if (NodeOp(pp3) == ArrayTypeOp) 
                  pp2 = RightChild(pp3);
              else if ((NodeOp(pp3) == RecompOp) &&
                       (NodeOp(LeftChild(pp1)) == FieldOp)) {
                  int ii;
                  
                  ii=1;
                  pp4 = pp3;
                  while ((pp4 != NullExp()) && ii) {
                     pp5 = LeftChild(RightChild(pp4));
                     if (strcmp(getname(GetAttr(IntVal(pp5), NAME_ATTR)),
                                getname(GetAttr(IntVal(LeftChild(LeftChild
                                        (pp1))), NAME_ATTR))) == 0) {
                       ii=0;
                       pp2 = RightChild(RightChild(pp4));
                     }
                     pp4 = LeftChild(pp4);
		 }
                 if (ii) {
                     printf("DEBUG!!!!!\n");
                     exit(0);
		 }
	      }
              else {printf("DEBUG!!!!!!\n"); exit(0);}
              pp3 = pp2;
              pp1 = RightChild(pp1);
	  }
          return(pp3);
       }
    }
    printf("DEBUG--not suppose to reach here\n");
    exit(0);
if (traceGen) printf("Leaving GetVarType\n"); }


/* find out type for the parameter */
int GetType(T)
tree T;
{
    tree p;
    tree p1;  
    if (traceGen) printf("Entering GetType\n");
    if (T == NullExp()) return(INTEGER_TYPE);
    if (T == intTypeT) return(INTEGER_TYPE);
    if (T == charTypeT) return(CHAR_TYPE);
    if (NodeKind(T) == CHARNode) return(CHAR_TYPE);
    if (NodeKind(T) == NUMNode) return(INTEGER_TYPE);
    if (NodeKind(T) == STRINGNode) return(STRING_TYPE);
    if (NodeKind(T) == STNode) {
        if (strcmp(getname(GetAttr(IntVal(T), NAME_ATTR)), "CHAR") == 0)
            return(CHAR_TYPE);
        else if (strcmp(getname(GetAttr(IntVal(T), NAME_ATTR)), 
                        "INTEGER") == 0)
            return(INTEGER_TYPE);
        else {
            printf("DEBUG--not any more \n");
            return(INTEGER_TYPE);
	}
    }
    if (NodeKind(T) != EXPRNode) {
        printf("DEBUG;;;;;;;;;;;\n");
        printtree(T, 0);
        exit(0);
    }
    if (NodeOp(T) == RoutineCallOp) {
        p = (tree) GetAttr(IntVal(LeftChild(T)), TYPE_ATTR);
        return(GetType(RightChild(RightChild(LeftChild(p)))));
    }
    if (NodeOp(T) != VarOp) return(INTEGER_TYPE);

    p = (tree)GetAttr(IntVal(LeftChild(T)), TYPE_ATTR);
    if (RightChild(T) == NullExp()) {
       if (p == charTypeT) return(CHAR_TYPE);
       else if (p== intTypeT) return(INTEGER_TYPE);
       else if (NodeOp(p) == SubrangeOp) return(INTEGER_TYPE);
       else return(STRING_TYPE);   /* maybe changed here */
    } else {
      p1 = RightChild(T);
      while (p1 != NullExp()) {
          if (NodeOp(LeftChild(p1)) == IndexOp) 
              p = RightChild(p);
          else if (NodeOp(LeftChild(p1)) == FieldOp) 
              p = (tree)GetAttr(IntVal(LeftChild(LeftChild(p1))), TYPE_ATTR);
          p1 = RightChild(p1);
      }
      if (p == charTypeT) {
	  if (traceGen) printf("Leaving GetType\n");
	  return(CHAR_TYPE);
      } else if (p == intTypeT) {
	  if (traceGen) printf("Leaving GetType\n");
	  return(INTEGER_TYPE);
      } else {
	  if (traceGen) printf("Leaving GetType\n");
	  return(STRING_TYPE);
      }
   } 
}

GenBlockAssign(dest, source, size)
char *dest, *source;
int size;
{
    char temp[20], temp1[20], temp2[20];

  if (traceGen) printf("Entering GenBlockAssign\n");
     op1.mode = NUM_CONST;
     op1.num_const = 0;

     if (CurrentTemp < 10) {
         op2.mode = REGISTER;
         op2.reg = CurrentTemp+2;
         sprintf(temp, "$%d", CurrentTemp+2);
     } else {
         op2.mode = IDENTIFIER;
         sprintf(temp, "$%d", CurrentTemp+2);
         op2.ident = temp;
     }

     CurrentTemp++;
     if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
         MaxTemp = CurrentTemp+1;
         sprintf(temp, "$%d", MaxTemp+1);
         emit_data(temp, 'l', 1);
    }	
    emit_most(MOV, 'l', 2,  op1, op2, op3);
    emit_label(LabelNum++);

    if (*dest == 'R') sprintf(temp1, "(%s)", dest);
    else sprintf(temp1, "@%s", dest);

    if (*source == 'R') sprintf(temp2, "(%s)", source);
    else sprintf(temp2, "@%s", source);
 
    op1.mode = IDENTIFIER;
    op1.ident = temp2;

    op2.mode = IDENTIFIER;
    op2.ident = temp1;
    emit_most(MOV, 'b', 2, op1, op2, op3);

    op1.mode = NUM_CONST;
    op1.num_const = 1;
    op2.mode = IDENTIFIER;
    op2.ident = dest;
    emit_most(ADD, 'l', 2, op1, op2, op3);

    op1.mode = NUM_CONST;
    op1.num_const = 1;
    op2.mode = IDENTIFIER;
    op2.ident = source;
    emit_most(ADD, 'l', 2, op1, op2, op3);

    op1.mode = NUM_CONST;
    op1.num_const = 1;
    op2.mode = IDENTIFIER;
    op2.ident = temp;
    emit_most(ADD, 'l', 2, op1, op2, op3);

    op1.mode = IDENTIFIER;
    op1.ident = temp;
    op2.mode = NUM_CONST;
    op2.num_const = size;
    emit_most(CMP, 'l', 2, op1, op2, op3);
 
    emit_goto(BGEQ, LabelNum++);
    emit_goto(JMP, LabelNum -2);
    emit_label(LabelNum-1);
    
    CurrentTemp--;
if (traceGen) printf("Leaving GenBlockAssign\n"); }

GenBlockPush(source, size)
char *source;
int size;
{
    char temp[20], temp1[20], temp2[20];

  if (traceGen) printf("Entering GenBlockPush\n");
     op1.mode = NUM_CONST;
     op1.num_const = 0;

     if (CurrentTemp < 10) {
         op2.mode = REGISTER;
         op2.reg = CurrentTemp+2;
         sprintf(temp, "\$%d", CurrentTemp+2);
     } else {
         op2.mode = IDENTIFIER;
         sprintf(temp, "\$%d", CurrentTemp+2);
         op2.ident = temp;
     }

     CurrentTemp++;
     if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
         MaxTemp = CurrentTemp+1;
         sprintf(temp, "$%d", MaxTemp+1);
         emit_data(temp, 'l', 1);
    }	
    emit_most(MOV, 'l', 2,  op1, op2, op3);

    op1.mode = NUM_CONST;
    op1.num_const = size*4-1;
    op2.mode = IDENTIFIER;
    op2.ident = source;
    op3.mode = IDENTIFIER;
    op3.ident = source;
    emit_most(ADD, 'l', 3, op1, op2, op3);

    if (*source == 'R') sprintf(temp1, "\tMOVB\t(%s),\t-(SP)", source);
    else sprintf(temp1, "@%s", source);
 
    op1.mode = IDENTIFIER;
    op1.ident = temp1;

    emit_label(LabelNum++);
    emit_idiot(temp1);

    op1.mode = NUM_CONST;
    op1.num_const = 1;
    op2.mode = IDENTIFIER;
    op2.ident = source;
    emit_most(SUB, 'l', 2, op1, op2, op3);

    op1.mode = NUM_CONST;
    op1.num_const = 1;
    op2.mode = IDENTIFIER;
    op2.ident = temp;
    emit_most(ADD, 'l', 2, op1, op2, op3);

    op1.mode = IDENTIFIER;
    op1.ident = temp;
    op2.mode = NUM_CONST;
    op2.num_const = size*4;
    emit_most(CMP, 'l', 2, op1, op2, op3);
 
    emit_goto(BGEQ, LabelNum++);
    emit_goto(JMP, LabelNum -2);
    emit_label(LabelNum-1);
    
    CurrentTemp--;
if (traceGen) printf("Leaving GenBlockPush\n"); }
    
     
int GenPushArg(T, type)
tree T, type;
{
    tree p, p1;
    int i;
    int return_value;
    int size;
    char temp[20], temp1[20];
  if (traceGen) printf("Entering GenPushArg\n");   
    if (T==NullExp()) {
  if (traceGen) printf("Leaving GenPushArg\n");   
      return(0);
    }
    return_value = GenPushArg(RightChild(T), RightChild(type));
    if (NodeOp(type) == RArgTypeOp) {
       GenVarAddr(LeftChild(T));
       op1.mode = IDENTIFIER;
       op1.ident = VarOffset;
       if ((VarOffset[0] =='T') ||
           (VarOffset[0] == 'R')){
           emit_most(PUSH, 'l', 1, op1, op2, op3);
           CurrentTemp--;
       }
       else 
           emit_most(PUSHA, 'l', 1, op1, op2, op3);
     } else { /* var type */
           p1 = (tree) GetAttr(IntVal(LeftChild(LeftChild(type))), TYPE_ATTR);
           i = GetType(LeftChild(T));
           if ((i==INTEGER_TYPE) || (i==CHAR_TYPE)){
               GenValueExp(LeftChild(T));
               op1.mode = IDENTIFIER;
               op1.ident = ExpValue;
               emit_most(PUSH, 'l', 1, op1, op2, op3);
               if ((ExpValue[0]=='R') ||
                   (ExpValue[0]=='T') ||
                   (ExpValue[0]=='(') ||
                   (ExpValue[0]=='@')) 
                   CurrentTemp--;
	   } else {
              if ((NodeKind(LeftChild(T)) != STRINGNode) &&
                   ((NodeKind(LeftChild(T)) != EXPRNode)
                    || (NodeOp(LeftChild(T)) != VarOp))){
                  printf("SEMANTIC ERROR, TYPE incompatibal\n");
                  exit(0);
 	     }
             GenVarAddr(LeftChild(T));
             if ((VarOffset[0] != 'R') &&
                  (VarOffset[0] != 'T')) {
                 op1.mode = IDENTIFIER;
                 op1.ident = VarOffset;
                 if (CurrentTemp < 10) {
                     op2.mode = REGISTER;
                     op2.reg = CurrentTemp+2;
                     sprintf(temp1, "\$%d", CurrentTemp+2);
                 } else {
                     op2.mode = IDENTIFIER;
                     sprintf(temp1, "$%d", CurrentTemp+2);
                     op2.ident = temp1;
                 }
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
                 emit_most(MOVA, 'l', 2,  op1, op2, op3);
	    }
            else strcpy(temp1, VarOffset);
            size = TypeSize(p1);
            if (size % 4 != 0) size = size /4 * 4 + 4;
            GenBlockPush(temp1, size / 4);
           if ((temp1[0] == 'R') || (temp1[0] == 'T'))
               CurrentTemp--;
       }
    }
if (traceGen) printf("Leaving GenPushArg\n");
    return(return_value+1);
}

DeclStmt (T) {
  if (traceGen) printf("Entering DeclStmt\n");   
    int  st_ind0, nest0;
    char name[50];
    char temp[20], temp1[20];
    tree p, p1, numnode;
    int sign = 1;
    int val;
    p = RightChild(T); p1 = LeftChild(p);

    //emit_data(".data\n", 'b', GlobalDataSize); 
    if (p1 != NullExp()) {
        p1 = LeftChild(p);
        if (NodeKind(p1) != STNode) {
            printf("wrong Decl format \n");
            exit(0);
        }
        strcpy(name, getname(GetAttr(IntVal(p1),
                     NAME_ATTR)));

        nest0 = GetAttr(IntVal(p1), NEST_ATTR);
        numnode = RightChild(RightChild(p));
        
        if (NodeOp(numnode) == UnaryNegOp) {
            sign = -1;
            numnode = LeftChild(numnode);
        }
        val = IntVal(numnode) * sign;
        if (nest0 == 1){ // global data
            int nStrInd = data_lookup(name);
            if ( nStrInd == -1 ){ // not exists
                emit_data(name, "word", val);
            }
        } else{// local data, in stack
            sprintf(temp1, "%d", val);
            op1.mode = IDENTIFIER;
            op1.ident = "\$11";
            op2.mode = IDENTIFIER;
            op2.ident = temp1;
    
            emit_most(LI, 'w', 2, op1, op2, op3);
            emit_pushstack("\$11");
        } // new data
    }
  if (traceGen) printf("Leaving DeclStmt\n");   
}

MethodStmt(T) {
  if (traceGen) printf("Entering MethodStmt\n");
    int  st_ind0, nest0;
    char name[50];
    char temp[20], temp1[20];
    tree p, p1;
    p = LeftChild(T); p1 = LeftChild(p);

    if (p1 != NullExp()) {
        if (NodeKind(p1) != STNode) {
            printf("wrong Method format \n");
            exit(0);
        }
        strcpy(name, getname(GetAttr(IntVal(p1),
                     NAME_ATTR)));

        if ( strcmp("main", name) == 0 ) {
            emit_main();
        }
        else{
            emit_entry(name);
        }
    }
  if (traceGen) printf("Leaving MethodStmt\n");
}

printlnStmt(T)
tree T;
{
/*.data
.align 2
S_36:	.asciiz	"x>=0"
.text
	li	$11	0		#load offset in $11
	add	$11	$11	0	#offr+field relative addr
	add	$11	$11	$gp		#class.---, add to t1 base
	move	$25	$11		#$offr=>$t25, base address
	move	$24	$t2		#$offr=>$t24, flag
	sw	$24	0($sp)		#push base $t2
	addi	$sp	$sp	-4	#push st
	sw	$25	0($sp)		#push base $t1
	addi	$sp	$sp	-4	#push st
	li	$v0	4		#print_str
	la	$a0	S_36		#address of string to print
	syscall				#print the arg
*/

    tree p;
    char name[50];
    p = LeftChild(RightChild(T));
    if (NodeKind (p) == STRINGNode){
        strcpy(name, getname(IntVal(p)));
        //strcpy(name, getname(GetAttr(IntVal(p), NAME_ATTR)));
        emit_rodata(IntVal(p), name);
    }
    emit_println(IntVal(p));
}

GenSingleStmt(T)
tree T;
{
    char name[50];
    tree p, p1;
    tree type, arg;
    char temp[20], temp1[20];
    int i,j,k;
    int size;
  if (traceGen) printf("Entering GenSingleStmt\n");   
    if (T == NullExp()) return(0);
    if (NodeOp(T) == 0) return(0);
    switch(NodeOp(T)) {
        case ProgramOp: // 100
            //emit_call("Program", 0);
            break;
        case BodyOp: // 101
            //emit_call("Body", 0);
            break;
        case DeclOp: // 102
            DeclStmt(T);
            break;
        case CommaOp: // 103
            //emit_call("Comma", 0);
            break;
        case ArrayTypeOp: // 104
            //emit_call("ArrayType", 0);
            break;
        case TypeIdOp: // 105
            //emit_call("TypeId", 0);
            break;
        case BoundOp: // 106
            //emit_call("Bound", 0);
            break;
        case RecompOp: // 107
            //emit_call("Recomp", 0);
            break;
        case ToOp: // 108
            //emit_call("To", 0);
            break;
        case DownToOp: // 109
            //emit_call("DownTo", 0);
            break;
        case ConstantIdOp: // 110
            //emit_call("ConstantId", 0);
            break;
        case ProceOp: // 111
            //emit_call("Proce", 0);
            break;
        case FuncOp: // 112
            //emit_call("Func", 0);
            break;
        case HeadOp: // 113
            //emit_call("Head", 0);
            break;
        case RArgTypeOp: // 114
            //emit_call("RArgType", 0);
            break;
        case VArgTypeOp: // 115
            //emit_call("VArgType", 0);
            break;
        case StmtOp: // 116
            //emit_call("Stmt", 0);
            break;
        case SpecOp: // 119
            //emit_call("Spec", 0);
            break;
        //case RoutineCallOp: // 120
        //    emit_call("RoutineCall", 0);
        //    break;
        //case AssignOp: // 121
        //    emit_call("Assign", 0);
        //    break;
        //case ReturnOp: // 122
        //    emit_call("Return", 0);
        //    break;
        case AddOp: // 123
            //emit_call("Add", 0);
            break;
        case SubOp: // 124
            //emit_call("Sub", 0);
            break;
        case MultOp: // 125
            //emit_call("Mult", 0);
            break;
        case DivOp: // 126
            //emit_call("Div", 0);
            break;
        case LTOp: // 127
            //emit_call("LT", 0);
            break;
        case GTOp: // 128
            //emit_call("GT", 0);
            break;
        case EQOp: // 129
            //emit_call("EQ", 0);
            break;
        case NEOp: // 130
            //emit_call("NE", 0);
            break;
        case LEOp: // 131
            //emit_call("LE", 0);
            break;
        case GEOp: // 132
            //emit_call("GE", 0);
            break;
        case AndOp: // 133
            //emit_call("And", 0);
            break;
        case OrOp: // 134
            //emit_call("Or", 0);
            break;
        case UnaryNegOp: // 135
            //emit_call("UnaryNeg", 0);
            break;
        case NotOp: // 136
            //emit_call("Not", 0);
            break;
        case VarOp: // 137
            //emit_call("Var", 0);
            break;
        case SelectOp: // 138
            //emit_call("Select", 0);
            break;
        case IndexOp: // 139
            //emit_call("Index", 0);
            break;
        case FieldOp: // 140
            //emit_call("Field", 0);
            break;
        case SubrangeOp: // 141
            //emit_call("Subrange", 0);
            break;
        case ClassOp: // 143
            //emit_call("Class", 0);
            break;
        case MethodOp: // 144
            MethodStmt(T);
            break;
        case ClassDefOp: // 145
            //emit_call("ClassDef", 0);
            break;

        // deal with Op
        case IfElseOp : 
            GenIfStmt(T);
            break;            
	case ExitOp :
            emit_call("exit", 0);
            break;
	case ReturnOp : 
            if (LeftChild(T) == NullExp())
                emit_idiot("\tret");
            else {
                GenValueExp(LeftChild(T));
                op1.mode = IDENTIFIER;
                op1.ident = ExpValue;
                op2.mode = REGISTER;
                op2.reg = 0;
                if ((ExpValue[0] =='R') || (ExpValue[0] == 'T') ||
                    (ExpValue[0] =='(') || (ExpValue[0] == '@'))
                    CurrentTemp--;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_idiot("\tret");
	    }
            break;
        case LoopOp :  GenLoopStmt(T); break;
        case AssignOp :
          if ((GetType(RightChild(LeftChild(T))) == CHAR_TYPE) ||
             (GetType(RightChild(LeftChild(T))) == INTEGER_TYPE)) {
            p = LeftChild(T);
            GenValueExp(RightChild(T));
            strcpy(temp, ExpValue);
            while (p!= NullExp()) {
                GenVarAddr(RightChild(p));
                if (VarOffset[0]=='R'){ 
                    sprintf(temp1, "(%s)", VarOffset);
                    CurrentTemp--;
		}
                else if (VarOffset[0] == 'T'){
                    sprintf(temp1, "@%s", VarOffset);
                    CurrentTemp--;

		}
                else strcpy(temp1, VarOffset);

                op1.mode = IDENTIFIER;
                op1.ident = temp;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;

                if (GetType(RightChild(p)) == CHAR_TYPE) 
                    emit_most(MOV, 'b', 2, op1, op2, op3);
                else if (GetType(RightChild(p)) == INTEGER_TYPE)
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                else {printf("Type incompatible\n"), exit(0);}
                p = LeftChild(p);
	    }
            if ((temp[0]=='R') || (temp[0] == 'T')
		|| (temp[0]=='(') || (temp[0]=='@'))  
               CurrentTemp--;
	  } else {
                    char blocktemp1[20], blcoktemp2[20];
                    char blocktemp[20];
                    char source[20], dest[20];
                    char dest1[20], source1[20];

/* caution mark, check again */

                    p = LeftChild(T);

                    GenVarAddr(RightChild(T));
                    strcpy(source, VarOffset); strcpy(source1, VarOffset);
                    if ((source[0] != 'R') && (source[0] != 'T')) {
                        op1.mode = IDENTIFIER;
                        op1.ident = VarOffset;
                        if (CurrentTemp < 10) {
                            op2.mode = REGISTER;
                            op2.reg = CurrentTemp+2;
                            sprintf(blocktemp1, "$%d", CurrentTemp+2);
                         } else {
                            op2.mode = IDENTIFIER;
                            sprintf(blocktemp1, "$%d", CurrentTemp+2);
                            op2.ident = blocktemp1;
                         }
                         CurrentTemp++;
                         if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                            MaxTemp = CurrentTemp+1;
                            sprintf(blocktemp, "$%d", MaxTemp+1);
                            emit_data(blocktemp, 'l', 1);
		         }
                         emit_most(MOVA, 'l', 2, op1, op2, op3);
                         strcpy(source, blocktemp1);
		    }
                    while (p != NullExp()) {

                      GenVarAddr(RightChild(p));
                      strcpy(dest, VarOffset); 
                      if ((dest[0] != 'R') && (dest[0] != 'T')) {
                          op1.mode = IDENTIFIER;
                          op1.ident = VarOffset;
                          if (CurrentTemp < 10) {
                              op2.mode = REGISTER;
                              op2.reg = CurrentTemp+2;
                              sprintf(blocktemp1, "$%d", CurrentTemp+2);
                           } else {
                              op2.mode = IDENTIFIER;
                              sprintf(blocktemp1, "$%d", CurrentTemp+2);
                              op2.ident = blocktemp1;
                           }
                           CurrentTemp++;
                           if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                              MaxTemp = CurrentTemp+1;
                              sprintf(blocktemp, "$%d", MaxTemp+1);
                              emit_data(blocktemp, 'l', 1);
		           }
                           emit_most(MOVA, 'l', 2, op1, op2, op3);
                           strcpy(dest, blocktemp1);
	 	       }

                       size = 4;
                       //size = TypeSize(GetVarType(RightChild(p)));
                       GenBlockAssign(dest, source, size);
                       if ((dest[0] == 'R') || (dest[0] == 'T'))
                           CurrentTemp--;
                       p = LeftChild(p);
		    }
                    if ((source[0] =='R') || (source[0] == 'T'))
                         CurrentTemp--;
		 }
            break;
        case RoutineCallOp : 
            p = LeftChild(T);
            if (GetAttr(IntVal(LeftChild(p)), PREDE_ATTR) == true) {
                strcpy(name, getname(GetAttr(IntVal(LeftChild(p)), 
                                     NAME_ATTR))); 
                /* deal with it right now, got to expand */
                if (strcmp("system", name) == 0) {
                    /* for left child tree */
                    p = RightChild(p); p1 = LeftChild(p);
                    if (p1 != NullExp()) {
                        p1 = LeftChild(p);
                        if (NodeKind(LeftChild(p1)) != STNode) {
                            printf("wrong system format \n");
                            exit(0);
                        }
                        strcpy(name, getname(GetAttr(IntVal(LeftChild(p1)),
                                     NAME_ATTR)));
                        if (strcmp("println", name) == 0){
                            //emit_call("println", 1);
                            printlnStmt(T);
                        }
                        else if (strcmp("readln", name) == 0){
                            emit_call("readln", 1);
                        }
                        else{
                            printf("wrong system format \n");
                            exit(0);
                        }
                        //p = RightChild(p);
                    }

                }
                else if (strcmp("CHR", name) == 0) {
                    int tempnum; 
                    char aa[50];
        
                    tempnum = CurrentTemp;
                    for (i= tempnum-1; i>=0; i--) {
                        if(i > 9) 
                          sprintf(aa, "\tPUSHL $%d", i+2);
                        else sprintf(aa, "\tPUSHL $%d", i+2);
                        emit_idiot(aa);
        	    }

                    GenValueExp(LeftChild(RightChild(T)));
                    op1.mode = IDENTIFIER;
                    op1.ident = ExpValue;
                    emit_most(PUSH, 'l', 1, op1, op2, op3);
                    emit_call("CHR", 1);

                    for (i=0; i<tempnum; i++) {
                        if (i>9) 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        else 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        emit_idiot(aa);
    		    }

                    if ( (ExpValue[0]=='R') || (ExpValue[0]=='T') ||
                         (ExpValue[0] == '@') || (ExpValue[0] == '('))
                         CurrentTemp--;                    
		}else if (strcmp("ORD", name) == 0) {
                    int tempnum; 
                    char aa[50];
        
                    tempnum = CurrentTemp;
                    for (i= tempnum-1; i>=0; i--) {
                        if(i > 9) 
                          sprintf(aa, "\tPUSHL $%d", i+2);
                        else sprintf(aa, "\tPUSHL $%d", i+2);
                        emit_idiot(aa);
        	    }

                    GenValueExp(LeftChild(RightChild(T)));
                    op1.mode = IDENTIFIER;
                    op1.ident = ExpValue;
                    emit_most(PUSH, 'l', 1, op1, op2, op3);
                    emit_call("ORD", 1);

                    for (i=0; i<tempnum; i++) {
                        if (i>9) 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        else 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        emit_idiot(aa);
    		    }

                    if ( (ExpValue[0]=='R') || (ExpValue[0]=='T') ||
                         (ExpValue[0] == '@') || (ExpValue[0] == '('))
                         CurrentTemp--;                    
		}else if (strcmp("EOF", name) == 0) {
                    int tempnum; 
                    char aa[50];
        
                    tempnum = CurrentTemp;
                    for (i= tempnum-1; i>=0; i--) {
                        if(i > 9) 
                          sprintf(aa, "\tPUSHL $%d", i+2);
                        else sprintf(aa, "\tPUSHL $%d", i+2);
                        emit_idiot(aa);
        	    }

                    emit_call("EOF", 0);

                    for (i=0; i<tempnum; i++) {
                        if (i>9) 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        else 
                           sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                        emit_idiot(aa);
    		    }

		} else {printf("DEBUG--not predefined\n");
                        exit(0);
                      }
	        }// endof if (GetAttr(IntVal(LeftChild(T)), PREDE_ATTR) == true)
                else {      /* function call */
                    int count;
                    type = (tree) GetAttr(IntVal(LeftChild(T)), TYPE_ATTR);
                    if (!type) {
                        break;
                    }
                    if (RightChild(LeftChild(type)) == NullExp()) {
                        emit_call(getname(GetAttr(IntVal(LeftChild
                                 (LeftChild(type))), NAME_ATTR)), 0);
		    }
                    else {
                        arg = LeftChild(RightChild(LeftChild(type)));
                        p = RightChild(T);
                        if (arg == NullExp())
                            emit_call(getname(GetAttr(IntVal(LeftChild
                                   (LeftChild(type))), NAME_ATTR)), 0);
                        else { /* pushing parameter */
                            count = GenPushArg(p, arg);
                            emit_call(getname(GetAttr(IntVal(LeftChild(T)),
                                               NAME_ATTR)), 
                                GetAttr(IntVal(LeftChild(T)), VALUE_ATTR));
			}
                    }              
                }// endof if (GetAttr(IntVal(LeftChild(T)), PREDE_ATTR) == true)
             break;
          default : printf("DEBUG--do't want to come to here\n");
                    exit(0);
	} 
  } 

GenStmts(T)
tree T;
{  if (traceGen) printf("Entering GenStmts\n");
    if (T==NullExp()) return;

  if (NodeKind (T) == EXPRNode && NodeOp (T) != IfElseOp)
    GenStmts(LeftChild(T));
    GenSingleStmt(RightChild (T)); 
  if (NodeKind (T) == EXPRNode && NodeOp (T) != IfElseOp)
    GenStmts(RightChild (T));

if (traceGen) printf("Leaving GenSingleStmt\n"); }

GenFun(T)
tree T;
{
    tree p, p1;
    char temp[80];
  if (traceGen) printf("Entering GenFun\n");
    if (RightChild(T) == NullExp()) return(0);
    p = LeftChild(LeftChild(T));
    sprintf(temp, "\t.ENTRY %s, 0", getname(GetAttr(IntVal(p), NAME_ATTR)));
    emit_idiot(temp);
    op1.mode= NUM_CONST;
    op1.num_const = GetAttr(IntVal(p), OFFSET_ATTR);
    op2.mode = REGISTER;
    op2.reg = SP;
    emit_most(SUB, 'l', 2, op1, op2, op3);
    GenStmts(RightChild(RightChild(T)));
    emit_idiot("\tret");
if (traceGen) printf("Leaving GenFun\n"); }


GenAll()   /* generate codes */
{
  if (traceGen) printf("Entering GenAll\n");

    emit_header();
    /* gen code for main */
    GenStmts(SyntaxTree);
    emit_end();
if (traceGen) printf("Leaving GenAll\n"); }

int GetConstValue(s)
char *s;
{
    int temp;
    char *p;
  if (traceGen) printf("Entering GetConstValue\n");
    if ((s[1]>='0') && (s[1]<='9')){
       p=s+1;
       temp = 0;
       while ((*p >='0') && (*p <= '9')) {
           temp = temp*10+(*p)-'0';
           p++;
       }
    } 
      else  temp = (int)s[5];
if (traceGen) printf("Leaving GetConstValue\n");
    return(temp);
}

GenValueExp(T)
tree T;
{
    char temp1[20];
    char temp2[20];
    char temp3[20];
    char temp[20];
    char ch;
  if (traceGen) printf("Entering GenValueExp\n"); 
    ExpValue[0] = '\0';

    if (NodeKind(T) == NUMNode) {
        sprintf(temp2, "%d", IntVal(T));
        strcpy(ExpValue, temp2);
        return(0);
    } else if (NodeKind(T) == CHARNode) {
        if (isprint(IntVal(T)))
            sprintf(temp2, "#^A/%c/", IntVal(T));
        else sprintf(temp2, "%d", IntVal(T));
        strcpy(ExpValue, temp2);
        return(0);
    } else if (NodeKind(T) == STRINGNode) {
        sprintf(temp1, "s%d", StringNum++);
        emit_str(temp1, getname(IntVal(T)));
        sprintf(temp2, "%s", temp1);
        strcpy(ExpValue, temp2);
        return(0);
    } else if (NodeOp(T) == VarOp) {
        int i;
        i = IntVal(LeftChild(T));
        if  (GetAttr(i, KIND_ATTR) == CONST) {
            if ((tree)GetAttr(i, TYPE_ATTR) == intTypeT){
               sprintf(ExpValue, "%d", GetAttr(i, VALUE_ATTR));
               return(0);
	   } else if ((tree)GetAttr(i, TYPE_ATTR) == charTypeT) {
               if (isprint(GetAttr(i, VALUE_ATTR))) 
                   sprintf(ExpValue, "#^A/%c/", GetAttr(i, VALUE_ATTR));
               else sprintf(ExpValue, "%d", GetAttr(i, VALUE_ATTR));
               return(0);
	   } else {
               if (!IsAttr(i, OFFSET_ATTR)) {
                   emit_str(getname(GetAttr(i, NAME_ATTR)),
                            getname(GetAttr(i, VALUE_ATTR)));
                   SetAttr(i, OFFSET_ATTR, 0);
               } 
               sprintf(ExpValue, "%s", getname(GetAttr(i, NAME_ATTR)));
               return(0);
	   }
	}
        GenVarAddr(T);
        if ((VarOffset[0]=='R'))
            sprintf(ExpValue, "(%s)", VarOffset);
        else if  ((VarOffset[0] == 'T'))
            sprintf(ExpValue, "@$%d", VarOffset);
        else 
            strcpy(ExpValue, VarOffset); 
        return(0);
    } else if (NodeOp(T) == RoutineCallOp) {
            int count;
            tree type, arg, p;
            int tempnum;
            int i;
            char aa[80];

            /* save temp first */

            tempnum = CurrentTemp;
            for (i= tempnum-1; i>=0; i--) {
                if(i > 9) 
                  sprintf(aa, "\tPUSHL $%d", i+2);
                else sprintf(aa, "\tPUSHL $%d", i+2);
                emit_idiot(aa);
	    }
            
            type = (tree) GetAttr(IntVal(LeftChild(T)), TYPE_ATTR);
            if (RightChild(LeftChild(type)) == NullExp()) {
                emit_call(getname(GetAttr(IntVal(LeftChild
                          (LeftChild(type))), NAME_ATTR)), 
                          GetAttr(IntVal(LeftChild(T)), 
                          VALUE_ATTR));
                for (i=0; i<tempnum; i++) {
                    if (i>9) 
                       sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                    else 
                       sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                    emit_idiot(aa);
		}
	    }
            else {
                arg = LeftChild(RightChild(LeftChild(type)));
                p = RightChild(T);
                if (arg == NullExp())
                    emit_call(getname(GetAttr(IntVal(LeftChild
                              (LeftChild(type))), NAME_ATTR)),
                          GetAttr(IntVal(LeftChild(T)), 
                          VALUE_ATTR));
                else { /* push parameters */
                    count = GenPushArg(p, arg);
                    emit_call(getname(GetAttr(IntVal(LeftChild(T)),
                                NAME_ATTR)), 
                          GetAttr(IntVal(LeftChild(T)), 
                          VALUE_ATTR));
		}
                for (i=0; i<tempnum; i++) {
                    if (i>9) 
                       sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                    else 
                       sprintf(aa, "\tMOVL (SP)+, $%d", i+2);
                    emit_idiot(aa);
		}

                op1.mode = REGISTER;
                op1.reg = R0;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                strcpy(ExpValue, temp1);
                return(0);
            }              
	} else {    /* all operation */
            if ((GetType(LeftChild(T)) == CHAR_TYPE) &&
                (GetType(RightChild(T)) == CHAR_TYPE)) 
                ch = 'b';
            else ch = 'l';
            if (NodeOp(T) == AddOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", CurrentTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                sprintf(ExpValue, "%d", GetConstValue(temp1)+
                                         GetConstValue(temp2));
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(ADD, 'l', 2, op1, op2, op3);
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                       CurrentTemp--;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", CurrentTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                op3.mode = IDENTIFIER;
                op3.ident = temp2;
                emit_most(ADD, 'l', 3, op1, op2, op3);
                strcpy(ExpValue, temp2);
                return(0);
	    }
      } else if (NodeOp(T) == SubOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", CurrentTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                sprintf(ExpValue, "%d", GetConstValue(temp1)-
                                         GetConstValue(temp2));
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(SUB, 'l', 2, op1, op2, op3);
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp--;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", CurrentTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                op3.mode = IDENTIFIER;
                op3.ident = temp2;
                emit_most(SUB, 'l', 3, op1, op2, op3);
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == MultOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", CurrentTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                sprintf(ExpValue, "%d", GetConstValue(temp1)*
                                         GetConstValue(temp2));
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MUL, 'l', 2, op1, op2, op3);
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if  ((temp2[0]=='(') || (temp2[0]=='@')) 
                       CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", CurrentTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                op3.mode = IDENTIFIER;
                op3.ident = temp2;
                emit_most(MUL, 'l', 3, op1, op2, op3);
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == DivOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp2) == 0) {
                    printf("Divided by zero\n");
                    exit(0);
		}
                sprintf(ExpValue, "%d", GetConstValue(temp1)/
                                         GetConstValue(temp2));
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum++);
                emit_call("ZERO_DIV", 0);
                emit_label(LabelNum-1);
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
           
                emit_most(DIV, 'l', 2, op1, op2, op3);
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp--;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp2;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                op3.mode = IDENTIFIER;
                op3.ident = temp2;
                emit_most(DIV, 'l', 3, op1, op2, op3);
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == LTOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, ch, 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch , 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) < GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BLSS, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if  ((temp2[0]=='(') || (temp2[0]=='@'))
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, ch, 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, ch, 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BLSS, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == GTOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "\$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, ch, 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch, 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) > GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BGTR, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if ((temp2[0]=='(') || (temp2[0]=='@'))
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, ch, 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, ch, 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BGTR, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      } else if (NodeOp(T) == EQOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, ch, 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch, 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) == GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')){
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, ch, 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, ch, 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == NEOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, ch, 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch, 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) != GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')){
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, ch, 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, ch, 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == LEOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')) {
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, ch, 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch, 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) <= GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BLEQ, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, ch, 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, ch, 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(CMP, ch, 2, op1, op2, op3);
                emit_goto(BLEQ, LabelNum++);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == GEOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')) {
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = "\$11";
                /*if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "\$1%d", CurrentTemp+2);
 	        } else {*/
                    op2.mode = IDENTIFIER;
                    op2.ident = temp1;
                //}
                //emit_most(MOV, ch, 2, op1, op2, op3);
                emit_most(LI, 'w', 2, op1, op2, op3);
                emit_readfp("\$12");
                emit_pushstack("\$12");
                /*CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, ch, 1);
		}*/
	    //}
            //if (NodeKind(RightChild(T)) == NUMNode) {
            }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if (GetConstValue(temp1) >= GetConstValue(temp2))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = "\$11";
                op2.mode = IDENTIFIER;
                sprintf(op2.ident, "%d", IntVal(RightChild(T)));
                emit_most(LI, 'w', 2, op1, op2, op3);
                emit_pushstack("\$11");
  char s_addon[128];
  sprintf(s_addon, "\tlw\t\$11\t4(\$fp)\n" 
        "\tlw\t\$11\t4(\$sp)\n"
	"\taddi\t\$sp\t\$sp\t4\n" //#pop st
	"\tlw\t\$12\t4(\$sp)\n" //#stack top -> $12
	"\tsge\t\$11\t\$12\t\$11\n" //#add r1 to r2
	"\tsw\t\$11\t4(\$sp)\n" //#push the sum on stack
	"\tlw\t\$11\t4(\$sp)"); //#store boolean result in register
	//"\tbeqz\t\$11\tL_1\n"// #if false, jump to L_1

  new_code();
  str_code(s_addon);
  tab_code(1, strlen(s_addon));

                emit_jmp(BEQZ, "\$11", LabelNum++);
                //emit_label(LabelNum-1);             
                return(0);
	    }
      }else if (NodeOp(T) == AndOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')) {
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

  
            if ((temp1[0] == '#') && (temp2[0] == '#')) { 
                 /* constant unfolded */ 
                if ((GetConstValue(temp1) != 0) &&
                    ( GetConstValue(temp2) != 0))
                    sprintf(ExpValue, "#1");
                else sprintf(ExpValue, "#0");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum++);
                op1.mode=IDENTIFIER;
                op1.ident = temp2;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum-1);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')){
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum++);
                op1.mode=IDENTIFIER;
                op1.ident = temp2;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum-1);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      }else if (NodeOp(T) == OrOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')) {
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            GenValueExp(RightChild(T));
            strcpy(temp2, ExpValue);

  
            if ((temp1[0] == '#') && (temp2[0] == '#')){
                 /* constant unfolded */ 
                if ((GetConstValue(temp1) == 0) &&
                    ( GetConstValue(temp2) == 0))
                    sprintf(ExpValue, "#0");
                else sprintf(ExpValue, "#1");
                return(0);
	    }
            else if (temp1[0]!= '#') {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum++);
                op1.mode=IDENTIFIER;
                op1.ident = temp2;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum-1);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                if ((temp2[0]=='R') || (temp2[0]=='T')
                    || (temp2[0]=='(') || (temp2[0]=='@')) 
                     CurrentTemp--;
                strcpy(ExpValue, temp1);
                return(0);
	    }
            else if (temp2[1]!= '#') {
                if ((temp2[0]!='R') && (temp2[0]!='T')) {
                    if ((temp2[0]=='(') || (temp2[0]=='@')) 
                        CurrentTemp --;
                    strcpy(temp, temp2);
                    op1.mode = IDENTIFIER;
                    op1.ident = temp;
                    if (CurrentTemp < 10) {
                        op2.mode = REGISTER;
                        op2.reg = CurrentTemp+2;
                        sprintf(temp2, "$%d", CurrentTemp+2);
 	            } else {
                        op2.mode = IDENTIFIER;
                        sprintf(temp2, "$%d", CurrentTemp+2);
                        op2.ident = temp2;
                    }
                    emit_most(MOV, 'l', 2, op1, op2, op3);
                    CurrentTemp++;
                    if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                        MaxTemp = CurrentTemp+1;
                        sprintf(temp, "$%d", MaxTemp+1);
                        emit_data(temp, 'l', 1);
	    	    }
	        }
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum++);
                op1.mode=IDENTIFIER;
                op1.ident = temp2;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BNEQ, LabelNum-1);
                op1.mode=NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode=NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp2;
                emit_most(MOV, 'l', 2, op1, op2, op3);
                emit_label(LabelNum-1);             
                strcpy(ExpValue, temp2);
                return(0);
	    }
      } else if (NodeOp(T) == UnaryNegOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T') ){
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            if (temp1[0] == '#') { 
                 /* constant unfolded */ 
                sprintf(ExpValue, "%d", 0-GetConstValue(temp1));
                return(0);
	    }
            else {
                op1.mode = IDENTIFIER;
                op1.ident = temp1;
                op2.mode = NUM_CONST;
                op2.num_const = 0;
                op3.mode = IDENTIFIER;
                op3.ident = temp1;
                emit_most(SUB, 'l', 3, op1, op2, op3);
                strcpy(ExpValue, temp1);
                return(0);
	    }
      } else if (NodeOp(T) == NotOp) {
            GenValueExp(LeftChild(T));
            strcpy(temp1, ExpValue);
            if ((temp1[0]!='#') && (temp1[0]!='R') && (temp1[0]!='T')) {
                 if ((temp1[0]=='(') || (temp1[0]=='@')) 
                     CurrentTemp--;
                strcpy(temp, temp1);
                op1.mode = IDENTIFIER;
                op1.ident = temp;
                if (CurrentTemp < 10) {
                    op2.mode = REGISTER;
                    op2.reg = CurrentTemp+2;
                    sprintf(temp1, "$%d", CurrentTemp+2);
 	        } else {
                    op2.mode = IDENTIFIER;
                    sprintf(temp1, "$%d", CurrentTemp+2);
                    op2.ident = temp1;
                }
                emit_most(MOV, 'l', 2, op1, op2, op3);
                CurrentTemp++;
                if ((CurrentTemp>10) && (CurrentTemp+1 > MaxTemp)) {
                    MaxTemp = CurrentTemp+1;
                    sprintf(temp, "$%d", MaxTemp+1);
                    emit_data(temp, 'l', 1);
		}
	    }
            if (temp1[0] == '#') { 
                 /* constant unfolded */
                if (GetConstValue(temp1) != 0)  
                    sprintf(ExpValue, "#0");
                else sprintf(ExpValue, "#1");
                return(0);
	    }
            else {
                op1.mode = NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(CMP, 'l', 2, op1, op2, op3);
                emit_goto(BEQL, LabelNum++);
                op1.mode = NUM_CONST;
                op1.num_const = 0;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);                
                emit_goto(JMP, LabelNum++);
                emit_label(LabelNum-2);
                op1.mode = NUM_CONST;
                op1.num_const = 1;
                op2.mode = IDENTIFIER;
                op2.ident = temp1;
                emit_most(MOV, 'l', 2, op1, op2, op3);                
                emit_label(LabelNum-1);
                strcpy(ExpValue, temp1);
                return(0);
	    }
       } else {
                printf("DEBUG--wrong exp\n");
                exit(0);
	}
    } 
if (traceGen) printf("Leaving GenValueExp\n"); }

/* get the lower bound for bound checking and code gen */
int GetLowerBound(T, index)
tree T;
int index;
{
    int count;
    tree p;
    int i,j;
  if (traceGen) printf("Entering GetLowerBound\n");
    p = LeftChild(T);
    count = 0;
    while (p!=NullExp()) {
       count ++;
       p = LeftChild(p);
    }
    p = LeftChild(T);
    for (i=0; i<count-index; i++) p = LeftChild(p);
if (traceGen) printf("Leaving GetLowerBound\n");
    return(LowerBound(RightChild(p)));
}

/* get upper bound for a certain index */
int GetUpperBound(T, index)
tree T;
int index;
{
    int count;
    tree p;
    int i,j;
  if (traceGen) printf("Entering GetUpperBound\n");
    p = LeftChild(T);
    count = 0;
    while (p!=NullExp()) {
       count ++;
       p = LeftChild(p);
    }
    p = LeftChild(T);
    for (i=0; i<count-index; i++) p = LeftChild(p);
if (traceGen) printf("Leaving GetUpperBound\n");
    return(UpperBound(RightChild(p)));
}
    
GenVarAddr(T)
tree T;
{
    tree p, p1, p2;
    char temp [20];
    char temp1[20], temp2[20], temp3[20];
    int location;
    int offset;
    tree type;
    int tag;
    int index;

  if (traceGen) printf("Entering GenVarAddr\n");
    if ((NodeKind(T) == STRINGNode)) {
        sprintf(temp1, "s%d", StringNum++);
        emit_str(temp1, getname(IntVal(T)));
        sprintf(temp2, "%s", temp1);
        strcpy(VarOffset, temp2);
        return(0);
    } else if (NodeKind(T) != EXPRNode) {
        printf("Incompatible assignment \n");
        sprintf(VarOffset, "%d", IntVal(T));
        return(0);
    }

        location = GetAttr(IntVal(LeftChild(T)), PLACE_ATTR);
        offset = GetAttr(IntVal(LeftChild(T)), OFFSET_ATTR);
        type = (tree)GetAttr(IntVal(LeftChild(T)), TYPE_ATTR);
        tag = 0;

    if (RightChild(T) == NullExp()) {
        if (location == GLOBAL)  
          sprintf(temp, "%d", offset);
        else if (location == RARGUE) {
          sprintf(temp1, "%d($ra)", offset);
          op1.mode = IDENTIFIER;
          op1.ident = temp1;
          if (CurrentTemp < 10) {
              op2.mode = REGISTER;
              op2.reg = CurrentTemp+2;
              sprintf(temp, "$%d", CurrentTemp+2);
 	  } else {
              op2.mode = IDENTIFIER;
              sprintf(temp, "$%d", CurrentTemp+2);
              op2.ident = temp;
          }
          emit_most(MOV, 'l', 2, op1, op2, op3);
          CurrentTemp++;
          if ((CurrentTemp>10) && 
              (CurrentTemp+1 > MaxTemp)) {
              MaxTemp = CurrentTemp+1;
              sprintf(temp, "$%d", CurrentTemp+1);
              emit_data(temp, 'l', 1);
          }          
        }
        else if (location == VARGUE)
          sprintf(temp, "%d($ra)", offset);
        else sprintf(temp, "%d($fp)", offset);
        strcpy(VarOffset, temp);
    } else {
        p = RightChild(T);
        temp1[0] = '\0';

        while (p != NullExp()) {    /* calculate the address */
            p1 = LeftChild(p);
            if (NodeOp(p1) ==IndexOp) {
                index = 1;
                GenValueExp(LeftChild(p1));
                if (ExpValue[0] != '#'){
                    tag = 1;
                    
                    /* checking the boundory */
                    op1.mode = NUM_CONST;
                    op1.num_const = GetLowerBound(type, index);
                    op2.mode = IDENTIFIER;
                    op2.ident = ExpValue;
                    emit_most(CMP, 'l', 2, op1, op2, op3);
                    emit_goto(BLEQ, LabelNum++);
                    emit_call("ARRAY_BOUND", 0);
                    emit_label(LabelNum -1);
                    op1.mode = NUM_CONST;
                    op1.num_const = GetUpperBound(type, index);
                    /* op2 the same */
                    emit_most(CMP, 'l', 2, op1, op2, op3);
                    emit_goto(BGEQ, LabelNum++);
                    emit_call("ARRAY_BOUND", 0);
                    emit_label(LabelNum-1);
                    if ((ExpValue[0] == 'R') || (ExpValue[0] =='T')) {
                        op1.mode = NUM_CONST;
                        op1.num_const = GetLowerBound(type, index);
                        op2.mode = IDENTIFIER;
                        op2.ident = ExpValue;
                        emit_most(SUB, 'l', 2, op1, op2, op3);
                        strcpy(temp1, ExpValue);
                    }  else { /* get a now tempary */
                        op1.mode = IDENTIFIER;
                        op1.ident = ExpValue;
                        if (CurrentTemp < 10) {
                            op2.mode = REGISTER;
                            op2.reg = CurrentTemp+2;
                            sprintf(temp1, "$%d", CurrentTemp+2);
 	                } else {
                            op2.mode = IDENTIFIER;
                            sprintf(temp1, "$%d", CurrentTemp+2);
                            op2.ident = temp1;
                        }
                        emit_most(MOV, 'l', 2, op1, op2, op3);
                        op1.mode = NUM_CONST;
                        op1.num_const = GetLowerBound(type, index);
                        /* op2 does not change */
                        emit_most(SUB, 'l', 2, op1, op2, op3);
                        CurrentTemp++;
                        if ((CurrentTemp>10) && 
                            (CurrentTemp+1 > MaxTemp)) {
                            MaxTemp = CurrentTemp+1;
                            sprintf(temp, "$%d", CurrentTemp+1);
                            emit_data(temp, 'l', 1);
   		        }
                   }
                }   
                else {
                    sprintf(temp1, "%d", GetConstValue(ExpValue)-
                                         GetLowerBound(type, index));
                }

                p2 = RightChild(p1); index ++;
                while(p2!=NullExp()) {

                    GenValueExp(LeftChild(p2));
                    if (ExpValue[0] != '#') tag = 1;
                    if (!tag) 
                         sprintf(temp1, "%d",GetConstValue(temp1)*
                           (GetUpperBound(type, index) - 
                            GetLowerBound(type, index)+1) +
                            GetConstValue(ExpValue)); 
                    if (tag) { /* gen code to calculate the index */
               
                       if (temp1[0]=='#')
/*** changed ***/         sprintf(temp1, "%d", GetConstValue(temp1)*
                                  (GetUpperBound(type, index)-
                                   GetLowerBound(type, index)+1));
                       else {
                           op1.mode = NUM_CONST;
                           op1.num_const = GetUpperBound(type, index)-
                                           GetLowerBound(type, index)+1;
                           op2.mode = IDENTIFIER;
                           op2.ident = temp1;
                           emit_most(MUL, 'l', 2, op1, op2, op3);
		       }
                        if (ExpValue[0]!='#') {
                           op1.mode = NUM_CONST;
                           op1.num_const = GetLowerBound(type, index);
                           op2.mode = IDENTIFIER;
                           op2.ident = ExpValue;
                           emit_most(CMP, 'l', 2, op1, op2, op3);
                           emit_goto(BLEQ, LabelNum++);
                           emit_call("ARRAY_BOUND", 0);
                           emit_label(LabelNum -1);
                           op1.mode = NUM_CONST;
                           op1.num_const = GetUpperBound(type, index);
                           /* op2 the same */
                           emit_most(CMP, 'l', 2, op1, op2, op3);
                           emit_goto(BGEQ, LabelNum++);
                           emit_call("ARRAY_BOUND", 0);
                           emit_label(LabelNum-1);
		       }
                       if (temp1[0] != '#'){
                           op1.mode = IDENTIFIER;
                           op1.ident = ExpValue;
                           op2.mode = IDENTIFIER;
                           op2.ident = temp1;
                           emit_most(ADD, 'l', 2, op1, op2, op3);
                           if ( (ExpValue[0] =='R') || (ExpValue[0]=='T'))
                               CurrentTemp --;
                           /* temp1 is still the same */
		       } else {
                           if ((ExpValue[0] == 'R') || (ExpValue[0] =='T')) {
                               op1.mode = NUM_CONST;
                               op1.num_const = GetConstValue(temp1);
                               op2.mode = IDENTIFIER;
                               op2.ident = ExpValue;
                               emit_most(ADD, 'l', 2, op1, op2, op3);
                               strcpy(temp1, ExpValue);
                           }  else { /* get a now tempary */
                               op1.mode = IDENTIFIER;
                               op1.ident = ExpValue;
                               if (CurrentTemp < 10) {
                                   op2.mode = REGISTER;
                                   op2.reg = CurrentTemp+2;
                                   sprintf(temp1, "$%d", CurrentTemp+2);
 	                       } else {
                                   op2.mode = IDENTIFIER;
                                   sprintf(temp1, "$%d", CurrentTemp+2);
                                   op2.ident = temp1;
                               }
                               emit_most(MOV, 'l', 2, op1, op2, op3);
                               op1.mode = NUM_CONST;
                               op1.num_const = GetLowerBound(type, index);
                               /* op2 does not change */
                               emit_most(ADD, 'l', 2, op1, op2, op3);
                               CurrentTemp++;
                               if ((CurrentTemp>10) && 
                                   (CurrentTemp+1 > MaxTemp)) {
                                   MaxTemp = CurrentTemp+1;
                                   sprintf(temp, "$%d", CurrentTemp+1);
                                   emit_data(temp, 'l', 1);
   		               }                       
			   }
		       }
		   }
		   p2 = RightChild(p2);
                   index ++;
		}
                type = RightChild(type);
                if (temp1[0] == '#') 
                   sprintf(temp1, "%d", GetConstValue(temp1) *
                               TypeSize(type));
                else if (temp1[0] != '\0') {
                   op1.mode = NUM_CONST;
                   op1.num_const = TypeSize(type);
                   op2.mode= IDENTIFIER;
                   op2.ident = temp1;
                   emit_most(MUL, 'l', 2, op1, op2, op3);
	       }
	    } else if (NodeOp(p1) == FieldOp) {
                if (temp1[0] == '\0') 
                    sprintf(temp1, "%d", GetAttr(IntVal(LeftChild(p1)),
                                                  OFFSET_ATTR)); 
                if (temp1[0] == '#') 
                    sprintf(temp1, "%d", GetConstValue(temp1)+
                                GetAttr(IntVal(LeftChild(p1)), OFFSET_ATTR));
                else {
                    op1.mode = NUM_CONST;
                    op1.num_const = GetAttr(IntVal(LeftChild(p1)), 
                                            OFFSET_ATTR);
                    op2.mode = IDENTIFIER;
                    op2.ident = temp1;
		    emit_most(ADD, 'l', 2, op1, op2, op3);
		}
                type = (tree) GetAttr(IntVal(LeftChild(p1)), TYPE_ATTR);
	    }
	    p = RightChild(p);
	}
        if (location == GLOBAL) {
          if (temp1[0] =='#') 
              sprintf(temp1, "%d", offset+GetConstValue(temp1));
          else {
              op1.mode= IDENTIFIER;
              sprintf(temp2, "%d", offset);
              op1.ident = temp2;
              if (CurrentTemp < 10) {
                  op2.mode = REGISTER;
                  op2.reg = CurrentTemp+2;
                  sprintf(temp, "$%d", CurrentTemp+2);
              } else {
                  op2.mode = IDENTIFIER;
                  sprintf(temp, "$%d", CurrentTemp+2);
                  op2.ident = temp;
              }
              emit_most(MOVA, 'l', 2, op1, op2, op3);
              op1.mode = IDENTIFIER;
              op1.ident = temp;
              op2.mode = IDENTIFIER;
              op2.ident = temp1;
              emit_most(ADD, 'l', 2, op1, op2, op3);
              CurrentTemp++;
              if ((CurrentTemp>10) && 
                  (CurrentTemp+1 > MaxTemp)) {
                   MaxTemp = CurrentTemp+1;
                   sprintf(temp, "$%d", CurrentTemp+1);
                   emit_data(temp, 'l', 1);
              } 
              CurrentTemp--;
	  }
	}
        else if (location == RARGUE){ /* will change here */
          if (temp1[0] == '#') {
              sprintf(temp, "%d($ra)", offset);
              op1.mode = IDENTIFIER;
              op1.ident = temp;
              op2.mode = NUM_CONST;
              op2.num_const = GetConstValue(temp1);
              if (CurrentTemp < 10) {
                  op3.mode = REGISTER;
                  op3.reg = CurrentTemp+2;
                  sprintf(temp1, "$%d", CurrentTemp+2);
  	      } else {
                  op3.mode = IDENTIFIER;
                  sprintf(temp1, "$%d", CurrentTemp+2);
                  op3.ident = temp1;
              }
              emit_most(ADD, 'l', 3, op1, op2, op3);
              CurrentTemp++;
              if ((CurrentTemp>10) && 
                  (CurrentTemp+1 > MaxTemp)) {
                  MaxTemp = CurrentTemp+1;
                  sprintf(temp, "$%d", CurrentTemp+1);
                  emit_data(temp, 'l', 1);
              } 
            } else {
              sprintf(temp, "%d($ra)", offset);
              op1.mode = IDENTIFIER;
              op1.ident = temp;
              op2.mode = IDENTIFIER;
              op2.ident = temp1;
              if ((temp1[0] == 'R') || (temp1[0] =='T')
                  || (temp1[0] == '(') || (temp1[0] =='@'))
                  CurrentTemp--;
              if (CurrentTemp < 10) {
                  op3.mode = REGISTER;
                  op3.reg = CurrentTemp+2;
                  sprintf(temp2, "$%d", CurrentTemp+2);
  	      } else {
                  op3.mode = IDENTIFIER;
                  sprintf(temp2, "$%d", CurrentTemp+2);
                  op3.ident = temp2;
              }
              emit_most(ADD, 'l', 3, op1, op2, op3);
              CurrentTemp++;
              if ((CurrentTemp>10) && 
                  (CurrentTemp+1 > MaxTemp)) {
                  MaxTemp = CurrentTemp+1;
                  sprintf(temp, "$%d", CurrentTemp+1);
                  emit_data(temp, 'l', 1);
              } 
              strcpy(temp1, temp2);
           }
        }               
        else if (location == VARGUE) {
          if (temp1[0] =='#')
              sprintf(temp1, "%d($ra)", offset+GetConstValue(temp1));
          else {
              op1.mode = REGISTER;
              op1.reg = AP;
              op2.mode = NUM_CONST;
              op2.num_const = offset;
              if (CurrentTemp < 10) {
                  op3.mode = REGISTER;
                  op3.reg = CurrentTemp+2;
                  sprintf(temp, "$%d", CurrentTemp+2);
              } else {
                  op3.mode = IDENTIFIER;
                  sprintf(temp, "$%d", CurrentTemp+2);
                  op3.ident = temp;
              }
              emit_most(ADD, 'l', 3, op1, op2, op3);
              op1.mode = IDENTIFIER;
              op1.ident = temp;
              op2.mode = IDENTIFIER;
              op2.ident = temp1;            /* might change here */
              emit_most(ADD, 'l', 2, op1, op2, op3);
              CurrentTemp++;
              if ((CurrentTemp>10) && 
                  (CurrentTemp+1 > MaxTemp)) {
                   MaxTemp = CurrentTemp+1;
                   sprintf(temp, "$%d", CurrentTemp+1);
                   emit_data(temp, 'l', 1);
              } 
              CurrentTemp--;
	  }
	}
        else {
          if (temp1[0] =='#')
              sprintf(temp1, "%d($fp)", offset+GetConstValue(temp1));
          else {
              op1.mode = REGISTER;
              op1.reg = FP;
              op2.mode = NUM_CONST;
              op2.num_const = offset;
              if (CurrentTemp < 10) {
                  op3.mode = REGISTER;
                  op3.reg = CurrentTemp+2;
                  sprintf(temp, "$%d", CurrentTemp+2);
              } else {
                  op3.mode = IDENTIFIER;
                  sprintf(temp, "$%d", CurrentTemp+2);
                  op3.ident = temp;
              }
              emit_most(ADD, 'l', 3, op1, op2, op3);
              op1.mode = IDENTIFIER;
              op1.ident = temp;
              op2.mode = IDENTIFIER;
              op2.ident = temp1;
              emit_most(ADD, 'l', 2, op1, op2, op3);
              CurrentTemp++;
              if ((CurrentTemp>10) && 
                  (CurrentTemp+1 > MaxTemp)) {
                   MaxTemp = CurrentTemp+1;
                   sprintf(temp, "$%d", CurrentTemp+1);
                   emit_data(temp, 'l', 1);
              } 
              CurrentTemp--;
	  }
	}

        strcpy(VarOffset, temp1);
    }
if (traceGen) printf("Leaving GenVarAddr\n"); }

/* gen if statement */
int GenIfStmt(T)
tree T;
{
    int tag;
    int i,j,k;
    tree p, p1;
  if (traceGen) printf("Entering GenIfStmt\n");
    if (T == NullExp()) {
  if (traceGen) printf("Leaving GenIfStmt\n");
      return(1);
    }
    tag = GenIfStmt(LeftChild(T));

    if (!tag) emit_label(PopLabel());
    else PushLabel(LabelNum++); /* for the end of the if loop */

    p = RightChild(T);
    if (p == NullExp()) 
        emit_label(PopLabel());
    else if ((NodeKind(p) == EXPRNode) && (NodeOp(p)!=CommaOp)) {
        /* for the files not if */
        GenStmts(p);
        emit_label(PopLabel());
    } 
    else {          /* for the ifels */
        GenValueExp(LeftChild(p));
        GenStmts(RightChild(p));

        /*op1.mode = NUM_CONST;
        op1.num_const = 0;
        op2.mode = IDENTIFIER;
        op2.ident = ExpValue;
        emit_most(CMP, 'l', 2, op1, op2, op3);

        emit_goto(BNEQ, LabelNum++);
        emit_goto(JMP, LabelNum++);*/
        PushLabel(LabelNum-1);
        if ((ExpValue[0] == 'R') || (ExpValue[0] == 'T')
            || (ExpValue[0] == '(') || (ExpValue[0] == '@'))
            CurrentTemp--;
        i = PopLabel();
        emit_goto(JMP, TopLabel());
        emit_label(LabelNum-1);
        emit_label(LabelNum-2);
        PushLabel(i);
    }
if (traceGen) printf("Leaving GenIfStmt\n");
    return(0);
}

/* generated code for loop statement */
GenLoopStmt(T)
tree T;
{
    tree p, p1;
    char temp1[20];
    char temp2[20];
    char temp3[20];
    int i,j,k;
    int location;
  if (traceGen) printf("Entering GenLoopStmt\n");
    p = LeftChild(T);
    if (p == NullExp()) { /* empty repeat */
        emit_label(LabelNum++);
        GenValueExp(RightChild(T));
        if ((ExpValue[0] == 'R') || (ExpValue[0] == 'T')
            || (ExpValue[0] == '(') || (ExpValue[0] == '@'))
            CurrentTemp--;
        op1.mode= NUM_CONST;
        op1.num_const = 0;
        op2.mode = IDENTIFIER;
        op2.ident = ExpValue;
        emit_most(CMP, 'l', 2, op1, op2, op3);
        emit_goto(BEQL, LabelNum++);
        emit_goto(JMP, LabelNum-2);
        emit_label(LabelNum-1);
    } else if ((NodeKind(p) == EXPRNode) &&
               (NodeOp(p) == StmtOp)) {

        emit_label(LabelNum++);
        PushLabel(LabelNum-1);
        GenStmts(p);
        GenValueExp(RightChild(T));
        if ((ExpValue[0] == 'R') || (ExpValue[0] == 'T')
            || (ExpValue[0] == '(') || (ExpValue[0] == '@')) 
            CurrentTemp--;
        op1.mode= NUM_CONST;
        op1.num_const = 0;
        op2.mode = IDENTIFIER;
        op2.ident = ExpValue;
        emit_most(CMP, 'l', 2, op1, op2, op3);
        emit_goto(BNEQ, LabelNum++);
        emit_goto(JMP, PopLabel());
        emit_label(LabelNum-1);
    } else if ((NodeKind(p) == EXPRNode) &&
               (NodeOp(p) != CommaOp)) {
        emit_label(LabelNum++);
        PushLabel(LabelNum-1);
        GenValueExp(p);
        if ((ExpValue[0] == 'R') || (ExpValue[0] == 'T')
            || (ExpValue[0] == '(') || (ExpValue[0] == '@')) 
            CurrentTemp--;
        op1.mode= NUM_CONST;
        op1.num_const = 0;
        op2.mode = IDENTIFIER;
        op2.ident = ExpValue;
        emit_most(CMP, 'l', 2, op1, op2, op3);
        emit_goto(BNEQ, LabelNum++);
        emit_goto(JMP, LabelNum++);
        emit_label(LabelNum-2);        
        PushLabel(LabelNum-1);
        GenStmts(RightChild(T));
        i = PopLabel();
        emit_goto(JMP, PopLabel());
        emit_label(i);
    } else {    /* for loop */
        p1 = RightChild(p);
        if (NodeOp(p1) == ToOp) {
            GenValueExp(LeftChild(p1));
            strcpy(temp1, ExpValue);

            if ((temp1[0] == 'R') || (temp1[0] == 'T')
               || (temp1[0] == '(') || (temp1[0] == '@')) 
                CurrentTemp--;

            location = GetAttr(IntVal(LeftChild(p)), PLACE_ATTR);
            if (location == GLOBAL) 
               sprintf(VarOffset, "%d", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            else if (location == LOCAL) 
               sprintf(VarOffset, "%d($fp)", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            else 
               sprintf(VarOffset, "%d($ra)", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            strcpy(temp2, VarOffset);
            op1.mode = IDENTIFIER;
            op1.ident = temp1;
            op2.mode = IDENTIFIER;
            op2.ident = temp2;
            emit_most(MOV, 'l', 2, op1, op2, op3);

            emit_label(LabelNum++);
            PushLabel(LabelNum-1);

            GenValueExp(RightChild(p1));
            strcpy(temp3, ExpValue);
            op1.mode = IDENTIFIER;
            op1.ident = temp2;
            op2.mode = IDENTIFIER;
            op2.ident = temp3;
            emit_most(CMP, 'l', 2, op1, op2, op3);
            if ((temp3[0] == 'R') || (temp3[0] == 'T')
                || (temp3[0] == '(') || (temp3[0] == '@'))
                  CurrentTemp--;
            emit_goto(BLEQ, LabelNum++);
            emit_goto(JMP, LabelNum++);
            PushLabel(LabelNum-1);
            emit_label(LabelNum-2);
            GenStmts(RightChild(T));
            op1.mode = NUM_CONST;
            op1.num_const = 1;
            op2.mode = IDENTIFIER;
            op2.ident = temp2;
            emit_most(ADD, 'l', 2, op1, op2,op3);
            i = PopLabel();
            emit_goto(JMP, PopLabel());
            emit_label(i);


	} else { /* downto */
            GenValueExp(LeftChild(p1));
            strcpy(temp1, ExpValue);

            if ((temp1[0] == 'R') || (temp1[0] == 'T')
                || (temp1[0] == '(') || (temp1[0] == '@')) 
                CurrentTemp--;


            location = GetAttr(IntVal(LeftChild(p)), PLACE_ATTR);
            if (location == GLOBAL) 
               sprintf(VarOffset, "%d", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            else if (location == LOCAL) 
               sprintf(VarOffset, "%d($fp)", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            else 
               sprintf(VarOffset, "%d($ra)", GetAttr(IntVal(LeftChild(p)),
                                             OFFSET_ATTR));
            strcpy(temp2, VarOffset);
            op1.mode = IDENTIFIER;
            op1.ident = temp1;
            op2.mode = IDENTIFIER;
            op2.ident = temp2;
            emit_most(MOV, 'l', 2, op1, op2, op3);

            emit_label(LabelNum++);
            PushLabel(LabelNum-1);
            GenValueExp(RightChild(p1));
            strcpy(temp3, ExpValue);
            op1.mode = IDENTIFIER;
            op1.ident = temp2;
            op2.mode = IDENTIFIER;
            op2.ident = temp3;
            emit_most(CMP, 'l', 2, op1, op2, op3);
            if ((temp3[0] == 'R') || (temp3[0] == 'T')
                || (temp3[0] == '(') || (temp3[0] == '@'))
                  CurrentTemp--;
            emit_goto(BGEQ, LabelNum++);
            emit_goto(JMP, LabelNum++);
            PushLabel(LabelNum-1);
            emit_label(LabelNum-2);
            GenStmts(RightChild(T));
            op1.mode = NUM_CONST;
            op1.num_const = 1;
            op2.mode = IDENTIFIER;
            op2.ident = temp2;
            emit_most(SUB, 'l', 2, op1, op2,op3);
            i = PopLabel();
            emit_goto(JMP, PopLabel());
            emit_label(i);


        }
    }
if (traceGen) printf("Leaving GenLoopStmt\n"); }            

void GenAsm (nd, depth)
tree nd;
int depth;
{
  int id, indx;

  if (!depth)
  {
    zerocrosses ();
  }
  if (IsNull (nd))
  {
    //indent (depth);
    fprintf (stderr,"Syntax Tree is Null.\n");
    return;
  }
  if (NodeKind (nd) == EXPRNode)
    GenAsm(RightChild (nd), depth + 1);
  //indent (depth); 
  switch (NodeKind (nd))
  {
    case IDNode:    
		    indx = IntVal(nd);
		    if (indx > 0)
		    {
		      id = indx; /*GetAttr(indx, NAME_ATTR); */
		      fprintf (stderr,"[IDNode,%d,\"%s\"]\n", IntVal(nd),
                                                    getname(id));
		    }
		    else 
		      fprintf (stderr,"[IDNode,%d,\"%s\"]\n", indx, "err");
		    break;

    case STNode:
                    indx = IntVal(nd);
                    if (indx > 0)
                    {
                      id = GetAttr(indx, NAME_ATTR);
                      fprintf (stderr,"[STNode,%d,\"%s\"]\n", IntVal(nd),
                                                    getname(id));
                    }
                    else 
                      fprintf (stderr,"[IDNode,%d,\"%s\"]\n", indx, "err");
                    break;

    case INTEGERTNode:
                      fprintf (stderr,"[INTEGERTNode]\n");
                    break;

    case NUMNode:   fprintf (stderr,"[NUMNode,%d]\n", IntVal (nd));
		    break;

    case CHARNode:  if (isprint (IntVal (nd)))
		      fprintf (stderr,"[CHARNode,%d,\'%c\']\n",
					 IntVal (nd), IntVal (nd));
		    else
		      fprintf (stderr,"[CHARNode,%d,\'\\%o\']\n",
					 IntVal (nd), IntVal (nd));
		    break;

    case STRINGNode:fprintf (stderr,"[STRINGNode,%d,\"%s\"]\n", IntVal (nd),
							getstring(IntVal(nd)));
		    break;

    //case EXPRNode:  fprintf (stderr,"[%s]\n", 
					//opnodenames [NodeOp(nd) - ProgramOp]);
//		    break;

    default:	    fprintf (stderr,"INVALID!!!\n");
		    break;
  }
  if (NodeKind (nd) == EXPRNode)
    printtree (LeftChild (nd), depth + 1);
}

