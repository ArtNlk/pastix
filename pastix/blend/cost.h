/*+********************************************************+*/
/*+                                                        +*/
/*+   NAME       : cost.h                                  +*/
/*+                                                        +*/
/*+   AUTHORS    : Pascal HENON                            +*/
/*+                                                        +*/
/*+   FUNCTION   : Part of a parallel direct block solver. +*/
/*+                Definitions of the structures that      +*/
/*+                contains cost informations              +*/
/*+                                                        +*/
/*+   DATES      : # Version 0.0  : from : 27 sep 1998     +*/
/*+                                 to     03 sep 1998     +*/
/*+                                                        +*/
/*+********************************************************+*/

/*
**  The type and structure definitions.
*/

typedef struct CostCblk_ {
  double                     compute; /* Compute cost                               */
  double                     total;   /* Cost of the treenode only (compute + send) */
  double                     subtree; /* Cost of the subtree (included total)       */
} CostCblk;

typedef struct CostBlok_ {
  double                    contrib; /*+ Cost of contrib bounded to this blok                  +*/
  pastix_int_t              linenbr; /*+ Number of no empty line above the blok (blok include) +*/
} CostBlok;

typedef struct CostMatrix_ {
    CostBlok              *     bloktab;
} CostMatrix;

/*
**  The function prototypes.
*/

#ifndef COST
#define static
#endif
pastix_int_t                         costInit            (CostMatrix *);
void                        costExit            (CostMatrix *);
pastix_int_t                         costLoad            (CostMatrix *, FILE *);
pastix_int_t                         costSave            (CostMatrix *, FILE *);

#undef static
