/************************************************************/
/**                                                        **/
/**   NAME       : elimin.h                                **/
/**                                                        **/
/**   AUTHORS    : Pascal HENON                            **/
/**                                                        **/
/**   FUNCTION   : Part of a parallel direct block solver. **/
/**                These lines are the data declarations   **/
/**                for the elimination tree.               **/
/**                                                        **/
/**   DATES      : # Version 0.0  : from : 22 jul 1998     **/
/**                                 to     27 jul 1998     **/
/**                                                        **/
/************************************************************/

/*
 **  The type and structure definitions.
 */

/*+ The node structure. +*/
typedef struct eTreeNode_s {
    double         total;   /* Cost of the treenode only (compute + send) */
    double         subtree; /* Cost of the subtree (includes total)       */
    pastix_int_t   sonsnbr; /* Number of sons                             */
    pastix_int_t   fathnum; /* index of the father node                   */
    pastix_int_t   fsonnum; /* index of first son                         */
} eTreeNode_t;

/*+ The elimination tree. +*/

typedef struct EliminTree_ {
    pastix_int_t   baseval;              /*+ Base value for numberings         +*/
    pastix_int_t   nodenbr;              /*+ Number of nodes                   +*/
    eTreeNode_t  * nodetab;              /*+ Array of node          [+1,based] +*/
    pastix_int_t * sonstab;              /*+ Sons index of nodes               +*/
} EliminTree;

pastix_int_t  eTreeInit      (      EliminTree *);
void          eTreeExit      (      EliminTree *);
void          eTreeGenDot    (const EliminTree *, FILE *);
void          eTreePrint     (const EliminTree *, FILE *, pastix_int_t );
pastix_int_t  eTreeLeavesNbr (const EliminTree *);
pastix_int_t  eTreeLevel     (const EliminTree *);
pastix_int_t  eTreeNodeLevel (const EliminTree *, pastix_int_t );
EliminTree   *eTreeBuild     (const SymbolMatrix *);

#define eTreeFather( __etree__, __node__ )       ((__etree__)->nodetab[(__etree__)->nodetab[(__node__)].fathnum])
#define eTreeSonI( __etree__, __node__, __i__ )  ((__etree__)->sonstab[(__etree__)->nodetab[(__node__)].fsonnum + (__i__)])
#define eTreeRoot( __etree__ )                   ((__etree__)->nodenbr - 1)
