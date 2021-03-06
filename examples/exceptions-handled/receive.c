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
 * Created by julien on Thu Jan 15 23:34:13 2009
 */

#include <types.h>
#include <libc/stdio.h>

void user_receive(int t) {
    int a;
    printf("P2T1: Receive value %d\n", t);
    if (t == 2) {
        a = t / 0;
    } else {
        a = t;
    }
    printf("P1T2: Value computed %d\n", a);
}
