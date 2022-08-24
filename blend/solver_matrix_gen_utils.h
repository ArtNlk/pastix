/**
 *
 * @file solver_matrix_gen_utils.h
 *
 * PaStiX solver structure generation functions to factorize
 * solver_matric_gen.c .
 *
 * @copyright 1998-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Tony Delarue
 * @author Pascal Henon
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Mathieu Faverge
 * @date 2021-01-03
 *
 * @addtogroup blend_dev_solver
 * @{
 *
 **/
#ifndef _solver_matrix_gen_utils_h_
#define _solver_matrix_gen_utils_h_

#include "elimintree.h"
#include "cost.h"
#include "symbol/symbol.h"
#include "cand.h"
#include "blend/solver.h"
#include "pastix/order.h"

/**
 *******************************************************************************
 *
 * @brief Initialize a solver block.
 *
 *******************************************************************************
 *
 * @param[inout] solvblok
 *          The pointer to the solver block to initialize.
 *
 * @param[in] lcblknm
 *          Local column block index.
 *
 * @param[in] fcblknm
 *          Facing column block index.
 *
 * @param[in] frownum
 *          First row of the block.
 *
 * @param[in] lrownum
 *          Last row of the block.
 *
 * @param[in] stride
 *          Stride of the column block.
 *
 * @param[in] nbcols
 *          Number of columns of the cblk to which belong the current block.
 *
 * @param[in] layout2D
 *          Parameter which indicates if the cblk layout is 1D lapack or 2D tile
 *          layout.
 *
 ********************************************************************************/
static inline void
solvMatGen_init_blok( SolverBlok  *solvblok,
                      pastix_int_t lcblknm,
                      pastix_int_t fcblknm,
                      pastix_int_t frownum,
                      pastix_int_t lrownum,
                      pastix_int_t stride,
                      pastix_int_t nbcols,
                      pastix_int_t layout2D )
{
    assert( fcblknm >= -1 );
    assert( lcblknm >= 0 );
    assert( (fcblknm == -1) || (lcblknm <= fcblknm) );
    assert( frownum >= 0 );
    assert( lrownum >= frownum );
    assert( stride  >= 0 );
    assert( nbcols  >= 0 );

    solvblok->handler[0] = NULL;
    solvblok->handler[1] = NULL;
    solvblok->fcblknm    = fcblknm;
    solvblok->lcblknm    = lcblknm;
    solvblok->frownum    = frownum;
    solvblok->lrownum    = lrownum;
    solvblok->coefind    = layout2D ? stride * nbcols : stride;
    solvblok->browind    = -1;
    solvblok->gpuid      = GPUID_UNDEFINED;
    solvblok->inlast     = 0;
    solvblok->LRblock    = NULL;
}

/**
 *******************************************************************************
 *
 * @brief Initialize a solver cblk.
 *
 *******************************************************************************
 *
 * @param[inout] solvcblk
 *          The pointer to the solver cblk to initialize.
 *
 * @param[in] fblokptr
 *          The pointer to the first block.
 *
 * @param[in] candcblk
 *          The associated cand structure to the cblk to know the type of the cblk.
 *
 * @param[in] symbcblk
 *          The associated symbol cblk structure to get symbol information.
 *
 * @param[in] fcolnum
 *          Index of the first column included in the cblk.
 *
 * @param[in] lcolnum
 *          Index of the last column included in the cblk.
 *
 * @param[in] brownum
 *          Index of the first contribution to this cblk in the browtab.
 *
 * @param[in] stride
 *          Stride of the cblk.
 *
 * @param[in] cblknum
 *          The global index of the cblk. -1 if virtual.
 *
 * @param[in] ownerid
 *          The owner if of the cluster that owns the main instance of the cblk.
 *
 *******************************************************************************/
static inline void
solvMatGen_init_cblk( SolverCblk          *solvcblk,
                      SolverBlok          *fblokptr,
                      const Cand          *candcblk,
                      const symbol_cblk_t *symbcblk,
                      pastix_int_t         fcolnum,
                      pastix_int_t         lcolnum,
                      pastix_int_t         brownum,
                      pastix_int_t         stride,
                      pastix_int_t         cblknum,
                      int                  ownerid )
{
    assert( fblokptr != NULL );
    assert( fcolnum >= 0 );
    assert( lcolnum >= fcolnum );
    assert( stride  >= 0 );
    assert( brownum >= 0 );

    /* Init the cblk */
    solvcblk->lock       = PASTIX_ATOMIC_UNLOCKED;
    solvcblk->ctrbcnt    = -1;
    solvcblk->cblktype   = (cblknum == -1) ? 0 : candcblk->cblktype;
    solvcblk->gpuid      = GPUID_UNDEFINED;
    solvcblk->fcolnum    = fcolnum;
    solvcblk->lcolnum    = lcolnum;
    solvcblk->fblokptr   = fblokptr;
    solvcblk->stride     = stride;
    solvcblk->lcolidx    = -1;
    solvcblk->brownum    = brownum;
    solvcblk->gcblknum   = cblknum;
    solvcblk->bcscnum    = -1;
    solvcblk->selevtx    = (symbcblk->selevtx == SYMBCBLK_PROJ) ? 1 : 0;
    solvcblk->ownerid    = ownerid;
    solvcblk->lcoeftab   = NULL;
    solvcblk->ucoeftab   = NULL;
    solvcblk->handler[0] = NULL;
    solvcblk->handler[1] = NULL;
    solvcblk->threadid   = -1;
}

/**
 *******************************************************************************
 *
 * @brief Register the original supernode index of the cblk.
 *
 * This computes the index in the original elimination tree before spliting the
 * large supernodes.
 *
 *******************************************************************************
 *
 * @param[inout] solvcblk
 *          The pointer to the cblk.
 *
 * @param[in] sndeidx
 *          The index of the last visited supernode to reduce the complexity of
 *          the function.
 *
 * @param[in] ordeptr
 *          The ordering structure.
 *
 * @return The supernode index of the given cblk. The value can be used in the
 *         future calls of the function to reduce its complexity cost.
 *
 *******************************************************************************/
static inline pastix_int_t
solvMatGen_supernode_index( SolverCblk           *solvcblk,
                            pastix_int_t          sndeidx,
                            const pastix_order_t *ordeptr )
{
    while ( (sndeidx < ordeptr->sndenbr ) &&
            (ordeptr->sndetab[sndeidx+1] <= solvcblk->lcolnum) )
    {
        sndeidx++;
    }
    assert( (ordeptr->sndetab[sndeidx]   <= solvcblk->fcolnum) &&
            (ordeptr->sndetab[sndeidx+1] >  solvcblk->lcolnum) );
    solvcblk->sndeidx = sndeidx;

    /* Register the cblk as being part of the last supernode */
    if ( solvcblk->sndeidx+1 == ordeptr->sndenbr ) {
        solvcblk->cblktype = solvcblk->cblktype | CBLK_IN_LAST;
    }

    return sndeidx;
}

/**
 *******************************************************************************
 *
 * @brief Update the 1D/2D infos of the solver matrix through a cblk.
 *
 *******************************************************************************
 *
 * @param[inout] solvmtx
 *          Pointer to the solver matrix.
 *
 * @param[inout] nbcblk2d
 *          Amount of 2D cblk.
 *          On exit, the number of cblk is updated if the given cblk is
 *          considered as 2D.
 *
 * @param[inout] nbblok2d
 *          Amount of 2D blocks.
 *          On exit, the number of blok is updated if the given cblk is
 *          considered as 2D.
 *
 * @param[in] nbbloks
 *          Amount blocks in the current cblk.
 *
 * @param[in] tasks2D
 *          Boolean which indicate if the task is 2D.
 *
 * @param[in] cblknum
 *          Current cblk index.
 *
 *******************************************************************************/
static inline void
solvMatGen_cblkIs2D( SolverMatrix *solvmtx,
                     pastix_int_t *nbcblk2d,
                     pastix_int_t *nbblok2d,
                     pastix_int_t  nbbloks,
                     pastix_int_t  tasks2D,
                     pastix_int_t  cblknum )
{
    /*
     * 2D tasks: Compute the number of cblk split in 2D tasks, and
     * the smallest id
     */
    if ( tasks2D ) {
        solvmtx->cblkmin2d = pastix_imin( solvmtx->cblkmin2d, cblknum );
        *nbcblk2d += 1;
        *nbblok2d += nbbloks;
    }
    else {
        solvmtx->cblkmax1d = pastix_imax( solvmtx->cblkmax1d, cblknum );
    }

    /*
     * Compute the maximum number of block per cblk for data
     * structure in PaRSEC/StarPU
     */
    if ( cblknum >= solvmtx->cblkmin2d ) {
        solvmtx->cblkmaxblk = pastix_imax( solvmtx->cblkmaxblk, nbbloks );
    }
}

void solvMatGen_fill_localnums( const symbol_matrix_t *symbmtx,
                                const SimuCtrl        *simuctrl,
                                SolverMatrix          *solvmtx,
                                pastix_int_t          *cblklocalnum,
                                pastix_int_t          *bloklocalnum,
                                pastix_int_t          *tasklocalnum,
                                solver_cblk_recv_t   **ftgttab );

SolverBlok* solvMatGen_register_local_cblk( const symbol_matrix_t *symbmtx,
                                            const Cand            *candcblk,
                                            const pastix_int_t    *cblklocalnum,
                                            SolverCblk            *solvcblk,
                                            SolverBlok            *solvblok,
                                            pastix_int_t           lcblknm,
                                            pastix_int_t           brownum,
                                            pastix_int_t           gcblknm,
                                            pastix_int_t           ownerid );

SolverBlok* solvMatGen_register_remote_cblk( const symbol_matrix_t    *symbmtx,
                                             const solver_cblk_recv_t *recvcblk,
                                             const Cand               *candcblk,
                                             const pastix_int_t       *cblklocalnum,
                                             SolverCblk               *solvcblk,
                                             SolverBlok               *solvblok,
                                             pastix_int_t              lcblknm,
                                             pastix_int_t              brownum,
                                             pastix_int_t              gcblknm );

pastix_int_t solvMatGen_reorder_browtab( const symbol_matrix_t *symbmtx,
                                         const symbol_cblk_t   *symbcblk,
                                         SolverMatrix          *solvmtx,
                                         SolverCblk            *solvcblk,
                                         pastix_int_t          *browtmp,
                                         const pastix_int_t    *cblklocalnum,
                                         const pastix_int_t    *bloklocalnum,
                                         pastix_int_t           brownum );

void solvMatGen_fill_tasktab( SolverMatrix       *solvmtx,
                              isched_t           *isched,
                              const SimuCtrl     *simuctrl,
                              const pastix_int_t *tasklocalnum,
                              const pastix_int_t *cblklocalnum,
                              const pastix_int_t *bloklocalnum,
                              pastix_int_t        clustnum,
                              int                 is_dbg );

void solvMatGen_stats_last( SolverMatrix *solvmtx );
void solvMatGen_max_buffers( SolverMatrix *solvmtx );

void solver_recv_update_fanin( solver_cblk_recv_t   **faninptr,
                               const symbol_matrix_t *symbmtx,
                               const symbol_cblk_t   *cblk,
                               const symbol_blok_t   *blok,
                               const symbol_cblk_t   *fcblk,
                               int ownerid );
void solver_recv_update_recv( solver_cblk_recv_t   **recvptr,
                              const symbol_matrix_t *symbmtx,
                              const symbol_cblk_t   *cblk,
                              const symbol_blok_t   *blok,
                              const symbol_cblk_t   *fcblk,
                              int                    ownerid );
int  solver_recv_get_bloknbr( const solver_cblk_recv_t *ftgtptr,
                              const symbol_cblk_t      *symbcblk,
                              const symbol_blok_t      *symbblok );

#endif /* _solver_matrix_gen_utils_h_ */

/**
 *@}
 */
