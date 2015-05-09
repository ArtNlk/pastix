/**
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
/*
 * File: z_raff_functions.h
 *
 * Functions computing operations for reffinement methods
 *
 */

#ifndef Z_RAFF_FUNCTIONS_H
#define Z_RAFF_FUNCTIONS_H

typedef pastix_int_t RAFF_INT;
typedef pastix_complex64_t RAFF_FLOAT;

#define MONO_BEGIN(arg) if(solveur.me(arg)==0){
#define MONO_END(arg)   }

#define SYNC_COEF(var) do {                      \
    MONOTHREAD_BEGIN;                            \
    sopalin_data->common_flt[0] = var;           \
    MONOTHREAD_END;                              \
    SYNCHRO_THREAD;                              \
    var = sopalin_data->common_flt[0];           \
    /* To be sure noone uses common_flt[0] */    \
    SYNCHRO_THREAD;                              \
  } while(0)
#define SYNC_REAL(var) do {                      \
    MONOTHREAD_BEGIN;                            \
    sopalin_data->common_dbl[0] = var;           \
    MONOTHREAD_END;                              \
    SYNCHRO_THREAD;                              \
    var = sopalin_data->common_dbl[0];           \
    /* To be sure noone uses common_dbl[0] */    \
    SYNCHRO_THREAD;                              \
  } while(0)
#ifdef SMP_RAFF
#  define MULTITHREAD_BEGIN
#  define MULTITHREAD_END(sync)
#  define NOSMP_SYNC_COEF(var) do {} while(0)
#  define NOSMP_SYNC_REAL(var) do {} while(0)
#else /* SMP_RAFF */
#  define MULTITHREAD_BEGIN if (me == 0) {
#  define MULTITHREAD_END(sync) } if (sync) {SYNCHRO_THREAD;}
#  define NOSMP_SYNC_COEF(var) SYNC_COEF(var)
#  define NOSMP_SYNC_REAL(var) SYNC_REAL(var)
#endif /* SMP_RAFF */

#define SYNCHRO(arg)                                                    \
  do {                                                                  \
    Sopalin_Data_t * sopalin_data;                                      \
    sopalin_data = (Sopalin_Data_t*)((sopthread_data_t *)arg)->data;    \
    SolverMatrix     *datacode     = sopalin_data->datacode;            \
    SYNCHRO_THREAD;                                                     \
  } while(0)

/*** ALLOCATIONS ET SYNCHRONISATIONS ***/

/* Synchronise le vecteur x dans la nb-ieme variable de la structure */
pastix_complex64_t *z_Pastix_Synchro_Vect(void *, void *, int);

/* Alloue un vecteur de taille size octets */
void *z_Pastix_Malloc(void *, size_t );

/* Libere un vecteur */
void z_Pastix_Free(void *, void *);


/*** GESTION DE L'INTERFACE ***/

/* Affichage à chaque itération et communication de certaines informations à la structure */
void z_Pastix_Verbose(void *, double, double, double, pastix_int_t);

/* Affichage final */
void z_Pastix_End(void*, pastix_complex64_t, pastix_int_t, double, pastix_complex64_t *);

/* Vecteur solution X */
void Pastix_X(void *, pastix_complex64_t *);

/* Taille d'un vecteur */
pastix_int_t z_Pastix_n(void *);

/* Nombre de second membres */
pastix_int_t z_Pastix_m(void *);

/* Second membre */
void z_Pastix_B(void *, pastix_complex64_t *);

/* Epsilon */
pastix_complex64_t z_Pastix_Eps(void *);

/* Itermax */
pastix_int_t z_Pastix_Itermax(void *);


/* Itermax */
pastix_int_t z_Pastix_Krylov_Space(void *);

/*** OPERATIONS DE BASE ***/
/* Multiplication pour plusieurs second membres */
void z_Pastix_Mult(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

/* Division pour plusieurs second membres */
void z_Pastix_Div(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

/* Calcul de la norme de frobenius */
pastix_complex64_t z_Pastix_Norm2(void *, pastix_complex64_t *);

/* Copie d'un vecteur */
void z_Pastix_Copy(void *, pastix_complex64_t *, pastix_complex64_t *, int);

/* Application du préconditionneur */
void z_Pastix_Precond(void *, pastix_complex64_t *, pastix_complex64_t *, int);

/* Calcul de alpha * x */
void z_Pastix_Scal(void *, pastix_complex64_t, pastix_complex64_t *, int);

/* Calcul du produit scalaire */
void z_Pastix_Dotc(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

void z_Pastix_Dotc_Gmres(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

/* Produit matrice vecteur */
void z_Pastix_Ax(void *, pastix_complex64_t *, pastix_complex64_t *);


/*** A MODIFIER! ***/
void z_Pastix_bMAx(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *);

void z_Pastix_BYPX(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

void z_Pastix_AXPY(void *, double, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

pastix_int_t z_Pastix_me(void *);

struct z_solver
{
  /*** ALLOCATIONS ET SYNCHRONISATIONS ***/
  pastix_complex64_t* (* Synchro)(void *, void *, int);
  void* (* Malloc)(void*, size_t);
  void (* Free)(void*, void*);

  /*** GESTION DE L'INTERFACE ***/
  void (* Verbose)(void *, double, double, double, pastix_int_t);
  void (* End)(void* , pastix_complex64_t, pastix_int_t, double, pastix_complex64_t*);
  void (* X)(void *, pastix_complex64_t*);
  pastix_int_t (* N)(void *);
  void (* B)(void *, pastix_complex64_t*);
  pastix_complex64_t (* Eps)(void *);
  pastix_int_t (* Itermax)(void *);
  pastix_int_t (* Krylov_Space)(void *);
  pastix_int_t (* me)(void *);


  /*** OPERATIONS DE BASE ***/
  void (* Mult)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);
  void (* Div)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);
  void (* Dotc_Gmres)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);

  pastix_complex64_t (* Norm)(void* , pastix_complex64_t *);
  void (* Copy)(void *, pastix_complex64_t *, pastix_complex64_t *, int);
  void (* Precond)(void *, pastix_complex64_t *, pastix_complex64_t *, int);

  void (* Scal)(void *, pastix_complex64_t, pastix_complex64_t *, int);
  void (* Dotc)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);
  void (* Ax)(void *, pastix_complex64_t *, pastix_complex64_t *);

  void (* bMAx)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *);
  void (* BYPX)(void *, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);
  void (* AXPY)(void *, double, pastix_complex64_t *, pastix_complex64_t *, pastix_complex64_t *, int);
};

void z_Pastix_Solveur(struct z_solver *);

/*
 ** Section: Function creating threads
 */
/*
 Function: method)

 Launch sopaparam->nbthrdcomm threads which will compute
 <method_smp)>.

 Parameters:
 datacode  - PaStiX <SolverMatrix> structure.
 sopaparam - <SopalinParam> parameters structure.
 */
void z_raff_thread(SolverMatrix *, SopalinParam *, void*(*)(void *));

#endif
/* Raffinement du second membre */

void* s_gmres_smp        (void *);
void* d_gmres_smp        (void *);
void* c_gmres_smp        (void *);
void* z_gmres_smp        (void *);
void s_gmres_thread(SolverMatrix *, SopalinParam *);
void d_gmres_thread(SolverMatrix *, SopalinParam *);
void c_gmres_thread(SolverMatrix *, SopalinParam *);
void z_gmres_thread(SolverMatrix *, SopalinParam *);
void* s_grad_smp         (void *);
void* d_grad_smp         (void *);
void* c_grad_smp         (void *);
void* z_grad_smp         (void *);
void s_grad_thread (SolverMatrix *, SopalinParam *);
void d_grad_thread (SolverMatrix *, SopalinParam *);
void c_grad_thread (SolverMatrix *, SopalinParam *);
void z_grad_thread (SolverMatrix *, SopalinParam *);
void* s_pivotstatique_smp(void *);
void* d_pivotstatique_smp(void *);
void* c_pivotstatique_smp(void *);
void* z_pivotstatique_smp(void *);
void s_pivot_thread(SolverMatrix *, SopalinParam *);
void d_pivot_thread(SolverMatrix *, SopalinParam *);
void c_pivot_thread(SolverMatrix *, SopalinParam *);
void z_pivot_thread(SolverMatrix *, SopalinParam *);
void* s_bicgstab_smp(void *);
void* d_bicgstab_smp(void *);
void* c_bicgstab_smp(void *);
void* z_bicgstab_smp(void *);
void s_bicgstab_thread(SolverMatrix *, SopalinParam *);
void d_bicgstab_thread(SolverMatrix *, SopalinParam *);
void c_bicgstab_thread(SolverMatrix *, SopalinParam *);
void z_bicgstab_thread(SolverMatrix *, SopalinParam *);
