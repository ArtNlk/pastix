/* Copyright INRIA 2004
 **
 ** This file is part of the Scotch distribution.
 **
 ** The Scotch distribution is libre/free software; you can
 ** redistribute it and/or modify it under the terms of the
 ** GNU Lesser General Public License as published by the
 ** Free Software Foundation; either version 2.1 of the
 ** License, or (at your option) any later version.
 **
 ** The Scotch distribution is distributed in the hope that
 ** it will be useful, but WITHOUT ANY WARRANTY; without even
 ** the implied warranty of MERCHANTABILITY or FITNESS FOR A
 ** PARTICULAR PURPOSE. See the GNU Lesser General Public
 ** License for more details.
 **
 ** You should have received a copy of the GNU Lesser General
 ** Public License along with the Scotch distribution; if not,
 ** write to the Free Software Foundation, Inc.,
 ** 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 **
 ** $Id: symbol.c 285 2005-03-10 10:25:31Z pelegrin $
 */
/************************************************************/
/**                                                        **/
/**   NAME       : symbol.c                                **/
/**                                                        **/
/**   AUTHORS    : David GOUDIN                            **/
/**                Pascal HENON                            **/
/**                Francois PELLEGRINI                     **/
/**                Pierre RAMET                            **/
/**                                                        **/
/**   FUNCTION   : Part of a parallel direct block solver. **/
/**                These lines are the general purpose     **/
/**                routines for the symbolic matrix.       **/
/**                                                        **/
/**   DATES      : # Version 0.0  : from : 22 jul 1998     **/
/**                                 to     07 oct 1998     **/
/**                # Version 0.1  : from : 03 dec 1998     **/
/**                                 to     03 dec 1998     **/
/**                # Version 3.0  : from : 29 feb 2004     **/
/**                                 to     29 feb 2004     **/
/**                                                        **/
/************************************************************/

/*
 **  The defines and includes.
 */

#define SYMBOL

#include "common.h"
#include "symbol.h"
#include "order.h"

/* For split_level parameter */
/* The chosen level to reduce computational cost: no effects if set to 0 */
/* A first comparison is computed according to upper levels */
/* If hamming distances are equal, the computation goes through lower levels */

/* For stop_criteria parameter */
/* Criteria to limit the number of comparisons when computing hamming distances */

/* For stop_when_fitting parameter */
/* Criteria to insert a line when no extra-blok is created */
/* If set to 0, the algorithm will minimize the cut between two lines */

void
symbolNewOrdering( const SymbolMatrix *symbptr, Order *order,
                   pastix_int_t split_level, int stop_criteria,
                   int stop_when_fitting )
{
    pastix_int_t itercblk, iterblok;
    pastix_int_t edgenbr = symbptr->bloknbr - symbptr->cblknbr;
    pastix_int_t cblknbr = symbptr->cblknbr;

    pastix_int_t *crow = symbptr->crowtab;
    pastix_int_t *brow = symbptr->browtab;

    Clock timer;
    double time_compute_vectors = 0.;
    double time_update_perm     = 0.;

    pastix_int_t start = 0;
    pastix_int_t end   = edgenbr;
    pastix_int_t i;

    pastix_int_t *levels;


    /* Create the levels structure */
    levels = calloc(cblknbr, sizeof(pastix_int_t));

    /* Define the depth of each cblk */
    for (i=0; i<cblknbr; i++){
        pastix_int_t father = order->treetab[i];
        levels[i]++;
        while (father != -1){
            levels[i]++;
            father = order->treetab[father];
        }
    }

    iterblok = 0;
    for (itercblk=0; itercblk<cblknbr; itercblk++){
        clockStart(timer);

        pastix_int_t size  = order->rangtab[itercblk+1] - order->rangtab[itercblk];
        pastix_int_t stop = 0;

        int **vectors      = malloc(size * sizeof(int*));
        int * wmatrix      = malloc(size * size * sizeof(int)); /* Hamming distances */
        int * vectors_size = calloc(size, sizeof(int));
        int * current_pos  = calloc(size, sizeof(int));
        memset(wmatrix, -1, size*size*sizeof(int));

        int **up_vectors      = malloc(size * sizeof(int*));
        int * up_wmatrix      = malloc(size * size * sizeof(int));
        int * up_vectors_size = calloc(size, sizeof(int));
        int * up_current_pos  = calloc(size, sizeof(int));
        memset(up_wmatrix, -1, size*size*sizeof(int));

        pastix_int_t saved_iterblok = iterblok;


        /* Current for each line within the current cblk the number of contributions */
        while (iterblok < end && stop == 0){
            SymbolBlok *blok = symbptr->bloktab + brow[iterblok];

            if (blok->frownum <  order->rangtab[itercblk] ||
                blok->lrownum >= order->rangtab[itercblk+1]){
                stop = 1;
            }
            else{
                /* For upper levels in nested dissection */
                if (levels[crow[iterblok]] <= split_level){
                    for (i=blok->frownum; i<=blok->lrownum; i++){
                        int index = i - order->rangtab[itercblk];
                        up_vectors_size[index]++;
                    }
                }
                else{
                    for (i=blok->frownum; i<=blok->lrownum; i++){
                        int index = i - order->rangtab[itercblk];
                        vectors_size[index]++;
                    }
                }
                iterblok++;
            }
        }


        /* Initiate vectors structure */
        for (i=0; i<size; i++){
            vectors[i]    = calloc(vectors_size[i],    sizeof(int));
            up_vectors[i] = calloc(up_vectors_size[i], sizeof(int));
        }

        iterblok = saved_iterblok;
        stop = 0;


        /* Fill-in vectors structure with contributing cblks */
        while (iterblok < end && stop == 0){
            SymbolBlok *blok = symbptr->bloktab + brow[iterblok];

            if (blok->frownum <  order->rangtab[itercblk] ||
                blok->lrownum >= order->rangtab[itercblk+1]){
                stop = 1;
            }
            else{
                /* For upper levels in nested dissection */
                if (levels[crow[iterblok]] <= split_level){
                    for (i=blok->frownum; i<=blok->lrownum; i++){
                        int index = i - order->rangtab[itercblk];
                        up_vectors[index][up_current_pos[index]] = crow[iterblok];
                        up_current_pos[index]++;
                    }
                }
                else{
                    for (i=blok->frownum; i<=blok->lrownum; i++){
                        int index = i - order->rangtab[itercblk];
                        vectors[index][current_pos[index]] = crow[iterblok];
                        current_pos[index]++;
                    }
                }
                iterblok++;
            }
        }

        clockStop(timer);
        time_compute_vectors += clockVal(timer);

        clockStart(timer);
        /* Permute lines in the current supernode */
        update_perm(size, order, itercblk,
                    wmatrix, vectors, vectors_size,
                    up_wmatrix, up_vectors, up_vectors_size,
                    stop_criteria, stop_when_fitting);
        clockStop(timer);
        time_update_perm += clockVal(timer);

        for (i=0; i<size; i++){
            free(vectors[i]);
            free(up_vectors[i]);
        }
        free(vectors);
        free(wmatrix);
        free(vectors_size);
        free(current_pos);
        free(up_vectors);
        free(up_wmatrix);
        free(up_vectors_size);
        free(up_current_pos);
    }

    printf("Time to compute vectors  %lf s\n", time_compute_vectors);
    printf("Time to update  perm     %lf s\n", time_update_perm);

    /* Update the permutation */
    for (i=0; i<symbptr->nodenbr; i++) {
        order->permtab[ order->peritab[i] ] = i;
    }
    free(levels);
}

int hamming_distance_symbol(int n, int **vectors, int *vectors_size,
                            int xi, int xj, int stop)
{
    int sum = 0;
    int *set1 = vectors[xi];
    int *set2 = vectors[xj];
    int *end1 = vectors[xi] + vectors_size[xi];
    int *end2 = vectors[xj] + vectors_size[xj];

    if (vectors_size[xi] - vectors_size[xj] > stop){
        return stop;
    }
    if (vectors_size[xj] - vectors_size[xi] > stop){
        return stop;
    }

    while((set1 < end1) && (set2 < end2))
    {
        if( *set1 == *set2)
        {
            set1++;
            set2++;
        }
        else if( *set1 < *set2 )
        {
            sum ++;
            set1++;
        }
        else if( *set1 > *set2 )
        {
            sum ++;
            set2++;
        }
        else
        {
            assert(0);
        }

        /* The computation is stopped if sum overlapped a given limit */
        if (sum > stop){
            return sum;
        }
    }

    sum += end1-set1;
    sum += end2-set2;
    return sum;
}


void update_perm(int sn_nvertex, Order *order, int sn_id,
                 int *wmatrix, int **vectors, int *vectors_size,
                 int *up_wmatrix, int **up_vectors, int *up_vectors_size,
                 int stop_criteria, int stop_when_fitting){

    if ( sn_nvertex < 3 ) {
        return;
    }

    int  i, j, k, l, elected;
    int *tmpinvp = malloc(sn_nvertex*sizeof(int));
    int *tmplen  = malloc(sn_nvertex*sizeof(int));
    memset(tmplen, 0, sn_nvertex*sizeof(int));

    tmpinvp[0] = 0;
    tmpinvp[1] = 1;

    wmatrix[ 1 * sn_nvertex + 0 ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, 1, 0, stop_criteria);

    tmplen[0] = wmatrix[ 1 * sn_nvertex + 0 ];
    tmplen[1] = wmatrix[ 1 * sn_nvertex + 0 ];

    for(i=2; i<sn_nvertex; i++) {

        /* Start by adding the row in first position */
        wmatrix[ i * sn_nvertex + tmpinvp[0] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[0], stop_criteria);
        wmatrix[ i * sn_nvertex + tmpinvp[1] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[1], stop_criteria);

        int minl =
            wmatrix[ i * sn_nvertex + tmpinvp[0] ] +
            wmatrix[ i * sn_nvertex + tmpinvp[1] ] - tmplen[0];
        int mpos = 1;

        int min_cut = -1;

        int stop_loc = 1000;
        for(j=1; j<i-1; j++ ){

            int DEEP_stop = 1;

            if (up_wmatrix[ i * sn_nvertex + tmpinvp[j]] == -1)
                up_wmatrix[ i * sn_nvertex + tmpinvp[j]   ] = hamming_distance_symbol(sn_nvertex, up_vectors, up_vectors_size, i, tmpinvp[j], DEEP_stop);

            if (up_wmatrix[ i * sn_nvertex + tmpinvp[j+1]] == -1)
                up_wmatrix[ i * sn_nvertex + tmpinvp[j+1] ] = hamming_distance_symbol(sn_nvertex, up_vectors, up_vectors_size, i, tmpinvp[j+1], DEEP_stop);

            if ( up_wmatrix[ i * sn_nvertex + tmpinvp[j  ]] < DEEP_stop ||
                 up_wmatrix[ i * sn_nvertex + tmpinvp[j+1]] < DEEP_stop )
            {
                if (wmatrix[ i * sn_nvertex + tmpinvp[j]] == -1)
                    wmatrix[ i * sn_nvertex + tmpinvp[j]   ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[j], stop_criteria);

                if (wmatrix[ i * sn_nvertex + tmpinvp[j+1]] == -1)
                    wmatrix[ i * sn_nvertex + tmpinvp[j+1] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[j+1], stop_criteria);

                l = wmatrix[ i * sn_nvertex + tmpinvp[j]   ] +
                    wmatrix[ i * sn_nvertex + tmpinvp[j+1] ] - tmplen[j];

                if ( l < minl ) {
                    minl = l; mpos = j+1;

                    min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j]];
                    if (wmatrix[ i * sn_nvertex + tmpinvp[j+1]] < min_cut){
                        min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j+1]];
                    }

                    /* WARNING: SUPPLEMENTARY TEST */
                    if (stop_when_fitting == 1){
                        if (l == 0){
                            j = i;
                        }
                    }
                }

                if (stop_when_fitting == 0){
                    if ( l == minl ) {
                        if (wmatrix[ i * sn_nvertex + tmpinvp[j]] < min_cut){
                            min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j]];
                            minl = l; mpos = j+1;
                        }
                        if (wmatrix[ i * sn_nvertex + tmpinvp[j+1]] < min_cut){
                            min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j+1]];
                            minl = l; mpos = j+1;
                        }
                    }

                    if (wmatrix[ i * sn_nvertex + tmpinvp[j]] == 0){
                        min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j]];
                        minl = l; mpos = j+1;
                        j = i;
                    }
                    else if (wmatrix[ i * sn_nvertex + tmpinvp[j+1]] == 0){
                        min_cut = wmatrix[ i * sn_nvertex + tmpinvp[j+1]];
                        minl = l; mpos = j+1;
                        j = i;
                    }
                }
            }
        }

        /* Test between last and first */
        wmatrix[ i * sn_nvertex + tmpinvp[i-1] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[i-1], stop_criteria);

        l = wmatrix[ i * sn_nvertex + tmpinvp[i-1] ] +
            wmatrix[ i * sn_nvertex + tmpinvp[0  ] ] - tmplen[i-1];
        if ( l < minl ) {
            minl = l; mpos = i;
        }

        if (mpos > 0){
            wmatrix[ i * sn_nvertex + tmpinvp[mpos-1] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[mpos-1], stop_criteria);
            tmplen[mpos-1] = wmatrix[ i * sn_nvertex + tmpinvp[mpos-1] ];
        }

        if (mpos < i)
        {
            int tmpi, tmpl;
            k = i;

            wmatrix[ i * sn_nvertex + tmpinvp[mpos] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[mpos], stop_criteria);
            l = wmatrix[ i * sn_nvertex + tmpinvp[mpos] ];

            /* Insert the line in the tmpinvp/tmplen arrays */
            for(j=mpos; j<i+1; j++ )
            {
                tmpi = tmpinvp[j];
                tmpl = tmplen[j];

                tmpinvp[j] = k;
                tmplen[j]  = l;

                k = tmpi;
                l = tmpl;
            }
        }
        else {
            tmpinvp[i] = i;

            wmatrix[ i * sn_nvertex + tmpinvp[0] ] = hamming_distance_symbol(sn_nvertex, vectors, vectors_size, i, tmpinvp[0], stop_criteria);

            tmplen[i]  = wmatrix[ i * sn_nvertex + tmpinvp[0] ];
        }
    }

    /* Search the best split line */
    int min_size = INT_MAX;
    for (i=0; i<sn_nvertex; i++)
    {
        if (vectors_size[i] < min_size){
            min_size = vectors_size[i];
        }
    }

    elected = 0;
    for (i=0; i<sn_nvertex; i++)
    {
        if (vectors_size[tmpinvp[i]] == min_size){
            elected = i;
        }
    }

    pastix_int_t *sn_connected;
    MALLOC_INTERN(sn_connected, sn_nvertex, pastix_int_t);
    {
        pastix_int_t *peritab = order->peritab + order->rangtab[sn_id];
        for (i=0; i<sn_nvertex; i++)
        {
            sn_connected[i] = peritab[ tmpinvp[(i + elected)%sn_nvertex] ];
        }
        memcpy( peritab, sn_connected, sn_nvertex * sizeof(pastix_int_t) );
    }
    free(sn_connected);
    free(tmpinvp);
    free(tmplen);
}
