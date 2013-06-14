/************************************************************/
/**                                                        **/
/**   NAME       : splitpart.h                             **/
/**                                                        **/
/**   AUTHORS    : Pascal HENON                            **/
/**                                                        **/
/**   FUNCTION   : Part of a parallel direct block solver. **/
/**                repartition and make processor          **/
/**                candidate groups                        **/
/**                                                        **/
/**   DATES      : # Version 0.0  : from : 22 jul 1998     **/
/**                                 to     09 sep 1998     **/
/**                                                        **/
/************************************************************/
#ifndef SPLITPART_H

#define CLUSTER 1
#define NOCLUSTER 0


#define static

#endif

void  splitPart     (SymbolMatrix *, BlendCtrl *, const Dof *);
pastix_int_t   check_candidat(SymbolMatrix *, BlendCtrl *);

void        setTreeLevel         (Cand *, const EliminTree *);
void        setTreeCostLevel     (Cand *, const EliminTree *, const CostMatrix *);
static void setSubtreeLevel      (pastix_int_t, Cand *, const EliminTree *);
static void setSubtreeCostLevel  (pastix_int_t, Cand *, const EliminTree *, const CostMatrix *);
static void setDistribType       (const pastix_int_t, SymbolMatrix *, Cand *, const pastix_int_t);
static void setSubtreeDistribType(const SymbolMatrix *, const CostMatrix *, pastix_int_t , const BlendCtrl *, pastix_int_t);

static void splitOnProcs    (SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, 
			     const Dof *, pastix_int_t, pastix_int_t);
static void splitCblk       (SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, 
			     const Dof *, pastix_int_t, pastix_int_t, pastix_int_t *);

static void printTree          (FILE*, const EliminTree *, pastix_int_t);
static void propMappTree       (SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, const Dof *);
static void propMappSubtree    (SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, const Dof *,
				pastix_int_t, pastix_int_t, pastix_int_t, pastix_int_t, double *);
static void propMappSubtreeNC  (SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, const Dof *,
				pastix_int_t, pastix_int_t, pastix_int_t, pastix_int_t, double *);
static void propMappSubtreeOn1P(SymbolMatrix *, ExtraSymbolMatrix *, ExtraCostMatrix *, BlendCtrl *, const Dof *,
				pastix_int_t, pastix_int_t, pastix_int_t, pastix_int_t);

static void propMappTreeNoSplit    (SymbolMatrix *, BlendCtrl *, const Dof *);
static void propMappSubtreeNoSplit (SymbolMatrix *, BlendCtrl *, const Dof *, pastix_int_t, pastix_int_t, pastix_int_t, double *);


static double maxProcCost     (double *, pastix_int_t);
static void   subtreeSetCand  (pastix_int_t, pastix_int_t, BlendCtrl *, double);
static double blokUpdateCost  (pastix_int_t, pastix_int_t, CostMatrix *, ExtraCostMatrix *, const SymbolMatrix *, 
			       const ExtraSymbolMatrix *, BlendCtrl *, const Dof *);

static pastix_int_t    countBlok            (pastix_int_t, SymbolMatrix *, pastix_int_t);
static pastix_int_t    setSubtreeBlokNbr    (pastix_int_t, const EliminTree *, SymbolMatrix *, ExtraSymbolMatrix *, pastix_int_t);
static void   clusterCandCorrect   (pastix_int_t, Cand *, const EliminTree *, BlendCtrl *);
static void   setClusterCand       (pastix_int_t, Cand *, const EliminTree *, pastix_int_t, pastix_int_t);

#undef static

