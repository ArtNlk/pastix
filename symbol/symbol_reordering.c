/**
 *
 * @file symbol_reordering.c
 *
 *  PaStiX symbol structure routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2015-04
 *
 **/
#include "common.h"
#include "symbol.h"
#include "order.h"

static inline pastix_int_t
compute_cblklevel( const pastix_int_t *treetab,
                   pastix_int_t *levels,
                   pastix_int_t  cblknum )
{
    /* cblknum level has already been computed */
    if ( levels[cblknum] != 0 ) {
        return levels[cblknum];
    }
    else {
        pastix_int_t father = treetab[cblknum];

        if ( father == -1 ) {
            return 1;
        }
        else {
            return compute_cblklevel( treetab, levels, father ) + 1;
        }
    }
}

static inline pastix_int_t
hamming_distance(pastix_int_t **vectors,
                 pastix_int_t  *vectors_size,
                 pastix_int_t   xi,
                 pastix_int_t   xj,
                 pastix_int_t   stop)
{
    /* For the fictive vertex */
    if (xi == -1){
        return vectors_size[xj];
    }
    if (xj == -1){
        return vectors_size[xi];
    }

    pastix_int_t sum = 0;
    pastix_int_t *set1 = vectors[xi];
    pastix_int_t *set2 = vectors[xj];
    pastix_int_t *end1 = vectors[xi] + vectors_size[xi];
    pastix_int_t *end2 = vectors[xj] + vectors_size[xj];

    if (vectors_size[xi] - vectors_size[xj] >= stop){
        return stop;
    }
    if (vectors_size[xj] - vectors_size[xi] >= stop){
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
            while (( set1 < end1 ) && ( *set1 < *set2 ))
            {
                sum ++;
                set1++;
            }
        }
        else if( *set1 > *set2 )
        {
            while (( set2 < end2 ) && ( *set1 > *set2 ))
            {
                sum ++;
                set2++;
            }
        }
        else
        {
            assert(0);
        }

        /* The computation is stopped if sum overlapped a given limit */
        if (sum >= stop){
            return stop;
        }
    }

    sum += end1-set1;
    sum += end2-set2;

    if (sum >= stop){
        return stop;
    }

    return sum;
}


static inline void
symbol_reorder_tsp(pastix_int_t size, Order *order, pastix_int_t sn_id,
                   pastix_int_t **lw_vectors, pastix_int_t *lw_vectors_size,
                   pastix_int_t **up_vectors, pastix_int_t *up_vectors_size,
                   pastix_int_t stop_criteria)
{

    if ( size < 3 ) {
        return;
    }

    pastix_int_t  i, j, k, l, elected;
    pastix_int_t *tmpinvp;
    pastix_int_t *tmplen;
    pastix_int_t distance;

    MALLOC_INTERN(tmpinvp, size+1, pastix_int_t);
    MALLOC_INTERN(tmplen,  size+1, pastix_int_t);
    memset(tmplen, 0, (size+1)*sizeof(pastix_int_t));

    tmpinvp[0] = -1;
    tmpinvp[1] = 0;

    distance = hamming_distance(lw_vectors, lw_vectors_size, 0, -1, stop_criteria);

    tmplen[0] = distance;
    tmplen[1] = distance;

    pastix_int_t min_cut = -1;
    for(i=1; i<size; i++) {
        pastix_int_t first_pos;
        pastix_int_t last_pos;

        pastix_int_t lw_before_pos;
        pastix_int_t lw_after_pos;

        pastix_int_t up_before_pos;
        pastix_int_t up_after_pos;

        /* Start by adding the row in first position */
        lw_before_pos = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[0], stop_criteria);
        lw_after_pos  = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[1], stop_criteria);
        up_after_pos  = hamming_distance(up_vectors, up_vectors_size, i, tmpinvp[1], 1);

        pastix_int_t minl = lw_before_pos + lw_after_pos - tmplen[0];
        pastix_int_t mpos = 1;
        pastix_int_t min_cut = -1;

        for(j=1; j<i; j++ ){
            up_before_pos = up_after_pos;
            up_after_pos  = hamming_distance(up_vectors, up_vectors_size, i, tmpinvp[j+1], 1);

            if ( up_before_pos < 1 ||
                 up_after_pos  < 1 )
            {

                /* If split was used previously, this first distance may not be already computed */
                if (lw_after_pos == -1)
                    lw_before_pos = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[j], stop_criteria);
                else
                    lw_before_pos = lw_after_pos;


                lw_after_pos = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[j+1], stop_criteria);

                l = lw_before_pos + lw_after_pos - tmplen[j];


                /* Minimize the cut between two lines, for the same TSP result */
                if ( l == minl ) {
                    if (lw_before_pos < min_cut){
                        min_cut = lw_before_pos;
                        minl = l; mpos = j+1;
                    }
                    if (lw_after_pos < min_cut){
                        min_cut = lw_after_pos;
                        minl = l; mpos = j+1;
                    }
                }

                /* Position that minimizes TSP */
                if ( l < minl ) {
                    minl = l; mpos = j+1;

                    min_cut = lw_before_pos;
                    if (lw_after_pos < min_cut){
                        min_cut = lw_after_pos;
                    }
                }

                if ( l < minl ) {
                    minl = l; mpos = j+1;
                    min_cut = lw_before_pos;
                    if (lw_after_pos < min_cut){
                        min_cut = lw_after_pos;
                    }
                }


                /* Stop if two lines are equal (already done tmpinvp[j]) */
                if (lw_after_pos == 0){
                    min_cut = 0;
                    minl = l; mpos = j+1;
                    j = i;
                }
            }
            else{
                lw_after_pos = -1;
            }

        }

        /* Test between last and first */
        first_pos = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[0], stop_criteria);
        last_pos  = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[i], stop_criteria);

        lw_before_pos = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[mpos-1], stop_criteria);
        lw_after_pos  = hamming_distance(lw_vectors, lw_vectors_size, i, tmpinvp[mpos  ], stop_criteria);

        l = first_pos + last_pos - tmplen[i];
        if ( l < minl ) {
            minl = l; mpos = i+1;
        }

        if (mpos > 0){
            tmplen[mpos-1] = lw_before_pos;
        }

        if (mpos < (i+1))
        {
            pastix_int_t tmpi, tmpl;
            k = i;
            l = lw_after_pos;

            /* Insert the line in the tmpinvp/tmplen arrays */
            for(j=mpos; j<i+2; j++ )
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
            tmpinvp[i+1] = i;
            tmplen[i+1]  = first_pos;
        }
    }

    elected = 0;
    for (i=0; i<size; i++)
    {
        if (tmpinvp[i] == -1){
            elected = i;
        }
    }

    pastix_int_t *sn_connected;
    MALLOC_INTERN(sn_connected, size, pastix_int_t);
    {
        pastix_int_t *peritab = order->peritab + order->rangtab[sn_id];
        for (i=0; i<size; i++)
        {
            sn_connected[i] = peritab[ tmpinvp[(i + 1 + elected)%(size+1)] ];
        }
        memcpy( peritab, sn_connected, size * sizeof(pastix_int_t) );
    }

    memFree_null(sn_connected);
    memFree_null(tmpinvp);
    memFree_null(tmplen);
}

static inline void
symbol_reorder_cblk( const SymbolMatrix *symbptr,
                     const SymbolCblk   *cblk,
                     Order              *order,
                     const pastix_int_t *levels,
                     pastix_int_t        cblklvl,
                     pastix_int_t       *depthweight,
                     pastix_int_t        depthmax,
                     pastix_int_t        split_level,
                     int                 stop_criteria,
                     double             *time_compute_vectors,
                     double             *time_update_perm)
{
    SymbolBlok *blok;
    pastix_int_t **up_vectors, *up_vectors_size;
    pastix_int_t **lw_vectors, *lw_vectors_size;
    pastix_int_t size = cblk->lcolnum - cblk->fcolnum + 1;
    pastix_int_t local_split_level = split_level;
    pastix_int_t i, iterblok;
    pastix_int_t *brow = symbptr->browtab;
    double timer;

    /**
     * Compute hamming vectors in two subsets:
     *   - The upper subset contains the cblk with level higher than the split_level
     *     in the elimination tree, (or depth lower than levels[cblk])
     *   - The lower subset contains the cblk with level lower than the split_level
     *     in the elimination tree, (or depth higher than levels[cblk])
     *
     * The delimitation between the lower and upper levels is made such that
     * the upper level represents 17% to 25% of the total number of cblk.
     */
    clockStart(timer);
    {
        pastix_int_t weight = 0;

        /* Compute the weigth of each level */
        for(iterblok=cblk[0].brownum; iterblok<cblk[1].brownum; iterblok++)
        {
            pastix_int_t blokweight;
            blok = symbptr->bloktab + brow[iterblok];
            blokweight = blok->lrownum - blok->frownum + 1;
            depthweight[ levels[ blok->lcblknm ] - 1 ] += blokweight;
            weight += blokweight;
        }

        /**
         * Compute the split_level:
         *    We start with the given split_level parameter
         *    and we try to correct it to minimize the following iterative process
         */
        {
            /* Current for each line within the current cblk the number of contributions */
            pastix_int_t up_total = 0;
            pastix_int_t lw_total = 0;
            pastix_int_t sign = 0;

          split:
            up_total = 0;
            lw_total = 0;

            for(i=0; i<local_split_level; i++)
            {
                up_total += depthweight[i];
            }
            for(; i<depthmax; i++)
            {
                lw_total += depthweight[i];
            }

            /* If there are too many upper bloks */
            if ( (lw_total < (5 * up_total)) &&
                 (lw_total > 10) && (up_total > 10) && (sign <= 0))
            {
                local_split_level--;
                sign--;
                goto split;
            }

            /* If there are too many lower bloks */
            if ( (lw_total > (3 * up_total)) &&
                 (lw_total > 10) && (up_total > 10) && (sign >= 0) )
            {
                local_split_level++;
                sign++;
                goto split;
            }
        }

        /* Compute the Hamming vector size for each row of the cblk */
        MALLOC_INTERN(up_vectors_size, size, pastix_int_t);
        memset(up_vectors_size, 0, size * sizeof(pastix_int_t));
        MALLOC_INTERN(lw_vectors_size, size, pastix_int_t);
        memset(lw_vectors_size, 0, size * sizeof(pastix_int_t));

        for(iterblok=cblk[0].brownum; iterblok<cblk[1].brownum; iterblok++)
        {
            blok = symbptr->bloktab + brow[iterblok];

            /* For upper levels in nested dissection */
            if (levels[blok->lcblknm] <= local_split_level){
                for (i=blok->frownum; i<=blok->lrownum; i++){
                    pastix_int_t index = i - cblk->fcolnum;
                    up_vectors_size[index]++;
                }
            }
            else{
                for (i=blok->frownum; i<=blok->lrownum; i++){
                    pastix_int_t index = i - cblk->fcolnum;
                    lw_vectors_size[index]++;
                }
            }
        }

        /* Initiate Hamming vectors structure */
        MALLOC_INTERN(lw_vectors, size, pastix_int_t*);
        MALLOC_INTERN(up_vectors, size, pastix_int_t*);
        for (i=0; i<size; i++) {
            MALLOC_INTERN(lw_vectors[i], lw_vectors_size[i], pastix_int_t);
            MALLOC_INTERN(up_vectors[i], up_vectors_size[i], pastix_int_t);
            memset(lw_vectors[i], 0, lw_vectors_size[i] * sizeof(pastix_int_t));
            memset(up_vectors[i], 0, up_vectors_size[i] * sizeof(pastix_int_t));
        }
        memset(lw_vectors_size, 0, size * sizeof(pastix_int_t));
        memset(up_vectors_size, 0, size * sizeof(pastix_int_t));

        /* Fill-in vectors structure with contributing cblks */
        for(iterblok=cblk[0].brownum; iterblok<cblk[1].brownum; iterblok++)
        {
            blok = symbptr->bloktab + brow[iterblok];

            /* For upper levels in nested dissection */
            if (levels[blok->lcblknm] <= local_split_level) {
                for (i=blok->frownum; i<=blok->lrownum; i++){
                    pastix_int_t index = i - cblk->fcolnum;
                    up_vectors[index][up_vectors_size[index]] = blok->lcblknm;
                    up_vectors_size[index]++;
                }
            }
            else{
                for (i=blok->frownum; i<=blok->lrownum; i++){
                    pastix_int_t index = i - cblk->fcolnum;
                    lw_vectors[index][lw_vectors_size[index]] = blok->lcblknm;
                    lw_vectors_size[index]++;
                }
            }
        }
    }

    clockStop(timer);
    *time_compute_vectors += clockVal(timer);

    clockStart(timer);
    {
        /* Apply the pseudo-TSP algorithm to the rows in the current supernode */
        symbol_reorder_tsp(size, order, cblk - symbptr->cblktab,
                           lw_vectors, lw_vectors_size,
                           up_vectors, up_vectors_size,
                           stop_criteria);
    }
    clockStop(timer);
    *time_update_perm += clockVal(timer);

    for (i=0; i<size; i++){
        memFree_null(lw_vectors[i]);
        memFree_null(up_vectors[i]);
    }
    memFree_null(lw_vectors);
    memFree_null(up_vectors);
    memFree_null(lw_vectors_size);
    memFree_null(up_vectors_size);
}

/* For split_level parameter */
/* The chosen level to reduce computational cost: no effects if set to 0 */
/* A first comparison is computed according to upper levels */
/* If hamming distances are equal, the computation goes through lower levels */

/* For stop_criteria parameter */
/* Criteria to limit the number of comparisons when computing hamming distances */

void
symbolReordering( const SymbolMatrix *symbptr,
                  Order *order,
                  pastix_int_t split_level,
                  int stop_criteria )
{
    SymbolCblk  *cblk;
    pastix_int_t itercblk;
    pastix_int_t cblknbr = symbptr->cblknbr;

    double time_compute_vectors = 0.;
    double time_update_perm     = 0.;

    pastix_int_t i, maxdepth;
    pastix_int_t *levels, *depthweight;

    /* Create the level array to compute the depth of each cblk and the maximum depth */
    {
        maxdepth = 0;
        levels = calloc(cblknbr, sizeof(pastix_int_t));

        for (i=0; i<cblknbr; i++) {
            levels[i] = compute_cblklevel( order->treetab, levels, i );
            maxdepth = pastix_imax( maxdepth, levels[i] );
        }
    }

    /**
     * Solves the Traveler Salesman Problem on each cblk to minimize the number
     * of off-diagonal blocks per row
     */
    cblk = symbptr->cblktab;
    MALLOC_INTERN( depthweight, maxdepth, pastix_int_t );
    for (itercblk=0; itercblk<cblknbr; itercblk++, cblk++) {

        memset( depthweight, 0, maxdepth * sizeof(pastix_int_t) );

        symbol_reorder_cblk( symbptr, cblk, order,
                             levels, levels[itercblk],
                             depthweight, maxdepth,
                             split_level, stop_criteria,
                             &time_compute_vectors, &time_update_perm);
    }

    printf("Time to compute vectors  %lf s\n", time_compute_vectors);
    printf("Time to update  perm     %lf s\n", time_update_perm);

    /* Update the permutation */
    for (i=0; i<symbptr->nodenbr; i++) {
        order->permtab[ order->peritab[i] ] = i;
    }
    memFree_null(levels);
    memFree_null(depthweight);
}

void
symbolReorderingPrintComplexity( const SymbolMatrix *symbptr )
{
    SymbolCblk  *cblk;
    pastix_int_t itercblk, iterblok;
    pastix_int_t cblknbr;
    pastix_int_t nbflops;

    cblk    = symbptr->cblktab;
    cblknbr = symbptr->cblknbr;
    nbflops = 0;

    /**
     * nbcblk is the number of non zeroes intersection between indivudal rows
     * and block columns.
     */
    for(itercblk=0; itercblk<cblknbr; itercblk++, cblk++)
    {
        pastix_int_t width;
        pastix_int_t nbcblk = 0;

        for(iterblok=cblk[0].brownum; iterblok<cblk[1].brownum; iterblok++)
        {
            SymbolBlok *blok = symbptr->bloktab + symbptr->browtab[iterblok];
            assert( blok->fcblknm == itercblk );

            nbcblk += blok->lrownum - blok->frownum + 1;
        }
        width = cblk->lcolnum - cblk->fcolnum + 1;
        nbflops += nbcblk * (width-1);

        if ( itercblk == (cblknbr-1) ) {
            pastix_int_t localflops = nbcblk * (width-1);
            fprintf(stdout, " Number of operations in reordering for last supernode: %ld (%lf)\n",
                    localflops, (double)localflops / (double)(nbflops) * 100. );
        }
    }
    fprintf(stdout, " Number of operations in reordering: %ld\n", nbflops );
}
