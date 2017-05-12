#ifndef _GET_OPTIONS_H_
#define _GET_OPTIONS_H_

/*
  File: get_options.h

  Definition of a global function to get exemple parameters.

 */

/*
  Function: str_tolower

  Rewrites *string* in lower case.

  Parameters:
    string - string to rewrite in lower case.
 */
int str_tolower(char * string);

/*
   Function: global_usage

   Print usage corresponding to all pastix exemples.

   Parameters:
     mpi_comm - MPI communicator.
     argv     - program argument

 */
void global_usage(MPI_Comm mpi_comm,
      char   **argv);

/*
  Function: get_options

  Get options from argv.

  Parameters:
    argc          - number of arguments.
    argv          - argument tabular.
    driver_type   - type of driver (output).
    filename      - Matrix filename (output).
    nbthread      - number of thread (left unchanged if not in options).
    verbose       - verbose level 1,2 or 3
    ordering      - ordering to choose.
    incomplete    - indicate if -incomp is present
    level_of_fill - Level of fill for incomplete factorization.
    amalgamation  - Amalgamation for kass.
    ooc           - Out-of-core limite (Mo or percent depending on compilation option)
    size          - Size of the matrix (generated matrix only)

 */
int get_options(int              argc,
                char           **argv,
                driver_type_t  **driver_type,
                char          ***filename,
                int             *nbmatrices,
                int             *nbthread,
                int             *verbose,
                int             *ordering,
                int             *incomplete,
                int             *level_of_fill,
                int             *amalgamation,
                int             *ooc,
                pastix_int_t    *size);

/*
  Function: d_get_idparm

  Get options from argv.

  Parameters:
    argc          - number of arguments.
    argv          - argument tabular.
    iparm         - type of driver (output, -1 if not set).
    dparm         - type of driver (output, -1 if not set).
 */
int d_get_idparm(int            argc,
                 char         **argv,
                 pastix_int_t  *iparm,
                 double        *dparm);
int s_get_idparm(int            argc,
                 char         **argv,
                 pastix_int_t  *iparm,
                 float        *dparm);

#endif
