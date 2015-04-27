/* traverse.c
 * The original parse tree is traversed by ValProgramOp() in preorder using
 * codes similar to those in checktree().  A symbol table is built for all
 * identifiers and static semantics are checked while traversing.  All IDNodes
 * in the parse tree are changed to either type nodes (integer, char, boolean)
 * or STNodes. 
 */

#include <stdio.h>
#include "proj2.h"
#include "proj3.h"

/* the following type trees are built in seman.c when ST is initialized */
extern tree intTypeT, charTypeT, booleanTypeT, stringTypeT;
extern int traceST;      /* flag -- trace semantic analysis, set in driver3.c */

/* function prototypes */
extern tree DupTree();		/* imported from tree.c */
extern tree MakeLeaf();
extern tree MakeTree();
extern tree LeftChild();
extern tree RightChild();
extern tree NullExp();
extern int NodeKind();
extern void error_msg();     	/* from seman.c */
extern int InsertEntry();
extern void SetAttr();
extern int GetAttr();
extern int LookUp();
tree ValType();			/* from this program */
tree ValSpecOp();
tree ValFieldOp();
tree searchField();

/*****************************************************************************/
/*               V a l P r o g r a m O p ( )                                 */
/* ValProgramOp()  : start to process the syntax tree from the root          */
/* Tree will be checked on a top down fashion, looking for IDNode leaves to  */
/* do the appropriate symbol table insertions.                               */
/*****************************************************************************/

void ValProgramOp(T)
tree T;
{
    OpenBlock();
    if (!IsNull(LeftChild(T)))
       ValBodyOp(LeftChild(T));

    if (!IsNull(RightChild(T)))
       ValStmtOp(RightChild(T));
    CloseBlock();
    if (traceST) outp("exiting ProgramOp\n");
}

/****************************************************/
/*     V a l B o d y O p ( )                        */
/* recursively inorder traverses  a BodyOp node     */
/****************************************************/
ValBodyOp(T)
tree T;
{
    if (!IsNull(LeftChild(T)))
        ValBodyOp(LeftChild(T));

    ValDef(RightChild(T));
    if (traceST) outp("exiting BodyOp\n");
}

/******************************************************/
/*             V a l D e f ( )                        */
/* traverses in order a definition subtree            */
/******************************************************/
ValDef(T)
tree T;
{
    int op;

    op = NodeOp(T);
    if ((op == ProceOp) || (op == FuncOp))
        ValRoutineOp(T);
    else if (op == ConstantIdOp)
         ValConstantIdOp(T);
    else if (op == TypeIdOp)
        ValTypeIdOp(T);
    else if (op == DeclOp)
        ValDeclOp(T);
    if (traceST) outp("exiting Def\n");
}

/***************************************************************/
/*           V a l T y p e I d O P( )                          */
/* Process a type declaration subtree  . IDNode is to  the     */
/* child of T. The IDNode type is to the right of T            */
/***************************************************************/
ValTypeIdOp(T)
tree T;
{
  tree left,right, newNode;
  int entry, dim;

  left = LeftChild(T);
  right = RightChild(T);
  entry = InsertEntry(left->IntVal);

  /* change IDNode to STNode, but we can't do these:
   * 	left->NodeKind = STNode;
   * 	left->IntVal = entry;
   * because some single type IDNode serves as many leaves in the tree.
   * So a new leaf has to be created like this:
   */
  newNode = MakeLeaf(STNode, entry);
  SetLeftChild(T, newNode);

  if (entry) {
      SetAttr(entry, TREE_ATTR, newNode);
      SetAttr(entry, PREDE_ATTR, false);
      SetAttr(entry, KIND_ATTR, TYPEDEF);
      SetAttr(entry, TYPE_ATTR, ValType(right,&dim));
  }

  if (NodeKind(right) == IDNode)
      SetRightChild(T, MakeLeaf(STNode, LookUp(right->IntVal)));

  if (entry && dim) SetAttr(entry, DIMEN_ATTR, dim);
  if (traceST) outp("exiting TypeIdOp\n");
}

/****************************************************************/
/*              V a l D e c l O p ( )                           */
/* Inorder traverses a variable declaration subtree             */
/****************************************************************/
ValDeclOp(T)
tree T;
{
    tree p;

    p = LeftChild(T);
    if (!IsNull(p)) ValDeclOp(p);
    ValCommaOp(RightChild(T),VAR, NullExp());
    if (traceST) outp("exiting DeclOp\n");
}

/*****************************************************************************/
/*                V a l C o m m a O p ( )                                    */
/* A new entry is created in the symbol table for the variable stored as the */
/* the left child of the CommaOp node. 'kind' is used to differenciate       */
/* among variables, record fields, and formal parameters.            */
/*****************************************************************************/
ValCommaOp(T,kind, dup)
tree T, dup;
int kind;
{
    tree left, right, typeT, newNode;
    int entry, dim;

    left = LeftChild(T);
    right = RightChild(T);
    entry = InsertEntry(left->IntVal);
    newNode = MakeLeaf(STNode, entry);
    SetLeftChild(T, newNode);
    if (!IsNull(dup)) SetLeftChild(dup, newNode);
    typeT = ValType(right, &dim);
    if (!IsNull(dup)) SetRightChild(dup,typeT);
    if (entry) {
        SetAttr(entry, TREE_ATTR, newNode);
        SetAttr(entry, PREDE_ATTR, false);
        SetAttr(entry, KIND_ATTR, kind);
        SetAttr(entry, TYPE_ATTR, typeT);
    }
    if (NodeKind(right) == IDNode)
      SetRightChild(T, MakeLeaf(STNode, LookUp(right->IntVal)));

    if (entry && dim) SetAttr(entry, DIMEN_ATTR, dim);
    if (traceST) outp("exiting CommaOp\n");
}

/****************************************************************/
/*              V a l T y p e ( )                               */
/* Process a type subtree and add the appropriate information   */
/* to the symbol table. If the caller request setAttr, caller's */
/* TYPE_ATTR is set to the type tree.                           */
/****************************************************************/

tree ValType(T,dimension)
tree T;
int *dimension;
{
    tree typeT;
    int op, stid;

    *dimension = 0; 
    typeT = NullExp();
    op = NodeOp(T);
    if (op == ArrayTypeOp) {
	typeT = MakeTree(ArrayTypeOp, NullExp(), NullExp());
        *dimension = ValArrayTypeOp(T, typeT);
    }
    else if (op == RecompOp) {
        OpenBlock();
	typeT = DupTree(T);
        ValRecompOp(T, typeT);
        CloseBlock();
    }
    else if (op == SubrangeOp) {
        ValSubrangeOp(T);
	typeT = T;
    }
    else {
      if (NodeKind(T) == IDNode) 
          stid = LookUp(T->IntVal);
      else if (NodeKind(T) == STNode)
	  stid = T->IntVal;
      else {
	  stid = 0;
          printf("Something wrong in ValType()\n");
      }
      if (stid) {   				/* found in symbol table */
        //if (GetAttr(stid,KIND_ATTR) != TYPE)
            //error_msg(NOT_TYPE, CONTINUE, T->IntVal, 0);
        typeT = (tree) GetAttr(stid,TYPE_ATTR);
        if (NodeOp(typeT) == ArrayTypeOp)
	    *dimension = GetAttr(stid,DIMEN_ATTR);
      }
    }
    if (traceST) outp("exiting Type\n");
    return typeT;
}

/*****************************************************************************/
/*                V a l A r r a y T y p e O p ( )                            */
/* Process an array type subtree and return the number of dimensions of the  */
/* array                                                                     */
/*****************************************************************************/
int ValArrayTypeOp(T,typeTree)
tree T, typeTree;
{
    tree left, right;
    int dimensions, dummy;

    left = LeftChild(T);
    dimensions = ValBoundOp(left);
    SetLeftChild(typeTree, left);

    right = RightChild(T);
    SetRightChild(typeTree, ValType(right,&dummy));
    if (NodeKind(right) == IDNode)
      SetRightChild(T, MakeLeaf(STNode, LookUp(right->IntVal)));

    if (traceST) outp("exiting ArrayTypeOp\n");

    return dimensions;
}

/*****************************************************************************/
/*                 V a l  B o u n d O p ( )                                  */
/* Semantic Check: Identifiers used as array bounds must be already in the   */
/*                 symbol table                                              */
/*****************************************************************************/

int ValBoundOp(T)
tree T;
{
    int dimensions;
    tree left;

    left = LeftChild(T);
    if (!IsNull(left))
       dimensions = (ValBoundOp(left) + 1);
    else
       dimensions = 1;

    ValSubrangeOp(RightChild(T));

    if (traceST) outp("exiting BoundOp\n");

    return dimensions;
}

/***************************************************/
/*          V a l  R e c o m p O p ( )             */
/* Process a record type subtree                   */
/***************************************************/
ValRecompOp(T, typeTree)
tree T, typeTree;
{
    tree p, q;

    p = LeftChild(T); q = LeftChild(typeTree);
    if (!IsNull(p)) ValRecompOp(p, q);
    ValCommaOp(RightChild(T),FIELD, RightChild(typeTree));
    if (traceST) outp("exiting RecompOp\n");
}

/*****************************************************************************/
/*             V a l S u b r a n g e O p ( )                                 */
/* ValSubrangeOp: process a subrange type subtree                            */
/*****************************************************************************/
ValSubrangeOp(T)
tree T;
{ 
    tree left, right;
    int entry;

    left = LeftChild(T);
    if (NodeKind(left) == IDNode )
      if (entry = LookUp(left->IntVal)) {   /* found in ST */
        //if (GetAttr(entry,KIND_ATTR) != CONSTANT ||
	//    GetAttr(entry,TYPE_ATTR) != (int)intTypeT) 
          //error_msg(NOT_INT_CONSTANT, CONTINUE, left->IntVal, 0);
        SetLeftChild(T, MakeLeaf(STNode, entry));
      }

    right = RightChild(T);
    if (NodeKind(right) == IDNode )
      if (entry = LookUp(right->IntVal)) {   /* found in ST */
        //if (GetAttr(entry,KIND_ATTR) != CONSTANT ||
	//    GetAttr(entry,TYPE_ATTR) != (int)intTypeT) 
          //error_msg(NOT_INT_CONSTANT, CONTINUE, right->IntVal, 0);
        SetRightChild(T, MakeLeaf(STNode, entry));
    }
    if (traceST) outp("exiting SubrangeOp\n");
}

/*****************************************************************************/
/*                    V a l C o n s t a n t I d O p ( )                      */
/* Creates a new symbol table entry for the constant                         */
/*****************************************************************************/
ValConstantIdOp(T)
tree T;
{
    int entry, kind;
    tree left, right, typeT, newNode;

    left = LeftChild(T);
    right = RightChild(T);
    entry = InsertEntry(left->IntVal);
    newNode = MakeLeaf(STNode, entry);
    SetLeftChild(T, newNode);
    
    kind = NodeKind(right);
    if (kind == NUMNode)
       typeT = intTypeT;
    else if (kind == CHARNode)
       typeT = charTypeT;
    else if (kind == STRINGNode)
       typeT = stringTypeT;
    else printf("Something is wrong in const def. Not const nodes.\n");

    if (entry) {
        SetAttr(entry, TREE_ATTR, newNode);
        SetAttr(entry, PREDE_ATTR, false);
    	SetAttr(entry, TYPE_ATTR, typeT);
    	SetAttr(entry, KIND_ATTR, CONST);
    	SetAttr(entry, VALUE_ATTR, right->IntVal);
	if (typeT == stringTypeT)
    	    SetAttr(entry, DIMEN_ATTR, 1);
    }
    if (traceST) outp("exiting ConstantIdOp\n");
}

/*****************************************************************************/
/*         V a l R o u t i n e O p ( )                                       */
/* Process a function or procedure declaration subtree                       */
/*****************************************************************************/
ValRoutineOp(T)
tree T;
{
    tree right;
    bool isForward;

    right = RightChild(T);
    if (IsNull(right))
        isForward = true;
    else
        isForward = false;

    ValHeadOp(LeftChild(T), T, isForward);

    right = RightChild(T);
    if (NodeOp(right) == BodyOp) {
       if(!IsNull(LeftChild(right)))
         ValBodyOp(LeftChild(right));

       if (NodeOp(RightChild(right)) == StmtOp)
         ValStmtOp(RightChild(right));
    }
    CloseBlock();

    if (traceST) outp("exiting RoutineOp\n");
}

/**********************************************************************/
/*                         V a l H e a d O p( )                       */
/* Inserts  the function in the symbol table and process the  formal  */
/* parameter list                                                     */
/**********************************************************************/
ValHeadOp(T,root,forward)
tree T, root;
bool forward;           /* true if the RoutineOp is a forward declaration  */
{
    tree left, right, typeT, oriRoot, oriSpecT;
    int entry, kind;
    bool reDefForward = false;  /* true if real def for a previous forward */

    left = LeftChild(T);
    right = RightChild(T);
    /* check to see if a forward declaration existed */
    if ((entry = LookUpHere(left->IntVal)) && 
        ((kind = GetAttr(entry, KIND_ATTR)) == PROCE || kind == FUNC) && 
	GetAttr(entry, FORWD_ATTR)) 		/* forward existed */
       	reDefForward = true;
    else
       if (entry = InsertEntry(left->IntVal)) {
          if (NodeOp(root) == ProceOp)
             kind = PROCE;
          else if (NodeOp(root) == FuncOp)
             kind = FUNC;
          else printf("Something is wrong with routine declaration\n");

          SetAttr(entry, TREE_ATTR, root);
          SetAttr(entry, PREDE_ATTR, false);
          SetAttr(entry, KIND_ATTR, kind);
          SetAttr(entry, TYPE_ATTR, root);
          if (forward)
              SetAttr(entry, FORWD_ATTR, true);
          else
              SetAttr(entry, FORWD_ATTR, false);
       }
    SetLeftChild(T, MakeLeaf(STNode, entry));
    OpenBlock();

    typeT = ValSpecOp(right);
/*    if (entry && (typeT != NullExp()) && !reDefForward)
           SetAttr(entry, TYPE_ATTR, typeT);
*/
    if (reDefForward) {
	oriRoot = (tree)GetAttr(entry, TREE_ATTR);
      	oriSpecT = RightChild(LeftChild(oriRoot));
	if (NodeOp(oriRoot) != NodeOp(root) || 
	    !IsSameTree(right, oriSpecT)) {
          //error_msg(FORWD_MISMATCH, CONTINUE, GetAttr(entry, NAME_ATTR));
          return;
        }
       	ChangeAttr(entry, FORWD_ATTR, false);      /* no longer forward */
        SetAttr(entry, TREE_ATTR, root);        /* set to new tree root */
    }
    if (traceST) outp("exiting HeadOp\n");
}

/**********************************************************************/
/*                   V a l S p e c O p ( )                            */
/**********************************************************************/
tree ValSpecOp(T)
tree T;
{
  tree left, right, typeT;
  int dummy;

  left = LeftChild(T);
  right = RightChild(T);
  if(!IsNull(left))
    ValArgs(left);
  if(!IsNull(right)) {
    typeT = ValType(right, &dummy);
    if (NodeKind(right) == IDNode)
      SetRightChild(T, MakeLeaf(STNode, LookUp(right->IntVal)));
  }
  else typeT = NullExp();

  if (traceST) outp("exiting SpecOp\n");
  return typeT;
}

/******************************************************************************/
/*                               V a l A r g s ( )                            */
/* Processes the formal parameter list for a procedure or function            */
/******************************************************************************/
ValArgs(T)
tree T;
{
    tree p, leftright;
    int op;

    p = T;
    while (!IsNull(p)) {
    	if (NodeOp(p) == VArgTypeOp)
            ValCommaOp(LeftChild(p),VALUE_ARG, NullExp());
    	else
            ValCommaOp(LeftChild(p),REF_ARG, NullExp());
	p = RightChild(p);
    }

    if (traceST) outp("exiting Args\n");
}

/****************************************************************************/
/*              V a l S t m t O p ( )                                       */
/* process a sequence of statements subtree                                 */
/****************************************************************************/
ValStmtOp(T)
tree T;
{
     if (!IsNull(LeftChild(T)))
          ValStmtOp(LeftChild(T));
     ValStmt(RightChild(T));
    if (traceST) outp("exiting StmtOp\n");
}

/****************************************************************************/
/*             V a l S t m t ( )                                            */
/* Process a statement subtree. which can be either an IfStmtOp             */
/* Statement subtrees     can be ifStmtOP,LoopStmtOP, AssignStmtOp, StmtOp  */
/*                        ExitOp, ReturnOp                                  */
/****************************************************************************/
ValStmt(T)
tree T;
{
    int op;

    /* no need to process exit stmt */
    op = NodeOp(T);
    if (op == LoopOp)
        ValLoopOp(T);
    else if (op == IfElseOp)
        ValIfElseOp(T);
    else if (op == RoutineCallOp)
        ValRoutineCallOp(T);
    else if (op == AssignOp)
        ValAssignOp(T);
    else if (op == ReturnOp)
        ValReturnOp(T);
    else if (op == StmtOp)
        ValStmtOp(T);
    if (traceST) outp("exiting Stmt\n");
}

/****************************************************************************/
/*               V a l I f E l s e O P ( )                                  */
/* Process an  ifelse statement subtree                                     */
/****************************************************************************/
ValIfElseOp(T)
tree T;
{
    tree temp;

    temp = LeftChild(T);

    ValIfElse(temp);

    temp = RightChild(T);

    if(!IsNull(temp))
      ValStmtOp(temp);
    if (traceST) outp("exiting IfElseOp\n");
}

/****************************************************************************/
/*               V a l I f E l s e  ( )                                     */
/* Process an  ifelse statement subtree                                     */
/****************************************************************************/
ValIfElse(T)
tree T;
{
  tree temp;

  temp = LeftChild(T);
  if(!IsNull(temp))
    ValIfElse(temp);
  temp = RightChild(T);
  ValExp(LeftChild(temp));
  ValStmtOp(RightChild(temp));
  if (traceST) outp("exiting IfElse\n");
}

/**********************************************************************/
/*                     V a l L o o p O p ( )                          */
/* cases                                                              */
/*     o for loops                                                    */
/*     o repeat loops                                                 */
/*     o while  loops                                                 */
/**********************************************************************/
ValLoopOp(T)
tree T;
{
    tree left, leftleft, typeT;
    int entry, op, kind;

    left = LeftChild(T);
    op = NodeOp(left);
    if (op == CommaOp) {			/* for loop */
       leftleft = LeftChild(left);
       entry = LookUp(leftleft->IntVal);
       if (entry) {
         SetLeftChild(left, MakeLeaf(STNode, entry));
    	 typeT = (tree)GetAttr(entry, TYPE_ATTR);
    	 kind = GetAttr(entry, KIND_ATTR);
    	 //if (kind != VARIABLE && kind != REF_ARG && kind != VALUE_ARG ||
	  //   typeT != intTypeT && typeT != charTypeT &&
	  //   NodeOp(typeT) != SubrangeOp)
      		//error_msg(LOOP_VAR, CONTINUE, GetAttr(entry, NAME_ATTR));
       }

       ValIterOp(RightChild(left));

       ValStmtOp(RightChild(T));
   }
   else if (op == StmtOp) {			/* repeat loop */
      ValStmtOp(left);
      ValExp(RightChild(T));
   }
   else {					/* while loop */
      ValExp(left);
      ValStmtOp(RightChild(T));
   }
   if (traceST) outp("exiting LoopOp\n");
}

/**********************************************************************/
/*                      V a l I t e r O p ( )                         */
/* Processes an iteractor subtree                                     */
/**********************************************************************/
ValIterOp(T)
tree T;
{
    ValExp(LeftChild(T));
    ValExp(RightChild(T));
    if (traceST) outp("exiting IterOp\n");
}

/**********************************************************************/
/*                  V a l R e t u r n O p ( )                         */
/* Process a return subtree                                           */
/**********************************************************************/
ValReturnOp(T)
tree T;
{
    if(!IsNull(LeftChild(T)))
       ValExp(LeftChild(T));
    if (traceST) outp("exiting ReturnOp\n");
}

/**********************************************************************/
/*                    V a l A s s i g n O p ( )                       */
/*Process an assignment subtree                                       */
/**********************************************************************/
ValAssignOp(T)
tree T;
{
    tree temp;
    int func;

    ValAssign(LeftChild(T));

    ValExp(RightChild(T));
    if (traceST) outp("exiting AssignOp\n");
}

/**********************************************************************/
/*                    V a l A s s i g n ( )                           */
/*Process an assignment subtree; left var shouldn't be a constant     */
/**********************************************************************/
ValAssign(T)
tree T;
{
  int entry,kind;

  if(!IsNull(LeftChild(T)))
    ValAssign(LeftChild(T));
  entry = ValVarOp(RightChild(T));
  if (entry) {
    kind = GetAttr(entry, KIND_ATTR);
    //if (kind != VARIABLE && kind != REF_ARG && kind != VALUE_ARG)
      //error_msg(ASSIGN_ERR, CONTINUE, GetAttr(entry, NAME_ATTR));
  }
  if (traceST) outp("exiting Assign\n");
}

/**********************************************************************/
/*                  V a l R o u t i n e C a l l O p ( )               */
/* Process a routine call subtree                                     */
/**********************************************************************/
ValRoutineCallOp(T)
tree T;
{
    tree left,specT,paraT;
    int entry;
    bool paraCheck=true;

    left = LeftChild(T);
    entry = LookUp(left->IntVal);
    if (entry) {
       SetLeftChild(T, MakeLeaf(STNode, entry));
       specT = RightChild(LeftChild(GetAttr(entry,TREE_ATTR)));
       if (IsNull(specT) && GetAttr(entry, PREDE_ATTR))
	 paraCheck = false;      /* no parameter check for read() and write() */
       else if (IsNull(specT))
	 paraT = NullExp();
       else
	 paraT = LeftChild(specT);
    }

    ValCommaInCall(RightChild(T), paraT, left->IntVal, paraCheck);
    if (traceST) outp("exiting RoutineCallOp\n");
}

/**********************************************************************/
/*                  V a l C o m m a I n C a l l( )                    */
/* Process the list of actual parameters in a function or procedure   */
/* Semantic Check : Formal and actual number of parameters should be  */
/*                  the same; variables are expected for reference    */
/*		    parameters. If check is false, no check is done.  */
/**********************************************************************/
ValCommaInCall(T, paraT, id, check)
tree T, paraT;
int id;
bool check;
{
    tree left;

    if (check && 
	(IsNull(T) && !IsNull(paraT) || !IsNull(T) && IsNull(paraT))) {
	//error_msg(PARA_MISMATCH, CONTINUE, id);
	check = false;	/* turn off checking to avoid multiple error msgs */
    }
    if (!IsNull(T)) {
    	left = LeftChild(T);
    	ValExp(left);
	if (check && !IsNull(paraT)) {
	  if (NodeOp(paraT) == RArgTypeOp && NodeOp(left) != VarOp) {
	    //error_msg(REF_ARG_MISMATCH, CONTINUE, id);
	    check = false;
	  }
	  paraT = RightChild(paraT);
	}
        if(!IsNull(RightChild(T)))
          ValCommaInCall(RightChild(T), paraT, id, check);
    }

    if (traceST) outp("exiting CommaInCall\n");
}

/**********************************************************************/
/*                  V a l E x p ( )                                   */
/* Process an expression subtree                                      */
/**********************************************************************/
ValExp(T)
tree T;
{
    int op;

    switch (NodeKind(T)) {
          case STRINGNode:
          case CHARNode:
          case NUMNode:
               break;
          case EXPRNode:
               op = NodeOp(T);
               if ((op ==AddOp) || (op== SubOp) || (op==MultOp)
                     || (op==DivOp) || (op==LTOp) || (op==GTOp)
                     || (op==EQOp) || (op==AndOp) || (op==OrOp)
                     || (op==LEOp) || (op==NEOp) || (op==GEOp)) {
                     ValExp(LeftChild(T));
                     ValExp(RightChild(T));
               }
               else if ((op==UnaryNegOp) || (op==NotOp))
                       ValExp(LeftChild(T));
               else if (op == RoutineCallOp)
                       ValRoutineCallOp(T);
               else if (op == VarOp)
                       ValVarOp(T);
               break;
    }
    if (traceST) outp("exiting Exp\n");
}

/**********************************************************************/
/*                  V a l V a r O p ( )                               */
/* process a variable reference subtree, change it to routine call    */
/* and process it as a routine call if it is the case                 */
/**********************************************************************/
int ValVarOp(T)
tree T;
{
    int entry, kind;
    tree left;

    left = LeftChild(T);
    entry = LookUp(left->IntVal);
      
    if (entry) {
       kind = GetAttr(entry, KIND_ATTR);
       switch (kind) {
	   case PROCE:
	       //error_msg(PROC_AS_VAR, CONTINUE, GetAttr(entry, NAME_ATTR));
	   case FUNC:
	       varOpToRoutineCallOp(T);
	       ValRoutineCallOp(T);
    	       if (traceST) outp("exiting VarOp\n");
               return entry;
	       break;
	   case TYPEDEF:
	       //error_msg(TYPE_AS_VAR, CONTINUE, GetAttr(entry, NAME_ATTR));
	       break;
       }
       SetLeftChild(T, MakeLeaf(STNode, entry));
       /* if it's a array or record, continue to process the rest of the tree */
       if (!IsNull(RightChild(T)))
          ValSelectOp(RightChild(T), entry, GetAttr(entry, TYPE_ATTR));
    }

    if (traceST) outp("exiting VarOp\n");

    return entry;
}

/**********************************************************************/
/*                  V a l S e l e c t O p ( )                         */
/* process indexing and field operations                              */
/**********************************************************************/
ValSelectOp(T,entry, currentType)
tree T, currentType;
int entry;
{
   tree left, typeT;
   int dimensions, op;

   typeT = NullExp();
   left = LeftChild(T);
   op = NodeOp(left);
   if (op == IndexOp) {
       if (NodeOp(currentType) == ArrayTypeOp) {
            dimensions = ValIndexOp(left);
	    if (dimensions != countArrayDim(currentType))
		//error_msg(ARR_DIME_MIS, CONTINUE, GetAttr(entry, NAME_ATTR), 0);
            typeT = RightChild(currentType);
       }
       //else //error_msg(ARR_TYPE_MIS, CONTINUE, GetAttr(entry, NAME_ATTR), 0);
   }
   else if(op == FieldOp) {
         if (NodeOp(currentType) == RecompOp)
       	      typeT = ValFieldOp(left, currentType); 
	 //else //error_msg(NOT_RECORD, CONTINUE, GetAttr(entry, NAME_ATTR), 0);
   }
   else printf("Something is wrong with SelectOp\n");
   
   if(!IsNull(RightChild(T)))
        ValSelectOp(RightChild(T),entry, typeT);
   if (traceST) outp("exiting SelectOp\n");
}

/**********************************************************************/
/*                  V a l I n d e x ( )                               */
/**********************************************************************/
int ValIndexOp(T)
tree T;
{
    int dimensions;

    ValExp(LeftChild(T));

    if (!IsNull(RightChild(T)))
        dimensions = ValIndexOp(RightChild(T)) + 1;
    else
        dimensions = 1;

    if (traceST) outp("exiting IndexOp\n");

    return dimensions;
}

/**********************************************************************/
/*                  V a l F i e l d O p ( )                           */
/* process a record accesing operation                                */
/**********************************************************************/
tree ValFieldOp(T,recType)
tree T,recType;
{
    tree left, nextType, commaOpTree;
    int entry;

    nextType = NullExp();
    left = LeftChild(T);
    commaOpTree = searchField(recType, left->IntVal);
    if (!IsNull(commaOpTree)) {	/* valid field name */
	entry = IntVal(LeftChild(commaOpTree));
        SetLeftChild(T, MakeLeaf(STNode, entry));
	nextType = RightChild(commaOpTree);
    } //else //error_msg(NOT_FIELD, CONTINUE, left->IntVal, 0);
    if (traceST) outp("exiting FieldOp\n");
    return nextType;
}

outp(s)
char s[];
{
  printf(s);
}

/****************************************************************/
/*         v a r O p T o R o u t i n e C a l l O p ( )          */
/* change a varOp to RoutineCallOp                              */
/****************************************************************/

varOpToRoutineCallOp(T)
tree T;
{
    tree right;

    if (NodeOp(T) == VarOp) {
      if (!IsNull(right = RightChild(T))) {
	SetRightChild(T, LeftChild(right));
	SetRightTreeOp(RightChild(T), CommaOp);
	//if (!IsNull(RightChild(right)))
	  //error_msg(ROUTINE_CALL_ERR, CONTINUE, IntVal(LeftChild(T)), 0);
      }
      SetNodeOp(T, RoutineCallOp);
    }
    else printf("varOpToRoutineCallOp() can only be called on VarOp node\n");
}

int countArrayDim(arrayType)
tree arrayType;
{
    tree p;
    int count = 0;

    if (NodeOp(arrayType) == ArrayTypeOp) {
	p = LeftChild(arrayType);
	while (!IsNull(p)) {
	    count++;
	    p = LeftChild(p);
	}
    } else printf("countArrayDim() can only be called on ArrayTypeOp\n");

    if (!count) printf("Something is wrong, array has zero dimension\n");
    return count;
}

tree searchField(recType, id)
tree recType;
int id;
{
    tree p, commaOpT;

    p = recType;
    while (NodeOp(p) == RecompOp) {
      	commaOpT = RightChild(p);
/*      	if (id == IntVal(LeftChild(commaOpT)))*/
      	if (id == GetAttr(IntVal(LeftChild(commaOpT)), NAME_ATTR))
	    return commaOpT;
	p = LeftChild(p);
    }
    return NullExp();
}
