/*
 * initCrystal.c
 *
 * Initialize crystal.
 */

#include "crystal.h"
#include "hash.h"

/* crystal things							*/
extern HashTable NodeTable, FlowTable;

/*
 * initCrystal 
 *
 * Initializes crystal.
 */
initCrystal()
{
    /* initialize crystal						*/
    ModelInit();
    HashInit (&NodeTable, 1000);
    HashInit (&FlowTable, 10);
}
