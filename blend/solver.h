/**
 * @file solver.h
 *
 * PaStiX solver structure header.
 *
 * @copyright 2004-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author David Goudin
 * @author Pascal Henon
 * @author Francois Pellegrini
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Xavier Lacoste
 * @date 2017-04-19
 *
 **/
#ifndef _SOLVER_H_
#define _SOLVER_H_

struct blendctrl_s;
typedef struct blendctrl_s BlendCtrl;

struct simuctrl_s;
typedef struct simuctrl_s SimuCtrl;

/**
 * @name Cblk properties
 * @{
 *  The type and structure definitions.
 *  Define the mask for the cblks in the cblktype field:
 *   - 1st bit: The cblk is a fake local cblk corresponding to a fanin that will be sent to someone else
 *   - 2nd bit: The cblk is stored in a 2D layout fashion as in a tiled matrix, otherwise the standard 1D lapack layout is used
 *   - 3rd bit: The cblk generates 2D granularity tasks, instead of a single 1D tasks that perform factorization, solves and updates
 *   - 4th bit: The cblk is compressed in Low-Rank (implies CBLK_LAYOUT_2D), otherwise it is stored in dense
 *   - 5th bit: The cblk is part of the Schur complement if set
 */
#define CBLK_FANIN      (1 << 0)
#define CBLK_LAYOUT_2D  (1 << 1)
#define CBLK_TASKS_2D   (1 << 2)
#define CBLK_COMPRESSED (1 << 3)
#define CBLK_IN_SCHUR   (1 << 4)

/**
 *@}
 */

/*
 * The type and structure definitions.
 */
#define COMP_1D                     0
#define DIAG                        1
#define E1                          2
#define E2                          3
#define DRUNK                       4

/**
 * @brief The task structure for the numerical factorization
 */
typedef struct task_s {
    pastix_int_t          taskid;  /**< COMP_1D DIAG E1 E2                                        */
    pastix_int_t          prionum; /**< Priority value for the factorization                      */
    pastix_int_t          cblknum; /**< Attached column block                                     */
    pastix_int_t          bloknum; /**< Attached block                                            */
    pastix_int_t volatile ftgtcnt; /**< Number of fan-in targets                                  */
    pastix_int_t volatile ctrbcnt; /**< Total number of contributions                             */
    pastix_int_t          indnum;  /**< For E2 (COMP_1D), index of ftgt (>0) else if local = -taskdest
                                        For DIAG and E1, index of btag (>0) if there is a
                                        local one it must be the first of the chain of local E1   */
#if defined(PASTIX_DYNSCHED)
    pastix_int_t          threadid;/**< Index of the bubble which contains the task               */
#endif
} Task;

/**
 * @brief The block low-rank structure to hold the information
 */
typedef struct pastix_lrblock_s {
    int   rk;    /**< Rank of the low-rank matrix: -1 is dense, otherwise rank-rk matrix           */
    int   rkmax; /**< Leading dimension of the matrix u                                            */
    void *u;     /**< Contains the dense matrix if rk=-1, or the u factor from u vT representation */
    void *v;     /**< Not referenced if rk=-1, otherwise, the v factor                             */
} pastix_lrblock_t;

/**
 * @brief Type of the functions to compress a dense block into a low-rank form.
 */
typedef void (*fct_ge2lr_t)( pastix_fixdbl_t, pastix_int_t, pastix_int_t,
                             const void *, pastix_int_t, void * );

/**
 * @brief Type of the functions to add two low-rank blocks together.
 */
typedef int  (*fct_rradd_t)( pastix_fixdbl_t, pastix_trans_t, const void *,
                             pastix_int_t, pastix_int_t, const pastix_lrblock_t *,
                             pastix_int_t, pastix_int_t,       pastix_lrblock_t *,
                             pastix_int_t, pastix_int_t );

/**
 * @brief Structure to define the type of function to use for the low-rank
 *        kernels and their parmaeters.
 */
typedef struct pastix_lr_s {
    pastix_int_t compress_when;       /**< When to compress in the full solver              */
    pastix_int_t compress_method;     /**< Compression method                               */
    pastix_int_t compress_min_width;  /**< Minimum width to compress a supernode            */
    pastix_int_t compress_min_height; /**< Minimum height to compress an off-diagonal block */
    double       tolerance;           /**< Absolute compression tolerance                   */
    fct_rradd_t  core_rradd;          /**< Recompression function                           */
    fct_ge2lr_t  core_ge2lr;          /**< Compression function                             */
} pastix_lr_t;

/**
 * @brief Fan-in target information fields
 * @warning The number of fields must be odd for memory alignment purpose.
 */
typedef enum {
    FTGT_CTRBNBR = 0,           /**< Number of contributions            */
    FTGT_CTRBCNT,               /**< Number of contributions remaining  */
    FTGT_PROCDST,               /**< Destination for fanintarget        */
    FTGT_TASKDST,               /**< Task  destination                  */
    FTGT_BLOKDST,               /**< Block destination (->COMP_1D)      */
    FTGT_PRIONUM,               /**< Fanintarget priority               */
    FTGT_FCOLNUM,               /**< Fanintarget first column           */
    FTGT_LCOLNUM,               /**< Fanintarget last column            */
    FTGT_FROWNUM,               /**< Fanintarget first row              */
    FTGT_LROWNUM,               /**< Fanintarget last row               */
#if defined(OOC) || defined(PASTIX_WITH_STARPU)
    FTGT_GCBKDST,               /**< Global Cblk destination(->COMP_1D) */
    FTGT_IDTRACE,               /**< To have 12 integer in FanInTarget  */
#endif
    FTGT_MAXINFO
} solver_ftgt_e;

/**
 * @brief Fan-in target structure for data exchange
 */
typedef struct solver_ftgt_s {
    pastix_int_t   infotab[FTGT_MAXINFO]; /**< Fan-in target header holding all information enumerated in solver_ftgt_e */
    void          *coeftab;               /**< Fan-in target coeficient array                                           */
} solver_ftgt_t;


#define GPUID_UNDEFINED -2 /**< GPU still undefined       */
#define GPUID_NONE      -1 /**< Block not computed on GPU */

/**
 * @brief Solver block structure.
 */
typedef struct solver_blok_s {
    void        *handler[2]; /**< Runtime data handler                     */
    pastix_int_t lcblknm;    /**< Local column block                       */
    pastix_int_t fcblknm;    /**< Facing column block                      */
    pastix_int_t frownum;    /**< First row index                          */
    pastix_int_t lrownum;    /**< Last row index (inclusive)               */
    pastix_int_t coefind;    /**< Index in coeftab                         */
    pastix_int_t browind;    /**< Index in browtab                         */
    int8_t       gpuid;      /**< Store on which GPU the block is computed */

    /* LR structures */
    pastix_lrblock_t *LRblock; /**< Store the blok (L/U) in LR format. Allocated for the cblk. */
} SolverBlok;

/**
 * @brief Solver column block structure.
 */
typedef struct solver_cblk_s  {
    pastix_atomic_lock_t lock;       /**< Lock to protect computation on the cblk */
    volatile uint32_t    ctrbcnt;    /**< Number of contribution to receive       */
    int8_t               cblktype;   /**< Type of cblk                            */
    int8_t               gpuid;      /**< Store on which GPU the cblk is computed */
    pastix_int_t         fcolnum;    /**< First column index                      */
    pastix_int_t         lcolnum;    /**< Last column index (inclusive)           */
    SolverBlok          *fblokptr;   /**< First block in column (diagonal)        */
    pastix_int_t         stride;     /**< Column block stride                     */
    pastix_int_t         lcolidx;    /**< Local first column index to the location in the rhs vector       */
    pastix_int_t         brownum;    /**< First block in row facing the diagonal block in browtab, 0-based */
    pastix_int_t         brown2d;    /**< First 2D-block in row facing the diagonal block in browtab, 0-based */
    pastix_int_t         gcblknum;   /**< Global column block index               */
    void                *lcoeftab;   /**< Coefficients access vector              */
    void                *ucoeftab;   /**< Coefficients access vector              */
    void                *handler[2]; /**< Runtime data handler                    */
    pastix_int_t         procdiag;   /**< Cluster owner of diagonal block (@todo: check if really required) */
} SolverCblk;

/**
 * @brief Solver column block structure.
 *
 * This structure stores all the numerical information about the factorization,
 * as well as the structure of the problem. Only local information to each
 * process is stored in this structure.
 *
 */
struct solver_matrix_s {
    int restore; /**< Flag to indicate if it is require to restore data with
                      solverBackupRestore: 0: No need, 1:After solve,
                      2:After Factorization */
    pastix_int_t            baseval;   /**< Base value for numberings                         */
    pastix_int_t            nodenbr;   /**< Number of nodes before dof extension              */
    pastix_int_t            coefnbr;   /**< Number of coefficients (node after dof extension) */
    pastix_int_t            gcblknbr;  /**< Global number of column blocks                    */
    pastix_int_t            cblknbr;   /**< Number of column blocks                   */
    pastix_int_t            cblkmin2d; /**< Rank of the first cblk beeing enabled for 2D computations        */
    pastix_int_t            cblkmaxblk;/**< Maximum number of blocks per cblk         */
    pastix_int_t            bloknbr;   /**< Number of blocks                          */
    pastix_int_t            brownbr;   /**< Size of the browtab array                 */
    SolverCblk   * restrict cblktab;   /**< Array of solver column blocks             */
    SolverBlok   * restrict bloktab;   /**< Array of solver blocks                    */
    pastix_int_t * restrict browtab;   /**< Array of blocks                           */

    pastix_lr_t             lowrank;   /**< Low-rank parameters                       */
    pastix_factotype_t      factotype; /**< General or symmetric factorization?       */

#if defined(PASTIX_WITH_PARSEC)
    sparse_matrix_desc_t   *parsec_desc;
#endif

    pastix_int_t              ftgtnbr;              /*+ Number of fanintargets                    +*/
    pastix_int_t              ftgtcnt;              /*+ Number of fanintargets to receive         +*/
    solver_ftgt_t * restrict  ftgttab;              /*+ Fanintarget access vector                 +*/

    pastix_int_t              diagmax;              /*+ Maximum size required during diagonal block factorization (hetrf/sytrf) +*/
    pastix_int_t              gemmmax;              /*+ Maximum size required during GEMM computation                           +*/
    pastix_int_t              nbftmax;              /*+ Maximum block number in ftgt              +*/
    pastix_int_t              arftmax;              /*+ Maximum block area in ftgt                +*/

    pastix_int_t              clustnum;             /*+ current processor number                  +*/
    pastix_int_t              clustnbr;             /*+ number of processors                      +*/
    pastix_int_t              procnbr;              /*+ Number of physical processor used         +*/
    pastix_int_t              thrdnbr;              /*+ Number of local computation threads       +*/
    pastix_int_t              bublnbr;              /*+ Number of local computation threads       +*/
    /* BubbleTree   * restrict   btree;                /\*+ Bubbles tree                              +*\/ */

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
 * @brief     Compute the number of columns in a column block.
 * @param[in] cblk
 *            The pointer to the column block.
 * @return    The number of columns in the cblk.
 */
static inline pastix_int_t
cblk_colnbr( const SolverCblk *cblk )
{
    return cblk->lcolnum - cblk->fcolnum + 1;
}

/**
 * @brief     Compute the number of blocks in a column block.
 * @param[in] cblk
 *            The pointer to the column block.
 * @return    The number of blocks in the cblk including the diagonal block.
 */
static inline pastix_int_t
cblk_bloknbr( const SolverCblk *cblk )
{
    return (cblk+1)->fblokptr - cblk->fblokptr + 1;
}

/**
 * @brief     Compute the number of rows of a block.
 * @param[in] blok
 *            The pointer to the block.
 * @return    The number of rows in the block.
 */
static inline pastix_int_t
blok_rownbr( const SolverBlok *blok )
{
    return blok->lrownum - blok->frownum + 1;
}

/**
 * @brief     Compute the number of rows of a column block.
 * @param[in] cblk
 *            The pointer to the column block.
 * @return    The number of rows in the column block.
 */
static inline pastix_int_t
cblk_rownbr( const SolverCblk *cblk )
{
    pastix_int_t rownbr = 0;
    SolverBlok * blok;
    for (blok = cblk->fblokptr; blok < cblk[1].fblokptr; blok++)
        rownbr += blok_rownbr(blok);
    return rownbr;
}

/**
 * @brief Check if a block is included inside another one.
 *
 * Indicate if a blok is included inside an other block.
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
is_block_inside_fblock( const SolverBlok *blok,
                        const SolverBlok *fblok )
{
#  if defined(NAPA_SOPALIN)
    return (((blok->frownum >= fblok->frownum) &&
             (blok->lrownum <= fblok->lrownum)) ||
            ((blok->frownum <= fblok->frownum) &&
             (blok->lrownum >= fblok->lrownum)) ||
            ((blok->frownum <= fblok->frownum) &&
             (blok->lrownum >= fblok->frownum)) ||
            ((blok->frownum <= fblok->lrownum) &&
             (blok->lrownum >= fblok->lrownum)));
#  else
    return ((blok->frownum >= fblok->frownum) &&
            (blok->lrownum <= fblok->lrownum));
#  endif /* defined(NAPA_SOPALIN) */
}

void solverInit( SolverMatrix *solvmtx );
void solverExit( SolverMatrix *solvmtx );

int  solverMatrixGen( pastix_int_t        clustnum,
                      SolverMatrix       *solvmtx,
                      const SymbolMatrix *symbmtx,
                      const SimuCtrl     *simuctl,
                      const BlendCtrl    *ctrl );

int  solverLoad( SolverMatrix       *solvptr,
                 FILE               *stream );
int  solverSave( const SolverMatrix *solvptr,
                 FILE               *stream );

void          solverRealloc( SolverMatrix       *solvptr);
SolverMatrix *solverCopy   ( const SolverMatrix *solvptr,
                             int                 flttype );


int           solverDraw      ( const SolverMatrix *solvptr,
                                FILE               *stream,
                                int                 verbose );
void          solverPrintStats( const SolverMatrix *solvptr );

/*
 * Solver backup
 */
struct SolverBackup_s;
typedef struct SolverBackup_s SolverBackup_t;

SolverBackup_t *solverBackupInit   ( const SolverMatrix *solvmtx                          );
int             solverBackupRestore(       SolverMatrix *solvmtx, const SolverBackup_t *b );
void            solverBackupExit   (                                    SolverBackup_t *b );

#endif /* _SOLVER_H_*/
