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
#include <gtypes.h>
#include <libc/stdio.h>
#include <libc/stdio.h>
#include <libc/string.h>

#define MYSTRING "Bonjour"

void user_send(arraytest__buffer* t) {
    char* toto = (char*)*t;
    memcpy(toto, MYSTRING, 7);
    toto[7] = '\0';

    printf("Sent value %s\n", (char*)*t);
}
