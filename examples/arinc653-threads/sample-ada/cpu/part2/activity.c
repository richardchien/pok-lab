/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team
 *
 * Created by julien on Tue Nov 24 21:49:41 2009
 */

#include <arinc653/types.h>
#include "subprograms.h"
#include <arinc653/time.h>
#include "deployment.h"
/*****************************************************/
/*  This file was automatically generated by Ocarina */
/*  Do NOT hand-modify this file, as your            */
/*  changes will be lost when you re-run Ocarina     */
/*****************************************************/
/*  Periodic task : thr2*/

/***************/
/* thr2_job   */
/**************/

void* thr2_job() {
    RETURN_CODE_TYPE ret;
    while (1) {
        /*  Call implementation*/
        test__hello_part2();
        PERIODIC_WAIT(&(ret));
    }
}
