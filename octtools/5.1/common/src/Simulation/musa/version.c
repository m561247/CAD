/***********************************************************************
 * Generate version string
 * Filename: version.h
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define VERSION_MUSA
#include "musa.h"

/***********************************************************************
 * VERSION
 */
char *version()
{
    static char string[256];

    (void) sprintf (string, "Version 5.1 (made %s)",__DATE__);
    return string;
}

