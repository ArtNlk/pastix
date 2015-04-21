/**
 *
 * @file coeftab.c
 *
 *  PaStiX factorization routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "bcsc.h"

void
coeftab_zinitcblk( const SolverMatrix  *solvmtx,
                   const pastix_bcsc_t *bcsc,
                   pastix_int_t itercblk,
                   int fakefillin, int factoLU );

void
coeftab_cinitcblk( const SolverMatrix  *solvmtx,
                   const pastix_bcsc_t *bcsc,
                   pastix_int_t itercblk,
                   int fakefillin, int factoLU );

void
coeftab_dinitcblk( const SolverMatrix  *solvmtx,
                   const pastix_bcsc_t *bcsc,
                   pastix_int_t itercblk,
                   int fakefillin, int factoLU );

void
coeftab_sinitcblk( const SolverMatrix  *solvmtx,
                   const pastix_bcsc_t *bcsc,
                   pastix_int_t itercblk,
                   int fakefillin, int factoLU );

void
coeftab_zdump( const SolverMatrix *solvmtx,
               const char   *filename );

void
coeftab_cdump( const SolverMatrix *solvmtx,
               const char   *filename );

void
coeftab_ddump( const SolverMatrix *solvmtx,
               const char   *filename );

void
coeftab_sdump( const SolverMatrix *solvmtx,
               const char   *filename );

/* Section: Functions */

/*
 * Function: z_CoefMatrix_Allocate
 *
 * Allocate matrix coefficients in coeftab and ucoeftab.
 *
 * Should be first called with me = -1 to allocated coeftab.
 * Then, should be called with me set to thread ID
 * to allocate column blocks coefficients arrays.
 *
 * Parameters
 *
 *    datacode  - solverMatrix
 *    factotype - factorization type (LU, LLT ou LDLT)
 *    me        - thread number. (-1 for first call,
 *                from main thread. >=0 to allocate column blocks
 *     assigned to each thread.)
 */
void
coeftabInit( const SolverMatrix  *datacode,
             const pastix_bcsc_t *bcsc,
             int fakefillin,
             int factoLU )
{
    pastix_int_t i, itercblk;
    pastix_int_t bubnum = 0;
    pastix_int_t task;
    void (*initfunc)(const SolverMatrix*,
                     const pastix_bcsc_t*,
                     pastix_int_t,
                     int, int) = NULL;
    void (*dumpfunc)(const SolverMatrix*,
                     const char *) = NULL;

    switch( bcsc->flttype ) {
    case PastixComplex32:
        initfunc = coeftab_cinitcblk;
        dumpfunc = coeftab_cdump;
        break;
    case PastixComplex64:
        initfunc = coeftab_zinitcblk;
        dumpfunc = coeftab_zdump;
        break;
    case PastixFloat:
        initfunc = coeftab_sinitcblk;
        dumpfunc = coeftab_sdump;
        break;
    case PastixDouble:
    case PastixPattern:
    default:
        initfunc = coeftab_dinitcblk;
        dumpfunc = coeftab_ddump;
    }

    for (i=0; i < datacode->ttsknbr[bubnum]; i++)
    {
        task = datacode->ttsktab[bubnum][i];
        itercblk = datacode->tasktab[task].cblknum;

        initfunc( datacode, bcsc, itercblk, fakefillin, factoLU );
    }

    dumpfunc( datacode, "lcoeftab.txt" );
}
