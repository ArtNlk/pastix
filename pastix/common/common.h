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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "debug.h"
#include "out.h"
#include "memory.h"
#include "integer.h"
#include "timing.h"
#include "../order/order.h"
#include "pastixstr.h"

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

#endif /* _PASTIX_H_ */
