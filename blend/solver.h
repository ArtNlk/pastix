/**
 * @file solver.h
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author David Goudin
 * @author Pascal Henon
 * @author Francois Pellegrini
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Xavier Lacoste
 * @date 2011-11-11
 *
 **/
#ifndef _SOLVER_H_
#define _SOLVER_H_

#include "ftgt.h"
#ifndef SOLVER_TASKS_TYPES
#define SOLVER_TASKS_TYPES

/**
 *  The type and structure definitions.
 *  Define the mask for th cblks:
 *   - 1st bit: The cblk will always be treated as one task, or can generate dynamic smaller update
 *   - 2nd bit: The cblk is dense, or is compressed
 *   - 3rd bit: Part of the Schur complement or not (Will not be factorized if yes)
 */
#define CBLK_SPLIT (1 << 0)
#define CBLK_DENSE (1 << 1)
#define CBLK_SCHUR (1 << 2)

/*
 **  The type and structure definitions.
 */
#define COMP_1D                     0
#define DIAG                        1
#define E1                          2
#define E2                          3
#define DRUNK                       4
#endif /* SOLVER_TASKS_TYPES */

typedef struct Task_ {
    pastix_int_t          taskid;  /*+ COMP_1D DIAG E1 E2                                     +*/
    pastix_int_t          prionum; /*+ Priority value for the factorization                   +*/
    pastix_int_t          cblknum; /*+ Attached column block                                  +*/
    pastix_int_t          bloknum; /*+ Attached block                                         +*/
    pastix_int_t volatile ftgtcnt; /*+ Number of fan-in targets                               +*/
    pastix_int_t volatile ctrbcnt; /*+ Total number of contributions                          +*/
    pastix_int_t          indnum;  /*+ For E2 (COMP_1D), index of ftgt (>0) else if local = -taskdest
                                    For DIAG and E1 , index of btag (>0) if there is a
                                    local one it must be the first of the chain of local E1   +*/
#if (defined PASTIX_DYNSCHED) || (defined TRACE_SOPALIN)
    pastix_int_t          threadid; /*+ Index of the bubble which contains the task +*/
#endif
#ifdef TRACE_SOPALIN
    pastix_int_t          fcandnum; /*+ First thread candidate                      +*/
    pastix_int_t          lcandnum; /*+ Last thread candidate                       +*/
    pastix_int_t          id;     /*+ Global cblknum of the attached column block +*/
#endif
} Task;

/*+ Solver block structure. +*/
typedef struct SolverBlok_ {
    pastix_int_t lcblknm;  /*< Local column block         */
    pastix_int_t fcblknm;  /*< Facing column block        */
    pastix_int_t browind;  /*< Index in browtab           */
    pastix_int_t coefind;  /*< Index in coeftab           */
    pastix_int_t frownum;  /*< First row index            */
    pastix_int_t lrownum;  /*< Last row index (inclusive) */
} SolverBlok;

/*+ Solver column block structure. +*/

typedef struct SolverCblk_  {
    pastix_atomic_lock_t lock;     /*< Lock to protect computation on the cblk */
    volatile int32_t     ctrbcnt;  /*< Number of contribution to receive       */
    int8_t               cblktype; /*< Type of cblk                            */
    int8_t               gpuid;
    pastix_int_t         fcolnum;  /*< First column index                      */
    pastix_int_t         lcolnum;  /*< Last column index (inclusive)           */
    SolverBlok          *fblokptr; /*< First block in column (diagonal)        */
    pastix_int_t         stride;   /*< Column block stride                     */
    pastix_int_t         lcolidx;  /*< Local first column index to the location in the rhs vector       */
    pastix_int_t         brownum;  /*< First block in row facing the diagonal block in browtab, 0-based */
    pastix_int_t         brown2d;  /*< First 2D-block in row facing the diagonal block in browtab, 0-based */
    pastix_int_t         gcblknum; /*< Global column block index               */
    void                *lcoeftab; /*< Coefficients access vector              */
    void                *ucoeftab; /*< Coefficients access vector              */

    /* Check if really required */
    pastix_int_t    procdiag; /*+ Cluster owner of diagonal block        +*/
} SolverCblk;

/*+ Solver matrix structure. +*/

/* All data are local to one cluster */
struct SolverMatrix_ {
    int restore; /*+ Flag to indicate if it is require to restore data with
                     solverBackupRestore: 0: No need, 1:After solve,
                     2:After Factorization +*/
    pastix_int_t            baseval; /*+ Base value for numberings                         +*/

    pastix_int_t            nodenbr;   /*< Number of nodes before dof extension              */
    pastix_int_t            coefnbr;   /*< Number of coefficients (node after dof extension) */
    pastix_int_t            gcblknbr;  /*< Global number of column blocks                    */
    pastix_int_t            cblknbr;   /*< Number of column blocks                   */
    pastix_int_t            cblkmin2d; /*< Rank of the first cblk beeing enabled for 2D computations        */
    pastix_int_t            cblkmaxblk;/*< Maximum number of blocks per cblk         */
    pastix_int_t            bloknbr;   /*< Number of blocks                          */
    pastix_int_t            brownbr;   /*< Size of the browtab array                 */
    SolverCblk   * restrict cblktab;   /*< Array of solver column blocks             */
    SolverBlok   * restrict bloktab;   /*< Array of solver blocks                    */
    pastix_int_t * restrict browtab;   /*< Array of blocks                           */
    pastix_int_t * restrict browcblk;  /*< Array of blocks                           */

#if defined(PASTIX_WITH_PARSEC)
    sparse_matrix_desc_t   *parsec_desc;
#endif

#ifdef PASTIX_WITH_STARPU
    /* All this part concern halo of the local matrix
     * ie: column blocks which will:
     *  - be updated by local column blocks
     *  - update local column blocks
     */
    pastix_int_t              hcblknbr;             /*+ Number of column block in the halo        +*/
    pastix_int_t *            gcblk2halo;           /*+ Indicate the local number corresponding
                                                     *  global column block.
                                                     *  gcblk2halo[gcblk] == 0 : gcblk not local nor in halo
                                                     *                    >  0 : local cblk number + 1
                                                     *                    <  0 : - (halo cblk number + 1)
                                                     *                                            +*/
    SolverCblk   * restrict hcblktab;             /*+ Array of halo column blocks               +*/
    SolverBlok   * restrict hbloktab;             /*+ Array of halo blocks                      +*/
    pastix_int_t *          fcblknbr;               /*+ Number of fanin buffer to send or recv    +*/
    SolverCblk  ** restrict fcblktab;               /*+ Fanin column block array                  +*/
    SolverBlok  ** restrict fbloktab;               /*+ Fanin block array                         +*/
#endif

    pastix_int_t              ftgtnbr;              /*+ Number of fanintargets                    +*/
    pastix_int_t              ftgtcnt;              /*+ Number of fanintargets to receive         +*/
    FanInTarget * restrict    ftgttab;              /*+ Fanintarget access vector                 +*/

    pastix_int_t              diagmax;              /*+ Maximum size required during diagonal block factorization (hetrf/sytrf) +*/
    pastix_int_t              gemmmax;              /*+ Maximum size required during GEMM computation                           +*/
    pastix_int_t              nbftmax;              /*+ Maximum block number in ftgt              +*/
    pastix_int_t              arftmax;              /*+ Maximum block area in ftgt                +*/

    pastix_int_t              clustnum;             /*+ current processor number                  +*/
    pastix_int_t              clustnbr;             /*+ number of processors                      +*/
    pastix_int_t              procnbr;              /*+ Number of physical processor used         +*/
    pastix_int_t              thrdnbr;              /*+ Number of local computation threads       +*/
    pastix_int_t              bublnbr;              /*+ Number of local computation threads       +*/
    BubbleTree   * restrict   btree;                /*+ Bubbles tree                              +*/

    pastix_int_t              indnbr;
    pastix_int_t * restrict   indtab;
    Task         * restrict   tasktab;              /*+ Task access vector                        +*/
    pastix_int_t              tasknbr;              /*+ Number of Tasks                           +*/
    pastix_int_t **           ttsktab;              /*+ Task access vector by thread              +*/
    pastix_int_t *            ttsknbr;              /*+ Number of tasks by thread                 +*/

    pastix_int_t *            proc2clust;           /*+ proc -> cluster                           +*/
    pastix_int_t              gridldim;             /*+ Dimensions of the virtual processors      +*/
    pastix_int_t              gridcdim;             /*+ grid if dense end block                   +*/
};

/**
 * Indicates whether a column block is in halo.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @retval API_YES if the column block is in halo.
 * @retval API_NO  if the column block is not in halo.
 */
static inline
int cblk_ishalo( SolverMatrix * datacode,
                 SolverCblk   * cblk )
{
    (void)datacode;
    (void)cblk;
#ifdef PASTIX_WITH_STARPU
    if ((size_t)cblk >= (size_t)datacode->hcblktab &&
        (size_t)cblk < (size_t)(datacode->hcblktab+datacode->hcblknbr)) {
        return API_YES;
    }
#endif
    return API_NO;
}

/**
 * Indicates whether a column block is a fanin column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @retval API_YES if the column block is a fanin column block.
 * @retval API_NO  if the column block is not a fanin column block.
 */
static inline
int cblk_isfanin( SolverMatrix * datacode,
                  SolverCblk   * cblk )
{
    (void)datacode;
    (void)cblk;
#ifdef PASTIX_WITH_STARPU
    pastix_int_t clustnum;
    for (clustnum = 0; clustnum < datacode->clustnbr; clustnum++) {
        if ((size_t)cblk >= (size_t)datacode->fcblktab[clustnum] &&
            (size_t)cblk < (size_t)(datacode->fcblktab[clustnum] +
                                    datacode->fcblknbr[clustnum])) {
            return API_YES;
        }
    }
#endif
    return API_NO;
}


/**
 * Indicates whether a column block is a local column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @retval API_YES if the column block is a local column block.
 * @retval API_NO  if the column block is not a local column block.
 */
static inline
int cblk_islocal( SolverMatrix * datacode,
                  SolverCblk   * cblk ) {
    if ((size_t)cblk >= (size_t)datacode->cblktab &&
        (size_t)cblk < (size_t)(datacode->cblktab+datacode->cblknbr)) {
        return API_YES;
    }
    return API_NO;
}


/**
 * Get the index of a local column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t cblk_getnum( SolverMatrix * datacode,
                          SolverCblk   * cblk ) {
    assert(cblk_islocal(datacode, cblk) == API_YES);
    return cblk - datacode->cblktab;
}


/**
 * Get the index of a halo column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t hcblk_getnum( SolverMatrix * datacode,
                           SolverCblk   * cblk )
{
    (void)datacode;
    (void)cblk;
#ifdef PASTIX_WITH_STARPU
    assert(cblk_ishalo(datacode, cblk) == API_YES);
    return cblk - datacode->hcblktab;
#else
    return -1;
#endif
}


/**
 * Get the index of a fanin block.
 *
 * @param datacode SolverMatrix structure.
 * @param block SolverBlok structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t fblok_getnum( SolverMatrix * datacode,
                           SolverBlok   * blok,
                           pastix_int_t   procnum )
{
    (void)datacode;
    (void)blok;
    (void)procnum;
#ifdef PASTIX_WITH_STARPU
    return blok - datacode->fbloktab[procnum];

#else
    return -1;
#endif
}

/**
 * Get the index of a local block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverBlok structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t blok_getnum( SolverMatrix * datacode,
                          SolverBlok   * blok )
{
    (void)datacode;
    (void)blok;
    return blok - datacode->bloktab;
}


/**
 * Get the index of a halo column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverBlok structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t hblok_getnum( SolverMatrix * datacode,
                           SolverBlok   * blok )
{
    (void)datacode;
    (void)blok;
#ifdef PASTIX_WITH_STARPU
    return blok - datacode->hbloktab;
#else
    return -1;
#endif
}


/**
 * Get the index of a fanin column block.
 *
 * @param datacode SolverMatrix structure.
 * @param column block SolverCblk structure to test.
 *
 * @returns the index of the column block.
 */
static inline
pastix_int_t fcblk_getnum( SolverMatrix * datacode,
                           SolverCblk   * cblk,
                           pastix_int_t   procnum )
{
    (void)datacode;
    (void)cblk;
    (void)procnum;
#ifdef PASTIX_WITH_STARPU
    assert(cblk_isfanin(datacode, cblk) == API_YES);
    return cblk - datacode->fcblktab[procnum];

#else
    return -1;
#endif
}

static inline
pastix_int_t fcblk_getorigin( SolverMatrix * datacode,
                              SolverMatrix * cblk )
{
    (void)datacode;
    (void)cblk;
#ifdef PASTIX_WITH_STARPU
    pastix_int_t clustnum;
    for (clustnum = 0; clustnum < datacode->clustnbr; clustnum++) {
        if ((size_t)cblk >= (size_t)datacode->fcblktab[clustnum] &&
            (size_t)cblk < (size_t)(datacode->fcblktab[clustnum] +
                                    datacode->fcblknbr[clustnum])) {
            return clustnum;
        }
    }
#endif
    return -1;
}
/**
 * Get the number of columns of a column block.
 *
 * @param column block SolverCblk structure.
 *
 * @returns the number of columns in the column block.
 */
static inline
pastix_int_t cblk_colnbr( const SolverCblk *cblk ) {
    return cblk->lcolnum - cblk->fcolnum + 1;
}

/**
 * Get the number of columns of a column block.
 *
 * @param column block SolverCblk structure.
 *
 * @returns the number of columns in the column block.
 */
static inline
pastix_int_t cblk_bloknbr( const SolverCblk *cblk ) {
    return (cblk+1)->fblokptr - cblk->fblokptr + 1;
}

/**
 * Get the number of rows of a block.
 *
 * @param block SolverBlok structure.
 *
 * @returns the number of rows in the block.
 */
static inline
pastix_int_t blok_rownbr( const SolverBlok * blok ) {
    return blok->lrownum - blok->frownum + 1;
}

/**
 * Get the number of rows of a column block.
 *
 * @param column block SolverCblk structure.
 *
 * @returns the number of rows in the column block.
 */
static inline
pastix_int_t cblk_rownbr( const SolverCblk * cblk ) {
    pastix_int_t rownbr = 0;
    SolverBlok * blok;
    for (blok = cblk->fblokptr; blok < cblk[1].fblokptr; blok++)
        rownbr += blok_rownbr(blok);
    return rownbr;
}

/**
 * Indicate if a blok is included inside an other block.
 * i.e. indicate if the row range of the first block is included in the
 * one of the second.
 *
 * @param first block SolverBlok structure to test.
 * @param second block SolverBlok structure to test.
 *
 * @retval true   if the first block is     included in the second one.
 * @retval false  if the first block is not included in the second one.
 */
#  if defined(NAPA_SOPALIN)
static inline int is_block_inside_fblock( const SolverBlok *blok,
                                          const SolverBlok *fblok ) {
    return (((blok->frownum >= fblok->frownum) &&
             (blok->lrownum <= fblok->lrownum)) ||
            ((blok->frownum <= fblok->frownum) &&
             (blok->lrownum >= fblok->lrownum)) ||
            ((blok->frownum <= fblok->frownum) &&
             (blok->lrownum >= fblok->frownum)) ||
            ((blok->frownum <= fblok->lrownum) &&
             (blok->lrownum >= fblok->lrownum)));
}
#  else
static inline int is_block_inside_fblock( const SolverBlok *blok,
                                          const SolverBlok *fblok ) {
    return ((blok->frownum >= fblok->frownum) &&
            (blok->lrownum <= fblok->lrownum));
}


#  endif /* defined(NAPA_SOPALIN) */

pastix_int_t sizeofsolver(const SolverMatrix *solvptr,
                          pastix_int_t *iparm );

void solverInit(SolverMatrix *);
void solverExit(SolverMatrix *);

pastix_int_t solverLoad(SolverMatrix *solvptr, FILE *stream);
pastix_int_t solverSave(const SolverMatrix *solvptr, FILE *stream);

void          solverRealloc(SolverMatrix *solvptr);
SolverMatrix *solverCopy(const SolverMatrix *solvptr, int flttype);

int solverComputeGPUDistrib( SolverMatrix *solvmtx,
                             int           ngpus,
                             double        memory_percentage,
                             size_t        eltsize,
                             int           criterium,
                             int           factotype );

struct SolverBackup_s;
typedef struct SolverBackup_s SolverBackup_t;

SolverBackup_t *solverBackupInit( const SolverMatrix *solvmtx );
int             solverBackupRestore( SolverMatrix *solvmtx, const SolverBackup_t *b );
void            solverBackupExit( SolverBackup_t *b );

int             solverDraw( const SolverMatrix *solvptr, FILE *stream, int verbose );

#endif /* _SOLVER_H_*/
