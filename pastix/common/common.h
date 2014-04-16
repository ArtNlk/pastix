/*
 *  File: common.h
 *
 *  Part of a parallel direct block solver.
 *
 *  These lines are the common data
 *  declarations for all modules.
 *
 *  Authors:
 *    Mathieu  Faverge    - faverge@labri.fr
 *    David    GOUDIN     - .
 *    Pascal   HENON      - henon@labri.fr
 *    Xavier   LACOSTE    - lacoste@labri.fr
 *    Francois PELLEGRINI - .
 *    Pierre   RAMET      - ramet@labri.fr
 *
 *  Dates:
 *    Version 0.0 - from 08 may 1998
 *                  to   08 jan 2001
 *    Version 1.0 - from 06 jun 2002
 *                  to   06 jun 2002
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include "pastix.h"
#include <unistd.h>
#include <assert.h>
#include "debug.h"
#include "out.h"
#include "memory.h"
#include "integer.h"
#include "timing.h"
#include "trace.h"
#include "pastixdata.h"
#include "redefine_functions.h"

/********************************************************************
 * Errors functions
 */
void errorProg  (const char * const);
void errorPrint (const char * const, ...);
void errorPrintW(const char * const, ...);

/*
  Macro: EXIT

  Set IPARM_ERROR_NUMBER  to module+error, dumps parameters and exit.

  Parameters:
    module - Module where the error occurs.
    error  - Value to set IPARM_ERROR_NUMBER to.
*/
#ifdef EXIT_ON_SIGSEGV
#define EXIT(module,error) { *(int *)0 = error; }
#else
#define EXIT(module,error) { abort(); }
#endif

/********************************************************************
 * CBLAS value address
 */
#ifndef CBLAS_SADDR
#define CBLAS_SADDR( a_ ) (&(a_))
#endif

/********************************************************************
 * Files handling macros
 */
/*
  Macro: PASTIX_FOPEN

  Open a file and handle errors.

  Parameters:
  FILE      - Stream (FILE*) to link to the file.
  filenamne - String containing the path to the file.
  mode      - String containing the opening mode.

*/
#define PASTIX_FOPEN(FILE, filenamne, mode)                                \
  {                                                                 \
    FILE = NULL;                                                    \
    if (NULL == (FILE = fopen(filenamne, mode)))                    \
      {                                                             \
        errorPrint("%s:%d Couldn't open file : %s with mode %s\n",  \
                   __FILE__, __LINE__, filenamne, mode);            \
        EXIT(MOD_UNKNOWN,FILE_ERR);                                 \
      }                                                             \
  }
/*
  Macro: PASTIX_FREAD

  Calls fread function and test his return value

  Parameters:
  buff   - Memory area where to copy read data.
  size   - Size of an element to read.
  count  - Number of elements to read
  stream - Stream to read from
*/
#define PASTIX_FREAD(buff, size, count, stream)        \
  {                                             \
    if ( 0 == fread(buff, size, count, stream)) \
      {                                         \
        errorPrint("%s:%d fread error\n",       \
                   __FILE__, __LINE__);         \
        EXIT(MOD_UNKNOWN,FILE_ERR);             \
      }                                         \
  }

/*
 * Get environment variable
 */
#if defined PASTIX_OS_WINDOWS

static inline char * pastix_getenv( char *var ) {
    char *str;
    int len = 512;
    int rc;
    str = (char*)malloc(len * sizeof(char));
    rc = GetEnvironmentVariable(var, str, len);
    if (rc == 0) {
        free(str);
        str = NULL;
    }
    return str;
}

static inline void pastix_cleanenv( char *str ) {
    if (str != NULL) free(str);
}

#else /* Other OS systems */

static inline char * pastix_getenv( char *var ) {
    return getenv( var );
}

static inline void pastix_cleanenv( char *str ) {
    (void)str;
}

#endif


static inline int
pastix_env_is_on(char * str) {
    char * val;
    if ( (val = pastix_getenv(str)) &&
         !strcmp(val, "1"))
        return API_YES;
    return API_NO;
}
static inline
int pastix_starpu_with_fanin() {
    return pastix_env_is_on("PASTIX_STARPU_FANIN");
}

static inline
int pastix_starpu_with_nested_task() {
    return pastix_env_is_on("PASTIX_STARPU_NESTED_TASK");
}

static inline
int pastix_starpu_with_separate_trsm() {
    return pastix_env_is_on("PASTIX_STARPU_SEPARATE_TRSM");
}

#endif /* _COMMON_H_ */

