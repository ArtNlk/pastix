/**
 *
 * @file symbol.h
 *
 * PaStiX symbol structure routines
 *
 * @copyright 2004-2021 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author David Goudin
 * @author Francois Pellegrini
 * @author Mathieu Faverge
 * @author Pascal Henon
 * @author Pierre Ramet
 * @author Gregoire Pichon
 * @author Tony Delarue
 * @author Vincent Bridonneau
 * @date 2020-06-17
 *
 * @addtogroup pastix_symbol
 * @{
 *   @brief Functions to generate and manipulate the symbolic factorization
 *   structure
 *
 *   This module provides the set of function to generate the symbolic
 *   factorization structure based on a given graph, and an associated
 *   ordering. The symbolic structure is described in the symbol_matrix_t
 *   structure, and it can be generated through two different algorithms
 *   respectively for direct and incomplete factorization.
 *
 **/
#ifndef _symbol_h_
#define _symbol_h_

#define SYMBCBLK_NOTHING 0
#define SYMBCBLK_PROJ    1
#define SYMBCBLK_KWAY    2

/**
 * @brief Symbol column block structure.
 */
typedef struct symbol_cblk_s {
    pastix_int_t fcolnum; /**< First column index               */
    pastix_int_t lcolnum; /**< Last column index (inclusive)    */
    pastix_int_t bloknum; /**< First block in column (diagonal) */
    pastix_int_t brownum; /**< First block in row facing the diagonal block in browtab, 0-based */
    int8_t       selevtx;
#if defined( PASTIX_SYMBOL_DUMP_SYMBMTX )
    pastix_int_t split_cblk;
#endif
} symbol_cblk_t;

/**
 * @brief Symbol block structure.
 */
typedef struct symbol_blok_s {
    pastix_int_t frownum; /**< First row index            */
    pastix_int_t lrownum; /**< Last row index (inclusive) */
    pastix_int_t lcblknm; /**< Local column block         */
    pastix_int_t fcblknm; /**< Facing column block        */
} symbol_blok_t;

/**
 * @brief Symbol matrix structure.
 *
 * This structure describes the symbolic block structure of the factorized
 * matrix L, U is never stored as it is a symmetry of L. This structure is
 * global and replicated on all processes. The default way to number the block
 * is the CSC format where block are continuously number per column, the browtab
 * array stores the CSR representation of the L structure to have a faster
 * access to the list of blocks updating a column block.
 *
 */
typedef struct symbol_matrix_s {
    pastix_int_t   baseval;   /**< Base value for numbering                   */
    pastix_int_t   cblknbr;   /**< Number of column blocks                    */
    pastix_int_t   bloknbr;   /**< Number of blocks                           */
    pastix_int_t   nodenbr;   /**< Number of nodes (Equal to gN in spm)       */
    pastix_int_t   schurfcol; /**< First column of the schur complement       */
    symbol_cblk_t *cblktab;   /**< Array of column blocks [+1,based]          */
    symbol_blok_t *bloktab;   /**< Array of blocks in CSC format [based]      */
    pastix_int_t  *browtab;   /**< Array of blocks in CSR format [based]      */
    pastix_int_t   browmax;   /**< Maximum number of input edges per node     */
    pastix_int_t   dof;       /**< Degrees of freedom per node (constant
                                   if > 0, variadic if < 1                    */
    pastix_int_t  *dofs;      /**< Array of the first column of each element
                                   in the expanded matrix [+1,based]          */
} symbol_matrix_t;

/**
 *******************************************************************************
 *
 * @brief Get the expanded column indexes of a symbol_cblk.
 *
 *******************************************************************************
 *
 * @param[in] symbmtx
 *          Pointer to the symbol matrix.
 *
 * @param[in] symbcblk
 *          The pointer to the current symbol_cblk.
 *
 * @param[inout] fcolnum
 *          First column index of the current cblk.
 *
 * @param[inout] symbcblk
 *          Last column index of the current cblk.
 *
 * @return The number of columns of the expanded cblk.
 *
 *******************************************************************************/
static inline pastix_int_t
symbol_cblk_get_colnum( const symbol_matrix_t *symbmtx,
                        symbol_cblk_t         *symbcblk,
                        pastix_int_t          *fcolnum,
                        pastix_int_t          *lcolnum )
{
    if ( symbmtx->dof < 0 ) {
        *fcolnum = symbmtx->dofs[symbcblk->fcolnum];
        *lcolnum = symbmtx->dofs[symbcblk->lcolnum + 1] - 1;
    }
    else {
        *fcolnum = symbmtx->dof *   symbcblk->fcolnum;
        *lcolnum = symbmtx->dof * ( symbcblk->lcolnum + 1 ) - 1;
    }
    return (*lcolnum) - (*fcolnum) + 1;
}

/**
 *******************************************************************************
 *
 * @brief Get the expanded row index of a symbol_blok.
 *
 *******************************************************************************
 *
 * @param[in] symbmtx
 *          Pointer to the symbol matrix.
 *
 * @param[in] symbblok
 *          The pointer to the current symbol_blok.
 *
 * @param[inout] frownum
 *          First row index of the current blok.
 *
 * @param[inout] lrownum
 *          Last row index of the current blok.
 *
 * @return The number of rows of the expanded blok.
 *
 *******************************************************************************/
static inline pastix_int_t
symbol_blok_get_rownum( const symbol_matrix_t *symbmtx,
                        symbol_blok_t         *symbblok,
                        pastix_int_t          *frownum,
                        pastix_int_t          *lrownum )
{
    if ( symbmtx->dof < 0 ) {
        *frownum = symbmtx->dofs[symbblok->frownum];
        *lrownum = symbmtx->dofs[symbblok->lrownum + 1] - 1;
    }
    else {
        *frownum = symbmtx->dof *   symbblok->frownum;
        *lrownum = symbmtx->dof * ( symbblok->lrownum + 1 ) - 1;
    }
    return (*lrownum) - (*frownum) + 1;
}

/**
 * @brief Check if a block is included inside another one.
 *
 * Indicate if a blok is included inside another block.
 * i.e. indicate if the row range of the first block is included in the
 * one of the second.
 *
 * @param[in] blok  The block that is tested for inclusion.
 * @param[in] fblok The block that is suppose to include the first one.
 *
 * @retval true   if the first block is     included in the second one.
 * @retval false  if the first block is not included in the second one.
 */
static inline int
is_symbblock_inside_fblock( const symbol_blok_t *blok,
                            const symbol_blok_t *fblok )
{
    return ((blok->frownum >= fblok->frownum) &&
            (blok->lrownum <= fblok->lrownum));
}

/**
 * @name Symbol basic subroutines
 * @{
 */
void pastixSymbolInit   ( const pastix_graph_t  *graph,
                          const pastix_order_t  *order,
                                symbol_matrix_t *symbptr );
void pastixSymbolExit   (       symbol_matrix_t *symbptr );
void pastixSymbolBase   (       symbol_matrix_t *symbptr,
                          const pastix_int_t     baseval );
void pastixSymbolRealloc(       symbol_matrix_t *symbptr );
int  pastixSymbolCheck  ( const symbol_matrix_t *symbptr );
void pastixSymbolExpand (       symbol_matrix_t *symbptr );

/**
 * @}
 * @name Symbol IO subroutines
 * @{
 */
int  pastixSymbolSave( const symbol_matrix_t *symbptr, FILE *stream );
int  pastixSymbolLoad(       symbol_matrix_t *symbptr, FILE *stream );
int  pastixSymbolDraw( const symbol_matrix_t *symbptr, FILE *stream );
void pastixSymbolDrawMap( pastix_data_t *pastix_data,
                          const char    *extname,
                          pastix_int_t   sndeidx );

/**
 * @}
 * @name Symbol statistical information subroutines
 * @{
 */
void   pastixSymbolPrintStats( const symbol_matrix_t *symbptr );
size_t pastixSymbolGetNNZ( const symbol_matrix_t *symbptr );
void   pastixSymbolGetFlops( const symbol_matrix_t *symbmtx,
                             pastix_coeftype_t      flttype,
                             pastix_factotype_t     factotype,
                             double                *thflops,
                             double                *rlflops );
void   pastixSymbolGetTimes( const symbol_matrix_t *symbmtx,
                             pastix_coeftype_t      flttype,
                             pastix_factotype_t     factotype,
                             double                *cblkcost,
                             double                *blokcost );

/**
 * @}
 * @name Symbol reordering subroutines
 * @{
 */
void pastixSymbolReordering( pastix_data_t * );
void pastixSymbolReorderingPrintComplexity( const symbol_matrix_t *symbptr );

/**
 * @}
 * @name Symbol construction subroutines
 * @{
 */
int          pastixSymbolFaxDirect ( symbol_matrix_t      *symbptr,
                                     const pastix_graph_t *graphA,
                                     const pastix_order_t *ordeptr );
int          pastixSymbolFaxILUk   ( symbol_matrix_t      *symbptr,
                                     pastix_int_t          levelk,
                                     const pastix_graph_t *graphA,
                                     const pastix_order_t *ordeptr );
void         pastixSymbolRustine   ( symbol_matrix_t *symbptr, symbol_matrix_t *symbptr2 );
void         pastixSymbolBuildRowtab( symbol_matrix_t *symbptr );
pastix_int_t pastixSymbolGetFacingBloknum( const symbol_matrix_t *symbptr,
                                           pastix_int_t           bloksrc,
                                           pastix_int_t           bloknum,
                                           pastix_int_t           startsearch,
                                           int                    ricar );
/**
 * @}
 */

#endif /* _symbol_h_ */

/**
 * @}
 */
