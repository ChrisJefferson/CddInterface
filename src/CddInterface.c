/*
 * CddInterface: Gap interface to Cdd package
 */

#include "src/compiled.h" /* GAP headers */
#include "../current_cddlib/lib-src/setoper.h"
#include "../current_cddlib/lib-src/cdd.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

/**********************************************************
*
*    Auxiliary functions to be used inside C 
* 
* ********************************************************/

extern void dd_SetLinearity(dd_MatrixPtr, char *);

// Old implementation
// Obj MPZ_TO_GAPOBJ(mpz_t x)
// {
//   //gmp_printf ("%s is an mpz %Zd\n", "here", x);
//   return INTOBJ_INT(mpz_get_si(x));
// }

// The following conversion has been taken from
// https://github.com/gap-packages/NormalizInterface
// Thanks to Max Horn
static Obj MPZ_TO_GAPOBJ( const mpz_t x)
{
    Obj res;
    Int size = x->_mp_size;
    int sign;
    
    if (size == 0) {
        return INTOBJ_INT(0);
    } else if (size < 0) {
        size = -size;
        sign = -1;
    } else {
        sign = +1;
    }
    if (size == 1) {
        res = ObjInt_UInt(x->_mp_d[0]);
        if (sign < 0)
            res = AInvInt(res);
    } else {
        size = sizeof(mp_limb_t) * size;
        if (sign > 0)
            res = NewBag(T_INTPOS, size);
        else
            res = NewBag(T_INTNEG, size);
        memcpy(ADDR_INT(res), x->_mp_d, size);
    }
    return res;
}

static Obj MPQ_TO_GAPOBJ(const mpq_t x)
{
  Obj num = MPZ_TO_GAPOBJ(mpq_numref(x));
  Obj den = MPZ_TO_GAPOBJ(mpq_denref(x));
  return QUO(num, den);
}

/**********************************************************
*
*    Converting functions
* 
* ********************************************************/

static Obj ddG_LinearityPtr(dd_MatrixPtr M)
{
  dd_rowrange r;
  dd_rowset s;
  int i;

  r = M->rowsize;
  s = M->linset;

  Obj current = NEW_PLIST(T_PLIST, 16);
  for (i = 1; i <= r; i++)
    if (set_member(i, s))
      AddPlist(current, INTOBJ_INT(i));

  return current;
}

static Obj ddG_InteriorPoint(dd_MatrixPtr M)
{
  dd_rowset R, S;
  dd_rowset LL, ImL, RR, SS, Lbasis;
  dd_LPSolutionPtr lps = NULL;
  dd_ErrorType err;
  long int j, dim;

  set_initialize(&R, M->rowsize);
  set_initialize(&S, M->rowsize);
  set_initialize(&LL, M->rowsize);
  set_initialize(&RR, M->rowsize);
  set_initialize(&SS, M->rowsize);
  set_copy(LL, M->linset); /* rememer the linset. */
  set_copy(RR, R);         /* copy of R. */
  set_copy(SS, S);         /* copy of S. */

  Obj result;

  if (dd_ExistsRestrictedFace(M, R, S, &err))
  {
    set_uni(M->linset, M->linset, R);
    dd_FindRelativeInterior(M, &ImL, &Lbasis, &lps, &err);
    dim = M->colsize - set_card(Lbasis) - 1;
    set_uni(M->linset, M->linset, ImL);

    result = NEW_PLIST(T_PLIST_EMPTY, lps->d);
    ASS_LIST(result, 1, INTOBJ_INT(dim));
    for (j = 1; j < (lps->d) - 1; j++)
    {
      ASS_LIST(result, j + 1, MPQ_TO_GAPOBJ(lps->sol[j]));
    }

    dd_FreeLPSolution(lps);
    set_free(ImL);
    set_free(Lbasis);
  }
  else
  {
    result = NEW_PLIST(T_PLIST_EMPTY, 1);
    ASS_LIST(result, 1, INTOBJ_INT(-1));
  }

  set_copy(M->linset, LL); /* restore the linset */
  set_free(LL);
  set_free(RR);
  set_free(SS);

  return result;
}

static Obj MatPtrToGapObj(dd_MatrixPtr M)
{
  Obj current, result;
  dd_Amatrix Ma;
  dd_rowrange nrRows = M->rowsize;
  dd_colrange nrCols = M->colsize;

  //dd_WriteMatrix(stdout, M);
  result = NEW_PLIST(T_PLIST_CYC, 7);

  // reading the representation of M
  ASS_LIST(result, 1, INTOBJ_INT(M->representation));

  // reading the number type
  ASS_LIST(result, 2, INTOBJ_INT(M->numbtype));

  // entry 3 is left unbound on purpose

  ASS_LIST(result, 4, INTOBJ_INT(nrRows));
  ASS_LIST(result, 5, INTOBJ_INT(nrCols));
  ASS_LIST(result, 6, ddG_LinearityPtr(M));

  Ma = M->matrix;

  current = NEW_PLIST(T_PLIST_CYC, nrRows);

  for (int i = 0; i < nrRows; i++)
  {
    Obj row = NEW_PLIST(T_PLIST_CYC, nrCols);
    ASS_LIST(current, i+1, row);
    for (int j = 0; j < nrCols; j++)
    {
      ASS_LIST(row, j+1, MPQ_TO_GAPOBJ(Ma[i][j]));
    }
  }

  ASS_LIST(result, 7, current);
  return result;
}

static dd_MatrixPtr GapInputToMatrixPtr(Obj input)
{

  int k_rep, k_linearity, k_rowrange, k_colrange, k_LPobject;
  char k_linearity_array[dd_linelenmax], k_rowvec[dd_linelenmax];

  // reset the global variable, before defining it again to be used in the current session.
  dd_set_global_constants();

  k_rep = INT_INTOBJ(ELM_PLIST(input, 1));
  k_linearity = INT_INTOBJ(ELM_PLIST(input, 3));
  k_rowrange = INT_INTOBJ(ELM_PLIST(input, 4));
  k_colrange = INT_INTOBJ(ELM_PLIST(input, 5));
  k_LPobject = INT_INTOBJ(ELM_PLIST(input, 8));
  Obj string = ELM_PLIST(input, 7);
  if (k_colrange == 0)
    ErrorMayQuit("k_colrange == 0 should not happen, please report this!", 0, 0);

  int str_len = GET_LEN_STRING(string);
  //fprintf(stdout, "%d: ", str_len);
  //ErrorMayQuit( "j", 0, 0 );

  char k_matrix[str_len];
  strcpy(k_linearity_array, CSTR_STRING(ELM_PLIST(input, 6)));
  strcpy(k_matrix, CSTR_STRING(ELM_PLIST(input, 7)));
  strcpy(k_rowvec, CSTR_STRING(ELM_PLIST(input, 9)));

  char k_value[dd_linelenmax];
  char *pch;
  int u;
  dd_MatrixPtr M = NULL;
  mytype rational_value;

  // // creating the matrix with these two dimesnions
  M = dd_CreateMatrix(k_rowrange, k_colrange);
  // controling if the given representation is H or V.
  if (k_rep == 2)
    M->representation = dd_Generator;
  else if (k_rep == 1)
    M->representation = dd_Inequality;
  else
    M->representation = dd_Unspecified;

  //
  // controling the numbertype in the matrix
  M->numbtype = dd_Rational;

  //
  //  controling the linearity of the given polygon.
  if (k_linearity == 1)
  {
    dd_SetLinearity(M, k_linearity_array);
  }
  //
  // // filling the matrix with elements scanned from the string k_matrix
  //
  
  pch = strtok(k_matrix, " ,.{}][");
  int uu,vv;

  for (uu = 0; uu < k_rowrange; uu++){
  for (vv = 0; vv < k_colrange; vv++){
  	//fprintf(stdout, "uu:%d: ", uu );
  	//fprintf(stdout, "vv:%d: ", vv );

    	strcpy(k_value, pch);
    	dd_init(rational_value);
    	dd_sread_rational_value(k_value, rational_value);
    	dd_set(M->matrix[uu][vv], rational_value);
    	dd_clear(rational_value);
    	pch = strtok(NULL, " ,.{}][");
  }
  } 

  if (k_LPobject == 0)
    M->objective = dd_LPnone;
  else if (k_LPobject == 1)
    M->objective = dd_LPmax;
  else
    M->objective = dd_LPmin;

  if (M->objective == dd_LPmax || M->objective == dd_LPmin)
  {
    pch = strtok(k_rowvec, " ,.{}][");
    for (u = 0; u < M->colsize; u++)
    {
      strcpy(k_value, pch);
      dd_init(rational_value);
      dd_sread_rational_value(k_value, rational_value);
      dd_set(M->rowvec[u], rational_value);
      dd_clear(rational_value);
      pch = strtok(NULL, " ,.{}][");
    }
  }

  return M;
}

static Obj FaceWithDimAndInteriorPoint(dd_MatrixPtr N, dd_rowset R, dd_rowset S, dd_colrange mindim)
{
  dd_ErrorType err;
  dd_rowset LL, ImL, RR, SS, Lbasis;
  dd_rowrange iprev = 0;
  dd_colrange j, dim;
  dd_LPSolutionPtr lps = NULL;
  Obj result, current2, result_2;
  dd_MatrixPtr M;

  M = dd_CopyMatrix(N);

  set_initialize(&LL, M->rowsize);
  set_initialize(&RR, M->rowsize);
  set_initialize(&SS, M->rowsize);
  set_copy(LL, M->linset);
  set_copy(RR, R);
  set_copy(SS, S);

  if (dd_ExistsRestrictedFace(M, R, S, &err))
  {

    result = NEW_PLIST(T_PLIST_CYC, 3);

    set_uni(M->linset, M->linset, R);
    dd_FindRelativeInterior(M, &ImL, &Lbasis, &lps, &err);
    dim = M->colsize - set_card(Lbasis) - 1;
    set_uni(M->linset, M->linset, ImL);

    ASS_LIST(result, 1, INTOBJ_INT(dim));

    int i;
    Obj r;

    ASS_LIST(result, 2, ddG_LinearityPtr(M));

    size_t n;
    n = (lps->d) - 2;
    current2 = NEW_PLIST((n > 0) ? T_PLIST_CYC : T_PLIST, n);
    for (j = 1; j <= n; j++)
    {
      ASS_LIST(current2, j, MPQ_TO_GAPOBJ(lps->sol[j]));
    }

    ASS_LIST(result, 3, current2);

    dd_FreeLPSolution(lps);
    set_free(ImL);
    set_free(Lbasis);

    if (dim > mindim)
    {

      result_2 = NEW_PLIST(T_PLIST_CYC, 1 + M->rowsize);
      ASS_LIST(result_2, 1, result);

      for (i = 1; i <= M->rowsize; i++)
      {
        if (!set_member(i, M->linset) && !set_member(i, S))
        {
          set_addelem(RR, i);
          if (iprev)
          {
            set_delelem(RR, iprev);
            set_delelem(M->linset, iprev);
            set_addelem(SS, iprev);
          }
          iprev = i;
          r = FaceWithDimAndInteriorPoint(M, RR, SS, mindim);
          ASS_LIST(result_2, i + 1, r);
        }
        else
        {
          ASS_LIST(result_2, i + 1, INTOBJ_INT(2019));
        }
      }


      return result_2;
    }
    else
    {


      return result;
    }
  }
  else
  {
    set_copy(M->linset, LL);
    set_free(LL);
    set_free(RR);
    set_free(SS);

    return INTOBJ_INT(2019);
  }
}

/**********************************************************
*
*    functions to be called from Gap 
* 
* ********************************************************/

static Obj CddInterface_FourierElimination(Obj self, Obj main)
{
  dd_MatrixPtr M, A, G;
  dd_PolyhedraPtr poly;
  dd_ErrorType err;
  err = dd_NoError;
  dd_set_global_constants();

  M = GapInputToMatrixPtr(main);
  A = dd_FourierElimination(M, &err);
  poly = dd_DDMatrix2Poly(A, &err);
  G = dd_CopyInequalities(poly);
  return MatPtrToGapObj(G);
}

static Obj CddInterface_DimAndInteriorPoint(Obj self, Obj main)
{
  dd_MatrixPtr M;
  Obj result;

  dd_PolyhedraPtr poly;
  dd_ErrorType err;
  err = dd_NoError;
  dd_set_global_constants();

  M = GapInputToMatrixPtr(main);
  //dd_WriteMatrix( stdout, M );
  poly = dd_DDMatrix2Poly(M, &err);
  M = dd_CopyInequalities(poly);
  //dd_WriteMatrix( stdout, M );

  result = ddG_InteriorPoint(M);

  dd_free_global_constants();

  return result;
}

static Obj CddInterface_Canonicalize(Obj self, Obj main)
{
  dd_MatrixPtr M;
  dd_set_global_constants();
  M = GapInputToMatrixPtr(main);
  dd_rowset impl_linset, redset;
  dd_rowindex newpos;
  dd_ErrorType err;
  dd_MatrixCanonicalize(&M, &impl_linset, &redset, &newpos, &err);
  dd_free_global_constants();
  return MatPtrToGapObj(M);
}

static Obj CddInterface_Compute_H_rep(Obj self, Obj main)
{
  dd_MatrixPtr M, A;
  dd_PolyhedraPtr poly;
  dd_ErrorType err;
  err = dd_NoError;
  dd_set_global_constants();
  M = GapInputToMatrixPtr(main);

  poly = dd_DDMatrix2Poly(M, &err);
  A = dd_CopyInequalities(poly);
  dd_free_global_constants();
  return MatPtrToGapObj(A);
}

static Obj CddInterface_Compute_V_rep(Obj self, Obj main)
{
  dd_MatrixPtr M, A;
  dd_PolyhedraPtr poly;
  dd_ErrorType err;
  err = dd_NoError;
  dd_set_global_constants();
  M = GapInputToMatrixPtr(main);

  poly = dd_DDMatrix2Poly(M, &err);
  A = dd_CopyGenerators(poly);
  dd_free_global_constants();

  return MatPtrToGapObj(A);
}

static Obj CddInterface_LpSolution(Obj self, Obj main)
{
  dd_MatrixPtr M;
  dd_ErrorType err;
  dd_LPPtr lp;
  dd_LPSolutionPtr lps;
  dd_LPSolverType solver;
  size_t n;
  dd_colrange j;
  dd_set_global_constants();
  solver = dd_DualSimplex;
  err = dd_NoError;
  Obj current, res;

  M = GapInputToMatrixPtr(main);
  lp = dd_Matrix2LP(M, &err);

  dd_LPSolve(lp, solver, &err);
  lps = dd_CopyLPSolution(lp);

  if (lps->LPS == dd_Optimal)
  {

    n = lps->d - 1;
    current = NEW_PLIST(T_PLIST_CYC, n);
    for (j = 1; j <= n; j++)
    {
      ASS_LIST(current, j, MPQ_TO_GAPOBJ(lps->sol[j]));
    }

    res = NEW_PLIST(T_PLIST_CYC, 2);
    ASS_LIST(res, 1, current);
    ASS_LIST(res, 2, MPQ_TO_GAPOBJ(lps->optvalue));

    return res;
  }

  else

    return Fail;
}

static Obj CddInterface_FacesWithDimensionAndInteriorPoints(Obj self, Obj main, Obj mindim)
{
  dd_MatrixPtr M;
  dd_rowset R, S;

  M = GapInputToMatrixPtr(main);

  set_initialize(&R, M->rowsize);
  set_initialize(&S, M->rowsize);

  return FaceWithDimAndInteriorPoint(M, R, S, INT_INTOBJ(mindim));
}

/******************************************************************/

typedef Obj (*GVarFunc)(/*arguments*/);

#define GVAR_FUNC_TABLE_ENTRY(srcfile, name, nparam, params) \
  {                                                          \
#name, nparam,                                           \
        params,                                              \
        (GVarFunc)name,                                      \
        srcfile ":Func" #name                                \
  }

// Table of functions to export
static StructGVarFunc GVarFuncs[] = {
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_Canonicalize, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_Compute_H_rep, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_Compute_V_rep, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_LpSolution, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_DimAndInteriorPoint, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_FourierElimination, 1, "main"),
    GVAR_FUNC_TABLE_ENTRY("CddInterface.c", CddInterface_FacesWithDimensionAndInteriorPoints, 2, "main, mindim"),

    {0} /* Finish with an empty entry */

};

/******************************************************************************
*F  InitKernel( <module> )  . . . . . . . . initialise kernel data structures
*/
static Int InitKernel(StructInitInfo *module)
{
  /* init filters and functions                                          */
  InitHdlrFuncsFromTable(GVarFuncs);

  /* return success                                                      */
  return 0;
}

/******************************************************************************
*F  InitLibrary( <module> ) . . . . . . .  initialise library data structures
*/
static Int InitLibrary(StructInitInfo *module)
{
  /* init filters and functions */
  InitGVarFuncsFromTable(GVarFuncs);

  /* return success                                                      */
  return 0;
}

/******************************************************************************
*F  InitInfopl()  . . . . . . . . . . . . . . . . . table of init functions
*/
static StructInitInfo module = {
    /* type        = */ MODULE_DYNAMIC,
    /* name        = */ "CddInterface",
    /* revision_c  = */ 0,
    /* revision_h  = */ 0,
    /* version     = */ 0,
    /* crc         = */ 0,
    /* initKernel  = */ InitKernel,
    /* initLibrary = */ InitLibrary,
    /* checkInit   = */ 0,
    /* preSave     = */ 0,
    /* postSave    = */ 0,
    /* postRestore = */ 0};

StructInitInfo *Init__Dynamic(void)
{
  return &module;
}
