/**
 *
 * @file symbol.h
 *
 *  PaStiX symbol structure routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author David Goudin
 * @author Francois Pelegrin
 * @author Mathieu Faverge
 * @author Pascal Henon
 * @author Pierre Ramet
 * @date 2013-06-24
 *
 **/
#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/**
 * @ingroup pastix_symbol
 * @struct symbolcblk_s - Symbol column block structure.
 */
typedef struct SymbolCblk_ {
    pastix_int_t fcolnum;  /*< First column index               */
    pastix_int_t lcolnum;  /*< Last column index (inclusive)    */
    pastix_int_t bloknum;  /*< First block in column (diagonal) */
    pastix_int_t brownum;  /*< First block in row facing the diagonal block in browtab, 0-based */
} SymbolCblk;

/**
 * @ingroup pastix_symbol
 * @struct symbolblok_s - Symbol block structure.
 */
typedef struct SymbolBlok_ {
    pastix_int_t frownum;  /*< First row index            */
    pastix_int_t lrownum;  /*< Last row index (inclusive) */
    pastix_int_t lcblknm;  /*< Local column block         */
    pastix_int_t fcblknm;  /*< Facing column block        */
} SymbolBlok;

/**
 * @ingroup pastix_symbol
 * @struct symbolmtx_s - Symbol matrix structure.
 */
typedef struct SymbolMatrix_ {
    pastix_int_t            baseval;  /*< Base value for numberings               */
    pastix_int_t            dof;      /*< Degrees of freedom per node
                                          (constant if > 0, unconstant if 0 (not implemented)) */
    pastix_int_t            cblknbr;  /*< Number of column blocks                 */
    pastix_int_t            bloknbr;  /*< Number of blocks                        */
    pastix_int_t            nodenbr;  /*< Number of node in the compressed symbol */
    SymbolCblk   * restrict cblktab;  /*< Array of column blocks [+1,based]       */
    SymbolBlok   * restrict bloktab;  /*< Array of blocks [based]                 */
    pastix_int_t * restrict browtab;  /*< Array of blocks [based]                 */
#ifdef STARPU_GET_TASK_CTX
    pastix_int_t            starpu_subtree_nbr;
#endif
} SymbolMatrix;

/*
 **  The function prototypes.
 */

int  symbolInit       (      SymbolMatrix *symbptr);
void symbolExit       (      SymbolMatrix *symbptr);
void symbolBase       (      SymbolMatrix *symbptr, const pastix_int_t baseval);
void symbolRealloc    (      SymbolMatrix *symbptr);
void symbolRustine    (      SymbolMatrix *symbptr, SymbolMatrix *symbptr2);
void symbolBuildRowtab(      SymbolMatrix *symbptr);
int  symbolCheck      (const SymbolMatrix *symbptr);
int  symbolSave       (const SymbolMatrix *symbptr, FILE *stream);
int  symbolLoad       (      SymbolMatrix *symbptr, FILE *stream);
int  symbolDraw       (const SymbolMatrix *symbptr, FILE *stream);
void symbolPrintStats (const SymbolMatrix *symbptr);

pastix_int_t
symbolGetFacingBloknum(const SymbolMatrix *symbptr,
                       pastix_int_t bloksrc,
                       pastix_int_t bloknum,
                       pastix_int_t startsearch,
                       int ricar);

void symbolCheckProperties( const SymbolMatrix * );
void symbolNewOrdering( const SymbolMatrix *, Order *,
                        pastix_int_t, int, int );
void symbolPrintComplexityReordering( const SymbolMatrix *symbptr );

/* Pastix_Int_Ternal routines for hamming vectors */
pastix_int_t hamming_distance_symbol( pastix_int_t **, pastix_int_t *, pastix_int_t, pastix_int_t, pastix_int_t);
void update_perm(pastix_int_t, Order *, pastix_int_t,
                 pastix_int_t *, pastix_int_t **, pastix_int_t *,
                 pastix_int_t *, pastix_int_t **, pastix_int_t *,
                 pastix_int_t, pastix_int_t);

void
symbolCost(const SymbolMatrix *symbmtx, const Dof *dofptr,
           pastix_coeftype_t flttype, pastix_factotype_t factotype,
           pastix_int_t *nnz, double *thflops, double *rlflops );
pastix_int_t symbolGetNNZ  ( const SymbolMatrix *symbptr);
void         symbolGetFlops( const SymbolMatrix *symbmtx,
                             pastix_coeftype_t flttype, pastix_factotype_t factotype,
                             double *thflops, double *rlflops );
void         symbolGetTimes( const SymbolMatrix *symbmtx,
                             pastix_coeftype_t flttype, pastix_factotype_t factotype,
                             double *cblkcost, double *blokcost );

#endif /* SYMBOL_H */
