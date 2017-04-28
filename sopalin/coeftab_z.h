/**
 * @file coeftab_z.h
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
 * @precisions normal z -> s d c
 *
 **/
#ifndef _coeftab_z_h_
#define _coeftab_z_h_

pastix_int_t coeftab_zcompress_one( SolverCblk *cblk, pastix_lr_t lowrank );
void coeftab_zalloc_one( SolverCblk *cblk );
void coeftab_zcompress( SolverMatrix *solvmtx );

void coeftab_zuncompress_one( SolverCblk *cblk, int factoLU );
void coeftab_zuncompress( SolverMatrix *solvmtx );

pastix_int_t coeftab_zmemory_one( SolverCblk *cblk, int factoLU );
void coeftab_zmemory( SolverMatrix *solvmtx );

void coeftab_zffbcsc( const SolverMatrix  *solvmtx,
                      const pastix_bcsc_t *bcsc,
                      pastix_int_t         itercblk );
void coeftab_zinitcblk( const SolverMatrix  *solvmtx,
                        const pastix_bcsc_t *bcsc,
                        pastix_int_t itercblk,
                        int fakefillin, int factoLU );

void coeftab_zdumpcblk( const SolverCblk *cblk,
                        void *array,
                        FILE *stream );
void coeftab_zdump( const SolverMatrix *solvmtx,
                    const char   *filename );

int coeftab_zdiff( const SolverMatrix *solvA,
                   SolverMatrix *solvB );

void coeftab_zgetschur( const SolverMatrix *solvmtx,
                        pastix_complex64_t *S, pastix_int_t lds );

#endif /* _coeftab_z_h_ */
