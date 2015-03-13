#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <pastix.h>
#include "../matrix_drivers/drivers.h"
#include "../symbol/symbol.h"
#include "../common/common.h"
#include "../fax/fax.h"
#include "../order/order.h"
#include <csc.h>

int
pastix_task_reordering(pastix_data_t *pastix_data)
{
    Clock timer;
    clockStart(timer);

    symbolNewOrdering( pastix_data->symbmtx, pastix_data->ordemesh );

    /* !!! We have to save the new ordering !!! */
    /* int retval = orderSave( pastix_data->ordemesh, NULL ); */

    symbolExit(pastix_data->symbmtx);
    free(pastix_data->symbmtx);
    pastix_data->symbmtx = NULL;

    pastix_task_symbfact( pastix_data, NULL, NULL );

    if (pastix_data->graph != NULL)
    {
        graphExit( pastix_data->graph );
        memFree_null( pastix_data->graph );
    }

    clockStop(timer);
    printf("Total time for reordering (with extra symbolic factorization) %.3g s\n", (double)clockVal(timer));

    return PASTIX_SUCCESS;
}
