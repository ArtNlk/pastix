/**
 * @file solver_recv.c
 *
 * PaStiX solver reception structure management.
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-01-03
 *
 * @addtogroup blend_dev_solver
 * @{
 *
 **/
#include "common.h"
#include "symbol/symbol.h"
#include "solver_matrix_gen_utils.h"

/**
 *******************************************************************************
 *
 * @brief Update columns indices of a reception/fanin cblk.
 *
 *******************************************************************************
 *
 * @param[inout] cblk
 *          The fanin or recv cblk to update. Must have been initialized first.
 *
 * @param[in] fcolnum
 *          The first column index of the contribution block.
 *
 * @param[in] fcolnum
 *          The last column index of the contribution block.
 *
 *******************************************************************************/
static inline void
solver_recv_update_cols( solver_cblk_recv_t *cblk,
                         pastix_int_t fcolnum,
                         pastix_int_t lcolnum )
{
    cblk->fcolnum = pastix_imin( fcolnum, cblk->fcolnum );
    cblk->lcolnum = pastix_imax( lcolnum, cblk->lcolnum );
}

/**
 *******************************************************************************
 *
 * @brief Update rows indices of a reception/fanin blok.
 *
 *******************************************************************************
 *
 * @param[inout] blok
 *          The fanin or recv blok to update. Must have been initialized first.
 *
 * @param[in] frownum
 *          The first row index of the contribution block.
 *
 * @param[in] frownum
 *          The last row index of the contribution block.
 *
 *******************************************************************************/
static inline void
solver_recv_update_rows( solver_blok_recv_t *blok,
                         pastix_int_t frownum,
                         pastix_int_t lrownum )
{
    blok->frownum = pastix_imin( frownum, blok->frownum );
    blok->lrownum = pastix_imax( lrownum, blok->lrownum );
}

/**
 *******************************************************************************
 *
 * @brief Create a new reception/fanin cblk and initialize to the default values.
 *
 *******************************************************************************
 *
 * @param[in] symbmtx
 *          The symbol matrix pointer (used to access the bloktab array)
 *
 * @param[in] cblk
 *          The symbol cblk used as a template to create the fanin/recv cblk.
 *
 * @return The pointer to the recv/fanin cblk initialized with respect to the
 *         input symbcblk.
 *
 *******************************************************************************/
static inline solver_cblk_recv_t *
solver_recv_cblk_init( const symbol_matrix_t *symbmtx,
                       const symbol_cblk_t   *cblk )
{
    solver_cblk_recv_t *cblkrecv;
    symbol_blok_t      *blok;
    pastix_int_t bloknbr = cblk[1].bloknum - cblk[0].bloknum;
    pastix_int_t i;

    assert( bloknbr >= 1 );
    cblkrecv = malloc( sizeof(solver_cblk_recv_t) + ((bloknbr-1) * sizeof(solver_blok_recv_t)) );
    cblkrecv->next    = NULL;
    cblkrecv->ownerid = -1;
    cblkrecv->fcolnum = cblk->lcolnum + 1;
    cblkrecv->lcolnum = cblk->fcolnum - 1;

    blok = symbmtx->bloktab + cblk->bloknum;
    for( i=0; i<bloknbr; i++, blok++ ) {
        cblkrecv->bloktab[i].frownum = blok->lrownum + 1;
        cblkrecv->bloktab[i].lrownum = blok->frownum - 1;
    }

    return cblkrecv;
}

static inline void
solver_recv_add_contrib( solver_cblk_recv_t    *rcblk,
                         const symbol_matrix_t *symbmtx,
                         const symbol_cblk_t   *cblk,
                         const symbol_blok_t   *blok,
                         const symbol_cblk_t   *fcblk )
{
    symbol_blok_t *lblok, *fblok;
    pastix_int_t i;

    /* Let's update the columns of the cblk */
    solver_recv_update_cols( rcblk, blok->frownum, blok->lrownum );

    /* Let's iterate on the blocks to update the rows */
    lblok = symbmtx->bloktab + cblk[1].bloknum;
    fblok = symbmtx->bloktab + fcblk->bloknum;
    i = 0;
    for( ; blok<lblok; blok++ ) {
        /* Look for the facing blok in the symbol */
        while(!is_symbblock_inside_fblock( blok, fblok )) {
            i++;
            fblok++;
            assert( fcblk[0].bloknum + i < fcblk[1].bloknum );
        }

        /* Update the rows in the recev block */
        solver_recv_update_rows( &(rcblk->bloktab[i]), blok->frownum, blok->lrownum );
    }
}

/**
 *******************************************************************************
 *
 * @brief Register a new contribution to a fanin cblk
 *
 *******************************************************************************
 *
 * @param[inout] faninptr
 *          The location of the pointer of the fanin cblk.
 *          On entry, points to NULL if the fanin does not exist, to the
 *          existing fanin otherwise.
 *          On exit, points to the updated fanin cblk.
 *
 * @param[in] symbmtx
 *          The symbol matrix pointer (used to access the bloktab array)
 *
 * @param[in] cblk
 *          The symbol cblk that holds the block responsible for the contribution.
 *
 * @param[in] blok
 *          The symbol blok responsible for the contribution.
 *
 * @param[in] fcblk
 *          The facing symbol cblk that corresponds to the faninptr, and that is
 *          updated by the contribution.
 *
 * @param[in] ownerid
 *          The index of the cluster that owns the fcblk.
 *
 *******************************************************************************/
void
solver_recv_update_fanin( solver_cblk_recv_t   **faninptr,
                          const symbol_matrix_t *symbmtx,
                          const symbol_cblk_t   *cblk,
                          const symbol_blok_t   *blok,
                          const symbol_cblk_t   *fcblk,
                          int ownerid )
{
    if ( *faninptr == NULL ) {
        *faninptr = solver_recv_cblk_init( symbmtx, fcblk );
        (*faninptr)->ownerid = ownerid;
    }
    assert( (*faninptr)->ownerid == ownerid );

    solver_recv_add_contrib( *faninptr, symbmtx, cblk, blok, fcblk );
}

/**
 *******************************************************************************
 *
 * @brief Register a new contribution to a recv cblk
 *
 *******************************************************************************
 *
 * @param[inout] recvptr
 *          The location of the pointer of the recv cblks list.
 *          On entry, points to NULL if no recv cblk has been registered yet, to the
 *          head of the list otherwise.
 *          On exit, points to the updated list of recv cblks.
 *
 * @param[in] symbmtx
 *          The symbol matrix pointer (used to access the bloktab array)
 *
 * @param[in] cblk
 *          The symbol cblk that holds the block responsible for the contribution.
 *
 * @param[in] blok
 *          The symbol blok responsible for the contribution.
 *
 * @param[in] fcblk
 *          The facing symbol cblk that corresponds to the recvptr, and that is
 *          updated by the contribution.
 *
 * @param[in] ownerid
 *          The index of the cluster that owns the original contribution (cblk).
 *
 *******************************************************************************/
void
solver_recv_update_recv( solver_cblk_recv_t   **recvptr,
                         const symbol_matrix_t *symbmtx,
                         const symbol_cblk_t   *cblk,
                         const symbol_blok_t   *blok,
                         const symbol_cblk_t   *fcblk,
                         int ownerid )
{
    solver_cblk_recv_t *prev, *rcblk;

    prev  = NULL;
    rcblk = *recvptr;
    while( (rcblk != NULL) && (rcblk->ownerid != ownerid) ) {
        prev  = rcblk;
        rcblk = rcblk->next;
    }

    /* Create a new cblk if not found */
    if ( rcblk == NULL ) {
        rcblk = solver_recv_cblk_init( symbmtx, fcblk );
        rcblk->ownerid = ownerid;
        if ( prev == NULL ) {
            /* Head of the list */
            *recvptr = rcblk;
        }
        else {
            assert( prev->next == NULL );
            prev->next = rcblk;
        }
    }

    assert( rcblk->ownerid == ownerid );
    solver_recv_add_contrib( rcblk, symbmtx, cblk, blok, fcblk );
}

/**
 *******************************************************************************
 *
 * @brief Compute the number of valid blocks in fanin/recv cblk
 *
 *******************************************************************************
 *
 * @param[in] ftgtptr
 *          The pointer to the fan-in contribution to study.
 *
 * @param[in] symbcblk
 *          The original symbol cblk associated to the fan-in
 *
 * @param[in] symbblok
 *          The first symbol blok of the symbcblk.
 *
 * @return The number of non empty blocks in the fan-in.
 *
 *******************************************************************************/
int
solver_recv_get_bloknbr( const solver_cblk_recv_t *ftgtptr,
                         const symbol_cblk_t      *symbcblk,
                         const symbol_blok_t      *symbblok )
{
    const solver_blok_recv_t *ftgtblok = ftgtptr->bloktab;
    pastix_int_t j;
    int bloknbr = 0;

    for ( j=symbcblk[0].bloknum; j<symbcblk[1].bloknum;
          j++, symbblok++, ftgtblok++ )
    {
        if ( (ftgtblok->frownum <= ftgtblok->lrownum) &&
             (ftgtblok->frownum >= symbblok->frownum) &&
             (ftgtblok->lrownum <= symbblok->lrownum) )
        {
            bloknbr++;
        }
    }
    assert( bloknbr >= 1 );
    return bloknbr;
}

/**
 * @}
 */
