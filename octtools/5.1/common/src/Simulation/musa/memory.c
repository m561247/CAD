
/***********************************************************************
 * Memory module
 * Filename: memory.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Desgin: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define MEMORY_MUSA
#include "musa.h"

/* Terminal types detected */

/***************************************************************************
 * MEMORY_READ
 * try to see if we can read a latch from Oct
 * Return 0 if this is not a MEMORY element
 */
int16 memory_read(ifacet, inst, celltype)
	octObject *ifacet, *inst;
	char *celltype;
{
	octObject prop;

	if (strcmp(celltype, "MEMORY") != 0) {
		return(FALSE);
	} /* if... */

	if (ohGetByPropName(ifacet, &prop, "SYNCHMODEL") == OCT_OK) {
		if (latch_read(ifacet, inst, celltype) == 0 && 
			tristate_read(ifacet, inst, celltype) == 0) {
			FatalError("Unable to undestand the device for SYNCHMODEL");
		} /* if... */
	} else if (ohGetByPropName(ifacet,&prop,"MEMORYMODEL") == OCT_OK) {
		(void) ram_read(ifacet, inst, celltype);
	} else {
		(void) sprintf(errmsg,"%s:%s is of type MEMORY but lacks both SYNCHMODEL and MEMORYMODEL props",ifacet->contents.facet.cell,ifacet->contents.facet.view);
		FatalError(errmsg);
	} /* if... */
	return(TRUE);
} /* memory_read... */


