/**
 *
 * @file get_options.c
 *
 * @copyright 2006-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Esragul Korkmaz
 * @author Gregoire Pichon
 * @author Tony Delarue
 * @date 2021-06-17
 *
 */
#include "common.h"
#include <unistd.h>
#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif  /* defined(HAVE_GETOPT_H) */
#include <string.h>

/**
 * @brief Print default usage for PaStiX binaries
 */
static inline void
pastix_usage(void)
{
    fprintf(stderr,
            "Matrix input (mandatory):\n"
            " -0 --rsa          : RSA/Matrix Market Fortran driver (only real)\n"
            " -1 --hb           : Harwell Boeing C driver\n"
            " -2 --ijv          : IJV coordinate C driver\n"
            " -3 --mm           : Matrix Market C driver\n"
            " -4 --spm          : SPM Matrix driver\n"
            " -9 --lap          : Generate a Laplacian (5-points stencil)\n"
            " -x --xlap         : Generate an extended Laplacian (9-points stencil)\n"
            " -G --graph        : SCOTCH Graph file\n"
            "\n"
            "Architecture arguments:\n"
            " -t --threads      : Number of threads per node (default: -1 to use the number of cores available)\n"
            " -g --gpus         : Number of gpus per node (default: 0)\n"
            " -s --sched        : Set the default scheduler (default: 1)\n"
            "                     0: Sequential, 1: Static, 2: PaRSEC, 3: StarPU\n"
            "\n"
            "Optional arguments:\n"
            " -f --fact                     : Choose factorization method (default: LU)\n"
            "                                 0: Cholesky, 1: LDL^[th], 2: LU, 3:LL^t, 4:LDL^t\n"
            "                                 3 and 4 are for complex matrices only\n"
            " -c --check                    : Choose the level of check to perform (default: 1)\n"
            "                                 0: None, 1: Backward error, 2: Backward and forward errors\n"
            " -o --ord                      : Choose between ordering libraries (default: scotch)\n"
            "                                 scotch, ptscotch, metis, parmetis\n"
            " -i --iparm <IPARM_ID> <value> : set any given integer parameter\n"
            " -d --dparm <DPARM_ID> <value> : set any given floating parameter\n"
            "\n"
            " -v --verbose[=lvl] : extra verbose output\n"
            " -h --help          : this message\n"
            "\n"
            );
}

/**
 * @brief Define the options and their requirement used by PaStiX
 */
#define GETOPT_STRING "0:1:2:3:4:9:x:G:t:g:s:o:f:c:i:d:v::h"

#if defined(HAVE_GETOPT_LONG)
/**
 * @brief Define the long options when getopt_long is available
 */
static struct option long_options[] =
{
    {"rsa",         required_argument,  0, '0'},
    {"hb",          required_argument,  0, '1'},
    {"ijv",         required_argument,  0, '2'},
    {"mm",          required_argument,  0, '3'},
    {"spm",         required_argument,  0, '4'},
    {"lap",         required_argument,  0, '9'},
    {"xlap",        required_argument,  0, 'x'},
    {"graph",       required_argument,  0, 'G'},

    {"threads",     required_argument,  0, 't'},
    {"gpus",        required_argument,  0, 'g'},
    {"sched",       required_argument,  0, 's'},

    {"ord",         required_argument,  0, 'o'},
    {"fact",        required_argument,  0, 'f'},
    {"check",       required_argument,  0, 'c'},
    {"iparm",       required_argument,  0, 'i'},
    {"dparm",       required_argument,  0, 'd'},

    {"verbose",     optional_argument,  0, 'v'},
    {"help",        no_argument,        0, 'h'},
    {0, 0, 0, 0}
};
#endif  /* defined(HAVE_GETOPT_LONG) */

/**
 *******************************************************************************
 *
 * @ingroup pastix_examples
 *
 * @brief PaStiX helper function to read command line options in examples.
 *
 * This function takes the command line arguments, and read the given parameters
 * (integers and doubles), as well as the matrix filename and the driver to read
 * it.
 *
 *******************************************************************************
 *
 * @param[in] argc
 *          The number of input parameters
 *
 * @param[in] argv
 *          The NULL terminated list of parameters
 *
 * @param[inout] iparam
 *          The integer array of parameters.
 *          On entry, must be initialized to the default value with pastixInitParam(),
 *          On exit, is updated with any option that matches the pastix parameters.
 *
 * @param[inout] dparam
 *          The double array of parameters.
 *          On entry, must be initialized to the default value with pastixInitParam(),
 *          On exit, is updated with any option that matches the pastix parameters.
 *
 * @param[inout] check
 *          On exit, the value is updated by the value of the -c option.
 *
 * @param[inout] driver
 *          On exit, contains the driver type give as option. -1, if no driver
 *          is specified.
 *
 * @param[out] filename
 *          The allocated string of the filename given with the driver.
 *
 *******************************************************************************/
void
pastixGetOptions( int argc, char **argv,
                  pastix_int_t *iparam, double *dparam,
                  int *check, spm_driver_t *driver, char **filename )
{
    int c;
    (void)dparam;

    if (argc == 1) {
        pastix_usage(); exit(0);
    }

    *driver = -1;
    do
    {
#if defined(HAVE_GETOPT_LONG)
        c = getopt_long( argc, argv, GETOPT_STRING,
                         long_options, NULL );
#else
        c = getopt( argc, argv, GETOPT_STRING );
#endif  /* defined(HAVE_GETOPT_LONG) */

        switch(c)
        {
        case '0':
            fprintf(stderr, "RSA driver is no longer supported and is replaced by the HB driver\n");
            pastix_attr_fallthrough;

        case '1':
            *driver = SpmDriverHB;
            *filename = strdup( optarg );
            break;

        case '2':
            *driver = SpmDriverIJV;
            *filename = strdup( optarg );
            break;

        case '3':
            *driver = SpmDriverMM;
            *filename = strdup( optarg );
            break;

        case '4':
            *driver = SpmDriverSPM;
            *filename = strdup( optarg );
            break;

        case '9':
            *driver = SpmDriverLaplacian;
            *filename = strdup( optarg );
            break;

        case 'x':
            *driver = SpmDriverXLaplacian;
            *filename = strdup( optarg );
            break;

        case 'G':
            *driver = SpmDriverGraph;
            *filename = strdup( optarg );
            break;

        case 't': iparam[IPARM_THREAD_NBR] = atoi(optarg); break;
        case 'g': iparam[IPARM_GPU_NBR] = atoi(optarg); break;

        case 'o':
            if (strncasecmp(optarg, "scotch", 6) == 0)
            {
                iparam[IPARM_ORDERING] = PastixOrderScotch;
            }
            else if (strncasecmp(optarg, "metis", 5) == 0)
            {
                iparam[IPARM_ORDERING] = PastixOrderMetis;
            }
            else if (strncasecmp(optarg, "ptscotch", 8) == 0)
            {
                iparam[IPARM_ORDERING] = PastixOrderPtScotch;
            }
            else if (strncasecmp(optarg, "parmetis", 8) == 0)
            {
                iparam[IPARM_ORDERING] = PastixOrderParMetis;
            }
            else if (strncasecmp(optarg, "personal", 8) == 0)
            {
                iparam[IPARM_ORDERING] = PastixOrderPersonal;
            }
            else {
                fprintf(stderr, "\nInvalid value for ordering option: %s\n\n", optarg);
                goto unknown_option;
            }
            break;

        case 'f': {
            int factotype = atoi( optarg );
            if ( (factotype >= 0) && (factotype <= 4)){
                iparam[IPARM_FACTORIZATION] = factotype;
            }
            else {
                fprintf(stderr, "\nInvalid value for factorization option: %s\n\n", optarg);
                goto unknown_option;
            }
        }
            break;

        case 'c': {
            int checkvalue = atoi( optarg );
            if ( (checkvalue >= 0) && (checkvalue < 6) ) {
                if ( check != NULL ) {
                    *check = checkvalue;
                }
            }
            else {
                fprintf(stderr, "\nInvalid value for check option: %s\n\n", optarg);
                goto unknown_option;
            }
        }
            break;

        case 's': {
            int schedtype = atoi( optarg );
            if ( (schedtype >= 0) && (schedtype <= 4) ){
                iparam[IPARM_SCHEDULER] = schedtype;
            }
            else {
                fprintf(stderr, "\nInvalid value for scheduler option: %s\n\n", optarg);
                goto unknown_option;
            }
        }
            break;

        case 'i':
        {
            pastix_iparm_t iparm_idx;
            int iparm_val;

            /* Get iparm index */
            iparm_idx = parse_iparm( optarg );
            if ( iparm_idx == (pastix_iparm_t)-1 ) {
                fprintf(stderr, "\n%s is not a correct iparm parameter\n\n", optarg );
                goto unknown_option;
            }

            /* Get iparm value */
            iparm_val = parse_enums( argv[optind] );
            if ( iparm_val == -1 ){
                fprintf(stderr, "\n%s is not a correct value for the iparm parameters\n\n", argv[optind] );
                goto unknown_option;
            }
            iparam[iparm_idx] = iparm_val;
        }
        break;

        case 'd':
        {
            pastix_dparm_t dparm_idx;
            double dparm_val;

            /* Get iparm index */
            dparm_idx = parse_dparm( optarg );
            if ( dparm_idx == (pastix_dparm_t)-1 ) {
                fprintf(stderr, "\n%s is not a correct dparm parameter\n\n", optarg );
                goto unknown_option;
            }

            /* Get iparm value */
            dparm_val = atof( argv[optind] );
            dparam[dparm_idx] = dparm_val;
        }
        break;

        case 'v':
            if(optarg)  iparam[IPARM_VERBOSE] = atoi(optarg);
            else        iparam[IPARM_VERBOSE] = 2;
            break;

        case 'h':
            pastix_usage(); exit(EXIT_FAILURE);

        case ':':
            fprintf(stderr, "\nOption %c is missing an argument\n\n", c );
            goto unknown_option;

        case '?': /* getopt_long already printed an error message. */
            pastix_usage(); exit(EXIT_FAILURE);
        default:
            break;
        }
    } while(-1 != c);

    return;

  unknown_option:
    pastix_usage(); exit(EXIT_FAILURE);
}
