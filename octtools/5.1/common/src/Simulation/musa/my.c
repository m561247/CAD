/***********************************************************************
 * Misc routines.
 * Filename: my.c
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

#define MY_MUSA
#include "musa.h"

/**********************************************************************
 * MYCOUNTCONTENTS
 */
octStatus myCountContents(object, mask, n)
	octObject *object;
	octObjectMask mask;
	int *n;
{
	octStatus status;
	octObject obj;
	octGenerator gen;

	*n = 0;
	status = octInitGenContents(object, mask, &gen);
	if (status != OCT_OK) return status;
		while ((status == octGenerate(&gen, &obj)) == OCT_OK) {
		(*n)++;
	}
	if (status != OCT_OK) return status;
	return OCT_OK;
} /* myCountContents... */

/************************************************************************
 * MYUNIQNAMES
 */
octStatus myUniqNames(container, mask)
	octObject *container;
	octObjectMask mask;
{
	octObject obj;
	octGenerator gen;
	char *name, dummyname[1024];
	st_table *name_table;
	int count;

	name_table = st_init_table(strcmp, st_strhash);

	OH_ASSERT(octInitGenContents(container, mask, &gen));
	while (octGenerate(&gen, &obj) == OCT_OK) {
		name = ohGetName(&obj);
		if (name == NIL(char)) return OCT_ERROR;
		if (name[0] == '\0') {
			/* make sure the name is not empty */
			if (obj.type == OCT_INSTANCE) {
				name = obj.contents.instance.master;
			} else {
				name = ohTypeName(&obj);
			} /* if... */
		} /* if... */
		name = util_strsav(name);
		count = 0;
		if (st_lookup(name_table, name, (char **) &count)) {
			(void) sprintf(dummyname, "%s-%d", name, count++);
		} else {
			(void) strcpy(dummyname, name);
		} /* if... */
		(void) st_insert(name_table, name, (char *) count);
		OH_ASSERT (ohPutName(&obj, dummyname));
		OH_ASSERT(octModify(&obj));
	} /* while... */
	(void) st_free_table(name_table);
	return OCT_OK;
} /* myUniqNames... */

/*********************************************************************
 * STRPROPEQL
 * Is the value of a string property prop equal to s ?
 */
int16 StrPropEql(prop,s)
	octObject *prop; char *s;
{
	return(!(strcmp(prop->contents.prop.value.string,s)));
} /* StrPropEql... */

