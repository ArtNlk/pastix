/**
 *
 * @file solver_draw.c
 *
 *  PaStiX factorization routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Gregoire Pichon
 * @date 2016-12-05
 *
 **/

#include "common.h"
#include "symbol.h"
#include "solver.h"

/*+ Generic PostScript (tm) output definitions. +*/

#define SYMBOL_PSDPI      72    /*+ PostScript dots-per-inch            +*/
#define SYMBOL_PSPICTSIZE 6.6   /*+ PostScript picture size (in inches) +*/

/**
 *******************************************************************************
 *
 * @ingroup pastix_sopalin
 *
 * solverDraw - writes a PostScript picture of the low-rank symbolic block
 * matrix
 *
 *******************************************************************************
 *
 * @param[in] solvptr
 *          The solver structure of PaStiX
 *
 * @param[in, out] stream
 *          File to write the PostScript
 *
 * @param[in] verbose
 *          Level of verbosity. If verbose > 4, the number of updates is printed
 *          on each low-rank block.
 *
 *******************************************************************************
 *
 * @return
 *          @arg 0    : SUCCESS
 *          @arg != 0 : FAILURE
 *
 *******************************************************************************/
int
solverDraw ( const SolverMatrix * const  solvptr,
             FILE * const                stream,
             int                         verbose )
{
    pastix_int_t cblknum;                    /* Number of current column block */
    time_t       picttime;                   /* Creation time                  */
    double       pictsize;                   /* Number of distinct coordinates */
    int          o;

    time (&picttime);                               /* Get current time */
    pictsize = (double) (solvptr->nodenbr + 1);     /* Get matrix size  */

    fprintf (stream, "%%!PS-Adobe-2.0 EPSF-2.0\n"); /* Write header */
    fprintf (stream, "%%%%Title: symbolmatrix (%ld,%ld,%ld)\n",
             (long) solvptr->cblknbr, (long) solvptr->bloknbr, (long)solvptr->nodenbr);
    fprintf (stream, "%%%%Creator: symbolDraw (LaBRI, Universite Bordeaux I)\n");
    fprintf (stream, "%%%%CreationDate: %s", ctime (&picttime));
    fprintf (stream, "%%%%BoundingBox: 0 0 %ld %ld\n",
             (long) (SYMBOL_PSPICTSIZE * SYMBOL_PSDPI),
             (long) (SYMBOL_PSPICTSIZE * SYMBOL_PSDPI));
    fprintf (stream, "%%%%Pages: 0\n");
    fprintf (stream, "%%%%EndComments\n");          /* Write shortcuts */
    fprintf (stream, "/c { 4 2 roll pop pop newpath 2 copy 2 copy moveto dup lineto dup lineto closepath fill } bind def\n");
    fprintf (stream, "/d { 4 2 roll pop pop newpath 2 copy 2 copy moveto dup lineto dup lineto closepath } bind def\n");
    fprintf (stream, "/b { 4 copy 2 index exch moveto lineto dup 3 index lineto exch lineto closepath fill pop } bind def\n");
    fprintf (stream, "/a { 4 copy 2 index exch moveto lineto dup 3 index lineto exch lineto closepath pop } bind def\n");
    fprintf (stream, "/r { setrgbcolor } bind def\n");
    fprintf (stream, "/g { setgray } bind def\n");

    fprintf (stream, "0 setlinecap\n");             /* Use miter caps       */
    fprintf (stream, "%f dup scale\n",              /* Print scaling factor */
             (double) SYMBOL_PSDPI * SYMBOL_PSPICTSIZE / pictsize);
    fprintf (stream, "/Times-Roman 70 selectfont\n"); /* activate text in eps file */
    fprintf (stream, "[ 1 0 0 -1 0 %d ] concat\n",  /* Reverse Y coordinate */
             (int) (solvptr->nodenbr + 1));

    fprintf (stream, "0 0\n");                      /* Output fake column block */
    for (cblknum = 0; cblknum < solvptr->cblknbr; cblknum ++) {
        float        coloval[3];               /* Color of diagonal block and previous color */
        SolverCblk  *cblk   = &solvptr->cblktab[cblknum];
        pastix_int_t ncols  = cblk_colnbr( cblk );
        SolverBlok  *blok   = cblk[0].fblokptr+1;
        SolverBlok  *lblok  = cblk[1].fblokptr;


        coloval[0] = 0.5;
        coloval[1] = 0.5;
        coloval[2] = 0.5;
        if ((coloval[0] == coloval[1]) &&
            (coloval[1] == coloval[2]))
            fprintf (stream, "%.2g g ",
                     (float) coloval[0]);
        else
            fprintf (stream, "%.2g %.2g %.2g r \n",
                     (float) coloval[0], (float) coloval[1], (float) coloval[2]);

        fprintf (stream, "%ld\t%ld\tc\n",             /* Begin new column block */
                 (long) (cblk->fcolnum - solvptr->baseval),
                 (long) (cblk->lcolnum - solvptr->baseval + 1));


        for (; blok<lblok; blok++)
        {
            float colbval[3]; /* Color of off-diagonal block */
            coloval[0] = colbval[0]; /* Save new color data */
            coloval[1] = colbval[1];
            coloval[2] = colbval[2];


            if ( cblk->cblktype & CBLK_DENSE ) {
                fprintf (stream, "%.2g %.2g %.2g r \n",
                         0.5, 0.5, 0.5);
            }
            else{
                pastix_int_t nrows       = blok_rownbr( blok );
                pastix_int_t conso_dense = 2*nrows*ncols;
                pastix_int_t conso_LR    = 0;
                double       gain;

                if (blok->LRblock[0].rk != -1){
                    conso_LR += (((nrows+ncols) * blok->LRblock[0].rk));
                }
                else{
                    conso_LR += nrows*ncols;
                }
                if (blok->LRblock[1].rk != -1){
                    conso_LR += (((nrows+ncols) * blok->LRblock[1].rk));
                }
                else{
                    conso_LR += nrows*ncols;
                }

                gain = 1.0 * conso_dense / conso_LR;

                /* There is no compression */
                if (gain == 1.){
                    fprintf(stream, "%.2g %.2g %.2g r \n",
                            0., 0., 0.);
                }
                /* Small amount of compression: red */
                else if (gain < 5.){
                    fprintf(stream, "%.2g %.2g %.2g r \n",
                            gain / 5., 0., 0.);
                }
                /* Huge amount of compression */
                else{
                    float color = 0.5 + (gain-5) / 10.;
                    if (color > 1)
                        color = 1.;
                    fprintf(stream, "%.2g %.2g %.2g r \n",
                            0., color, 0.);
                }
            }

            fprintf (stream, "%ld\t%ld\tb\n",         /* Write block in column block */
                     (long) (blok->frownum - solvptr->baseval),
                     (long) (blok->lrownum - solvptr->baseval + 1));
        }
    }

    /* Plot numbers */
    if (verbose > 4){
        int nb_bloks = 0;
        int nb_cblks = 0;
        FILE *fd1 = fopen( "contribblok.txt", "r" );
        FILE *fd2 = fopen( "contribcblk.txt", "r" );
        FILE *fd3 = fopen( "stats.txt", "w" );
        int original_cblk = 1;
        double color      = 0.2;

        fprintf(fd3, "%ld\n", solvptr->bloknbr-solvptr->cblknbr);

        fprintf (stream, "0 0\n");                      /* Output fake column block */
        for (cblknum = 0; cblknum < solvptr->cblknbr; cblknum ++) {
            int unused, nb_contrib;
            SolverCblk *cblk   = &solvptr->cblktab[cblknum];
            pastix_int_t ncols = cblk_colnbr( cblk );
            SolverBlok *blok   = cblk[0].fblokptr+1;
            SolverBlok *lblok  = cblk[1].fblokptr;
            fscanf(fd2, "%d %d %d\n", &unused, &nb_contrib, &original_cblk);

            fprintf (stream, "%.2g g %ld\t%ld\tc\n",             /* Begin new column block */
                     color,
                     (long) (cblk->fcolnum - solvptr->baseval),
                     (long) (cblk->lcolnum - solvptr->baseval + 1));
            if ( ! (cblk->cblktype & CBLK_DENSE) ) {
                fprintf (stream, "%ld\t%ld\t4 copy 3 index exch moveto [ 1 0 0 -1 0 0 ] concat 0.0 0.0 0.0 setrgbcolor (%d) show [ 1 0 0 -1 0 0 ] concat pop\n",
                         (long) (cblk->fcolnum - solvptr->baseval),
                         (long) (cblk->lcolnum - solvptr->baseval + 1),
                         nb_contrib);
            }


            for (; blok<lblok; blok++)
            {
                int unused, nb_contrib;
                double gain = 0;

                fscanf(fd1, "%d %d\n", &unused, &nb_contrib);
                fprintf (stream, "%ld\t%ld\ta\n",         /* Write block in column block */
                         (long) (blok->frownum - solvptr->baseval),
                         (long) (blok->lrownum - solvptr->baseval + 1));
                if ( ! (cblk->cblktype & CBLK_DENSE) ) {
                    pastix_int_t nrows       = blok_rownbr( blok );
                    pastix_int_t conso_dense = 2*nrows*ncols;
                    pastix_int_t conso_LR    = 0;
                    fprintf (stream, "%ld\t%ld\t4 copy 3 index exch moveto [ 1 0 0 -1 0 0 ] concat 1.0 1.0 1.0 setrgbcolor (%d) show [ 1 0 0 -1 0 0 ] concat pop\n",
                             (long) (blok->frownum - solvptr->baseval),
                             (long) (blok->lrownum - solvptr->baseval + 1),
                             nb_contrib);

                    if (blok->LRblock[0].rk != -1){
                        conso_LR += (((nrows+ncols) * blok->LRblock[0].rk));
                    }
                    else{
                        conso_LR += nrows*ncols;
                    }
                    if (solvptr->factoLU){
                        if (blok->LRblock[1].rk != -1){
                            conso_LR += (((nrows+ncols) * blok->LRblock[1].rk));
                        }
                        else{
                            conso_LR += nrows*ncols;
                        }
                    }

                    gain = 1.0 * conso_dense / conso_LR;
                }
                nb_bloks++;

                fprintf(fd3, "%d\n%f\n", nb_contrib, gain);
            }

            if (original_cblk == 0){
                if (color < 0.3)
                    color = 0.8;
                else
                    color = 0.2;
            }
            nb_cblks++;
        }
        fclose(fd1);
        fclose(fd2);
        fclose(fd3);
    }

    fprintf (stream, "pop pop\n");        /* Purge last column block indices */
    o = fprintf (stream, "showpage\n");   /* Restore context                 */


    return ((o != EOF) ? 0 : 1);
}
