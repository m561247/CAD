/* att.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)att.c	2.1 (Berkeley) 4/9/83"
 *
 * This file contains routines that manage attributes for Crystal.
 * Attributes are a way for designers to feed extra information into
 * analysis programs like Crystal.  The main way that attributes are
 * used now is to indicate the direction of flow in pass transistors.
 */

#include "crystal.h"
#include "hash.h"

/* Imports from other Crystal modules: */

extern char *NextField(), *ExpandNext(), *PrintFET();
extern HashTable NodeTable;

HashTable AttTable;		/* Used to hold attribute information. */


Attribute *
AttBuild(name)
char *name;

/*---------------------------------------------------------
 *	This procedure makes sure that an attribute by a
 *	given name exists in the attribute table.
 *
 *	Results:
 *	The return value is a pointer to the attribute.
 *
 *	Side Effects:
 *	A new Attribute record is added to the attribute hash
 *	table if necessary.
 *---------------------------------------------------------
 */

{
    Attribute *att;
    HashEntry *h;

    /* Make sure there's already a hash table entry for the
     * attribute, then allocate the attribute itself if there
     * isn't one already.
     */

    h = HashFind(&AttTable, name);
    att = (Attribute *) HashGetValue(h);
    if (att == 0)
    {
	att = (Attribute *) malloc(sizeof(Attribute));
	att->at_name = h->h_name;
	att->at_entered = (Node *) NULL;
	att->at_left = (Node *) NULL;
	att->at_flags = 0;
	HashSetValue(h, att);
    }
    return att;
}


AttInit()

/*---------------------------------------------------------
 *	This routine just initializes the attribute hash
 *	table.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The attribute table is initialized.
 *---------------------------------------------------------
 */

{
    Attribute *att;

    HashInit(&AttTable, 10);
    att = AttBuild("In");
    att->at_flags = A_INONLY;
    att = AttBuild("Out");
    att->at_flags = A_OUTONLY;
}


char *
AttParseNode(node, string)
Node *node;
char *string;

/*---------------------------------------------------------
 *	AttParseNode parses attribute information for a
 *	node, and sets up attribute table info.
 *
 *	Results:
 *	NULL is returned if no errors were encountered.
 *	Otherwise an error string is returned.
 *
 *	Side Effects:
 *	For now, there are none, since node attributes are
 *	ignored entirely.
 *---------------------------------------------------------
 */

{
    return NULL;
}


char *
AttParseFET(fet, string)
FET *fet;
char *string;

/*---------------------------------------------------------
 *	AttParseFET parses an attribute string specified for
 *	a transistor.  The attribute string must be of the
 *	form "x=a,b,c,...", where x is "s", "d", or "g" to
 *	indicate that the attribute pertains to the transistor
 *	source, drain or gate, and each of the comma-separated
 *	strings a, b, c, ... is of the form "Crystal:goo", where
 *	goo is the attribute information we store.
 *
 *	Results:
 *	The return value is NULL if the attribute was parsed
 *	successfully.  Otherwise, an error string is returned.
 *	Attributes not pertaining to Crystal are ignored.
 *
 *	Side Effects:
 *	If the attribute is parsed correctly, attribute records
 *	are created to associate it with the transistor.
 *---------------------------------------------------------
 */

{
    char *next;
    APointer *ap;
    Attribute *att;
    int aloc;			/* What the attribute refers to (s, d, g). */

    /* Make sure the attribute has the "x=" stuff at the front. */

    switch (*string)
    {
	case 's': aloc = AP_SOURCE;  break;
	case 'd': aloc = AP_DRAIN; break;
	case 'g': aloc = AP_GATE; break;
	default: return "bad terminal";
    }
    string += 1;
    if (*string != '=') return "bad format";
    string += 1;

    /* Now skim off the comma-separated attributes one at a time. */

    if (*string == 0) next = 0;
    else next = string;
    while (next != 0)
    {
	string = next;

	/* Find the comma that ends the attribute, and replace it with
	 * a null.  Then save the location of the next attribute.
	 */
	
	next = strchr(string, ',');
	if (next != 0) *next++ = 0;

	/* Make sure that the first characters of the attribute are
	 * the letters "Crystal:" or "Cr:".  Then skip them.
	 */
	
	if (strncmp(string, "Cr:", 3) == 0) string += 3;
	else if (strncmp(string, "Crystal:", 8) == 0) string += 8;
	else continue;
	if (*string == 0) continue;

	/* Now make an attribute for what is left and tie it into the
	 * list for the transistor.
	 */
	
	att = AttBuild(string);
	ap = (APointer *) malloc(sizeof(APointer));
	ap->ap_att = att;
	ap->ap_flags = aloc;
	ap->ap_next = fet->f_ap;
	fet->f_ap = ap;
    }
    return NULL;
}


int
AttCheckIn(fet, node)
register FET *fet;		/* Fet being passed though. */
Node *node;			/* Node from which info flows INTO fet.*/

/*---------------------------------------------------------
 *	AttCheckIn sees if it is OK to pass through a transistor
 *	in a given direction.
 *
 *	Results:
 *	The return value is TRUE if it is OK to pass through,
 *	FALSE otherwise.
 *
 *	Side Effects:
 *	If TRUE is returned, then info may have been changed
 *	in the attribute.  AttCheckOut must be called to
 *	reset the bits.
 *
 *	More Info:
 *	This procedure and AttCheckOut are the ones that use
 *	direction information in transistors.  This procedure
 *	is called when starting through a transistor, and
 *	AttCheckOut is called when all paths through the transistor
 *	have been finished with.  Source and drain attributes
 *	indicate directions of information flow.  Once information
 *	has flowed into a transistor with a source or drain attribute
 *	it cannot flow into any other transistor in the opposite
 *	direction until AttCheckOut has been called.  This prevents
 *	us from chasing illegitimate paths through muxes and shift
 *	arrays.
 *---------------------------------------------------------
 */

{
    int enteredBit;		/* Selects whether info enters at source
				 * or drain. */
    int leftBit;		/* Selects where info leaves transistor. */
    register APointer *ap;

    /* Take a quick return if there are no attributes. */

    if (fet->f_ap == NULL) return TRUE;

    /* Figure out which side we're coming from, then skim through the
     * attribute list to make sure that there are no direction violations.
     */

    if (node == fet->f_source)
    {
	enteredBit = AP_SOURCE;
	leftBit = AP_DRAIN;
    }
    else if (node == fet->f_drain)
    {
	enteredBit = AP_DRAIN;
	leftBit = AP_SOURCE;
    }
    else return TRUE;

    for (ap = fet->f_ap; ap != NULL; ap = ap->ap_next)
    {
	if (ap->ap_att->at_flags & A_IGNORE) continue;
	if (ap->ap_flags & enteredBit)
	    if ((ap->ap_att->at_flags & A_OUTONLY)
		|| (ap->ap_att->at_left != NULL)) return FALSE;
	if (ap->ap_flags & leftBit)
	    if ((ap->ap_att->at_flags & A_INONLY)
		|| (ap->ap_att->at_entered != NULL)) return FALSE;
    }

    /* Now go through again and turn on bits in attributes so
     * we can't go the wrong way later.
     */

    for (ap = fet->f_ap; ap != NULL; ap = ap->ap_next)
    {
	if ((ap->ap_flags & enteredBit) && (ap->ap_att->at_entered == NULL))
	    ap->ap_att->at_entered = node;
	if ((ap->ap_flags & leftBit) && (ap->ap_att->at_left == NULL))
	    ap->ap_att->at_left = node;
    }
    return TRUE;
}


AttCheckOut(fet, node)
FET *fet;
Node *node;			/* Node from which info flowed into fet. */

/*---------------------------------------------------------
 *	This procedure just clears the markings left by
 *	AttCheckIn.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	All the markings left by the corresponding call to
 *	AttCheckIn are cleared.
 *---------------------------------------------------------
 */

{
    APointer *ap;

    for (ap = fet->f_ap; ap != NULL; ap = ap->ap_next)
    {
	if (ap->ap_att->at_entered == node)
	    ap->ap_att->at_entered = (Node *) NULL;
	if (ap->ap_att->at_left == node)
	    ap->ap_att->at_left = (Node *) NULL;
    }
}


AttPrint(node)
Node *node;

/*---------------------------------------------------------
 *	This routine prints out the information in all
 *	attributes for the node or any transistors connecting
 *	to the node.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Stuff is printed on standard output.
 *---------------------------------------------------------
 */

{
    APointer *ap;
    FET *fet;
    Pointer *p;

    for (p = node->n_pointer; p != NULL; p = p->p_next)
    {
	fet = p->p_fet;
	printf("FET: %s\n", PrintFET(fet));
	for (ap = fet->f_ap; ap != NULL; ap = ap->ap_next)
	{
	    printf("    ");
	    if (ap->ap_flags & AP_SOURCE) printf("S");
	    if (ap->ap_flags & AP_DRAIN)  printf("D");
	    if (ap->ap_flags & AP_GATE)   printf("G");
	    printf(": %s\n", ap->ap_att->at_name);
	}
    }
}


AttPrintFromString(string)
char *string;

/*---------------------------------------------------------
 *	This routine reads node names from a string and
 *	prints out attribute information.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Lots of stuff gets printed.
 *---------------------------------------------------------
 */

{
    char *p;
    Node *n;

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != NULL)
	    AttPrint(n);
    }
}


AttFlow(string)
char *string;			/* Describes flow information. */

/*---------------------------------------------------------
 *	This routine marks attributes to restrict flow or
 *	permit arbitrary flow.  The string contains one of
 *	the keywords "in", "out", or "ignore", followed by
 *	one or more attribute names.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Each of the named attributes is marked according to
 *	the keyword:  "in" means that signals may only flow
 *	into the transistor from the side containing the
 *	attribute;  "out" means that signals may only flow
 *	out at the side of the attribute (i.e. the transistor
 *	can only drive gates on the attribute side);  and "ignore"
 *	means completely ignore this attribute for flow control.
 *---------------------------------------------------------
 */

{
    static char *(keywords[]) = {"ignore", "in", "out", NULL};
    Attribute *att;
    char *p;
    int index, flags;

    /* Scan off the keyword and look it up. */

    p = NextField(&string);
    if (p == NULL)
    {
	printf("No keyword given:  try ignore, in, or out.\n");
	return;
    }
    index = Lookup(p, keywords);
    if (index < 0)
    {
	printf("Bad keyword:  try ignore, in, or out.\n");
	return;
    }
    switch (index)
    {
	case 0: flags = A_IGNORE; break;
	case 1: flags = A_INONLY; break;
	case 2: flags = A_OUTONLY; break;
    }

    /* Go through the attributes and mark them. */

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((att = (Attribute *) ExpandNext(&AttTable))
	    != (Attribute *) NULL)
	{
	    att->at_flags &= ~(A_INONLY | A_OUTONLY | A_IGNORE);
	    att->at_flags |= flags;
	}
    }
}
