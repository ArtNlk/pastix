/*
 * File: pastix_internal.h
 *
 * Header for function internal to pastix.c
 *
 */
#ifndef pastix_int_tERNAL_H
#define pastix_int_tERNAL_H
/*
 * Function: pastix_fake_fillin_csc
 *
 * Fill in the internal csc based on the user csc and fill in the coeftab structure
 *
 * Parameters:
 * pastix_data - PaStiX data structure.
 * pastix_comm - PaStiX MPI communicator.
 * n           - Size of the matrix.
 * colptr      - starting index of each column in row and avals.
 * row         - row of each element of the matrix.
 * avals       - value of each element of the matrix.
 * b           - Right hand side.
 * nrhs        - Number of right-hand-sides.
 * loc2glob    - global number of local columns, NULL if not distributed.
 */
#define pastix_fake_fillin_csc PASTIX_PREFIX_F(pastix_fake_fillin_csc)
int pastix_fake_fillin_csc( pastix_data_t *pastix_data,
                            MPI_Comm       pastix_comm,
                            pastix_int_t            n,
                            pastix_int_t           *colptr,
                            pastix_int_t           *row,
                            pastix_float_t         *avals,
                            pastix_float_t         *b,
                            pastix_int_t            nrhs,
                            pastix_int_t           *loc2glob);

/*
 *  Function: pastix_fillin_csc
 *
 *  Fill in the internal csc based on the user csc and fill in the coeftab structure
 *
 *  Parameters:
 *  pastix_data - PaStiX data structure.
 *  pastix_comm - PaStiX MPI communicator.
 *  n           - Size of the matrix.
 *  colptr      - starting index of each column in row and avals.
 *  row         - row of each element of the matrix.
 *  avals       - value of each element of the matrix.
 *  b           - Right hand side.
 *  nrhs        - Number of right-hand-sides.
 *  loc2glob    - global number of local columns, NULL if not distributed.
 */
#define pastix_fillin_csc PASTIX_PREFIX_F(pastix_fillin_csc)
int pastix_fillin_csc( pastix_data_t *pastix_data,
                       MPI_Comm       pastix_comm,
                       pastix_int_t            n,
                       pastix_int_t           *colptr,
                       pastix_int_t           *row,
                       pastix_float_t         *avals,
                       pastix_float_t         *b,
                       pastix_int_t            nrhs,
                       pastix_int_t           *loc2glob);


/*
 * Function: pastix_checkMatrix_int
 *
 * Check the matrix :
 * - Renumbers in Fortran numerotation (base 1) if needed (base 0)
 * - Check that the matrix contains no doubles,  with flagcor == API_YES,
 *   correct it.
 * - Can scale the matrix if compiled with -DMC64 -DSCALING (untested)
 * - Checks the symetry of the graph in non symmetric mode.
 *   With non distributed matrices, with flagcor == API_YES,
 *   correct the matrix.
 * - sort the CSC.
 *
 * Parameters:
 *   pastix_comm - PaStiX MPI communicator
 *   verb        - Level of prints (API_VERBOSE_[NOT|NO|YES])
 *   flagsym     - Indicate if the given matrix is symetric
 *                 (API_SYM_YES or API_SYM_NO)
 *   flagcor     - Indicate if we permit the function to reallocate the matrix.
 *   n           - Number of local columns.
 *   colptr      - First element of each row in *row* and *avals*.
 *   row         - Row of each element of the matrix.
 *   avals       - Value of each element of the matrix.
 *   loc2glob    - Global column number of local columns
 *                 (NULL if not distributed).
 *   dof         - Number of degrees of freedom.
 *   flagalloc   - indicate if allocation on CSC uses internal malloc.
 */
pastix_int_t pastix_checkMatrix_int(MPI_Comm pastix_comm,
                           pastix_int_t      verb,
                           pastix_int_t      flagsym,
                           pastix_int_t      flagcor,
                           pastix_int_t      n,
                           pastix_int_t    **colptr,
                           pastix_int_t    **row,
                           pastix_float_t  **avals,
                           pastix_int_t    **loc2glob,
                           pastix_int_t      dof,
                           pastix_int_t      flagalloc);

/*
 * Function: pastix_welcome_print
 *
 * Will print welcome message, options and parameters.
 *
 * Parameters:
 * pastix_data - PaStiX data structure
 * colptr      - starting index of each column in the CSC.
 * n           - number of columns.
 *
 */
void pastix_welcome_print(pastix_data_t *pastix_data,
                          pastix_int_t           *colptr,
                          pastix_int_t            ln);

/*
 * Function: pastix_task_clean
 *
 * Cleaning task
 *
 * Parameters:
 *
 */
void pastix_task_clean(pastix_data_t **pastix_data,
                       MPI_Comm        pastix_comm);

/*
 * Function: pastix_task_init
 *
 * Allocate and fill-in pastix_data
 *
 * Parameters:
 *   pastix_data - structure to build
 *   pastix_comm - PaStiX MPI communicator
 *   iparm       - integer parameters, to fill-in pastix_data
 *   dparm       - floating parameters, to fill-in pastix_data
 */
void pastix_task_init(pastix_data_t **pastix_data,
                      MPI_Comm        pastix_comm,
                      pastix_int_t            *iparm,
                      double         *dparm);

/*
 * Function: pastix_initParam
 *
 * sets default parameters for iparm and dparm
 *
 * Parameters:
 * iparm - tabular of IPARM_SIZE integer parameters.
 * dparm - tabular of DPARM_SIZE double parameters.
 */
void pastix_initParam(pastix_int_t    *iparm,
                      double *dparm);




#endif /* not pastix_int_tERNAL_H */
