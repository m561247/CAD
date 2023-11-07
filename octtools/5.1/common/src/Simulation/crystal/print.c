/* print.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)print.c	2.7 (Berkeley) 10/14/88"
 *
 * This file contains a few routines that are used to identify
 * nodes and transistors in a standard way for Crystal.
 */

#include "crystal.h"
#include "hash.h"
#include "vov.h"

/* Library imports: */

/* Imports from other Crystal modules: */

extern char *ExpandNext();
extern char *NextField();
extern int RepeatLimit;
extern HashTable NodeTable;
extern Type TypeTable[];

extern char *VddName, *GroundName;	/* For prnodes.*/

/* The following global contains the micron units in which distances
 * are printed.  That is, if PrintUnits is 2.0, then a print-out of 1
 * corresponds to 2.0 microns.
 */

float PrintUnits = 2.0;


char *
PrintFET(fet)
FET *fet;			/* Pointer to FET to be printed. */

/*---------------------------------------------------------
 *	This routine generates a string to describe a FET.
 *
 *	Results:
 *	The return value is a pointer to a string describing
 *	the indicated FET.  Note:  the string is in static
 *	storage that will be trashed by the next call to this
 *	routine.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
	static char string[100];
	sprintf(string, "g=%s, s=%s, d=%s, loc=%.0f,%.0f",
	    fet->f_gate->n_name, fet->f_source->n_name,
	    fet->f_drain->n_name, fet->f_xloc/PrintUnits,
	    fet->f_yloc/PrintUnits);
	return string;
}


char *
PrintNode(node)
Node *node;			/* Pointer to node to be printed. */

/*---------------------------------------------------------
 *	This routine generates a string describing a node
 *	in a standard way.
 *
 *	Results:
 *	The return value is a pointer to a string describing
 *	the node.  The string is allocated in static storage
 *	and will be trashed the next time this procedure is
 *	called.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    float x, y;
    static char string[100];
    Pointer *p;
    char *terminal;
    FET *f;
    int i;

    p = node->n_pointer;
    if (p == NULL)
    {
	x = y = 0;
	terminal = "?";
    }
    else
    {
	/* Try to use a gate as the reference location for the node. */

	for (i=0; i<10; i++)
	{
	    f = p->p_fet;
	    if (node == f->f_gate) break;
	    if (p->p_next == NULL) break;
	    p = p->p_next;
	}
	x = f->f_xloc/PrintUnits;
	y = f->f_yloc/PrintUnits;
	if (node == f->f_gate) terminal = "gate";
	else if (node == f->f_source) terminal = "source";
	else if (node == f->f_drain) terminal = "drain";
	else
	{
	    printf("Crystal bug: node %s doesn't connect right!\n",
		node->n_name);
	    return "??";
	}
    }
    sprintf(string, "%s (see %s at %.0f,%.0f)",
	node->n_name, terminal, x, y);
    return string;
}



/*---------------------------------------------------------
 *	The routines on this page find the coordinates for
 *	a Node or FET, and return them as integers.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The x and y parameters are filled in with the
 *	coordinates of the FET or Node, and the terminal
 *	parameter in PrintNode is filled in with the text
 *	name of the terminal corresponding to the node.
 *---------------------------------------------------------
 */

PrintFETXY(fet, x, y)
FET *fet;
float *x, *y;			/* Filled in with x- and y-locations. */
{
    *x = fet->f_xloc/PrintUnits;
    *y = fet->f_yloc/PrintUnits;
}

PrintNodeXY(node, pterm, x, y)
Node *node;
char **pterm;			/* Filled in with name of terminal to
				 * which node connects, unless NULL.
				 */
float *x, *y;			/* Filled in with x and y locations. */
{
    Pointer *p;
    int i;
    FET *f;

    p = node->n_pointer;
    *x = 0.0;
    *y = 0.0;
    if (pterm != NULL) *pterm = "??";
    if (p == NULL) return;
    for (i=0; i<10; i++)
    {
	f = p->p_fet;
	if (f->f_gate == node) break;
	if (p->p_next == NULL) break;
	p = p->p_next;
    }
    if (pterm != NULL)
    {
	if (f->f_gate == node) *pterm = "gate";
	else if (f->f_source == node) *pterm = "source";
	else if (f->f_drain == node) *pterm = "drain";
	else *pterm = "??";
    }
    *x = f->f_xloc/PrintUnits;
    *y = f->f_yloc/PrintUnits;
}


PrintCap(string)
char *string;		/* Describes what nodes to print. */

/*---------------------------------------------------------
 *	This routine prints out all nodes with whose internal
 *	capacitance is greater than a given threshold.  String
 *	contains switches (-c for Caesar command file, -t for
 *	threshold, default 0) followed by an optional list
 *	of nodes.  If no nodes are specified, than "*" is used.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Stuff is output on stdout, and perhaps also to a Caesar
 *	command file.
 *---------------------------------------------------------
 */

{
    float threshold, x, y;
    char *p, text[20], *terminal, label[50];
    Node *n;
    HashTable ht;
    HashEntry *h;
    int i, nDups;
    FILE *file;

    /* See if there are any switches. */

    file = NULL;
    threshold = 0.0;
    while ((p = NextField(&string)) != NULL)
    {
	if (*p != '-') break;
	p += 1;
	switch (*p)
	{
	    case 'g':
		if (*++p == 0) p = NextField(&string);
		if (p == NULL)
		{
		    printf("No file name given for graphical output.\n");
		    break;
		}
		file = VOVfopen(p, "w");
		if (file == NULL)
		{
		    printf("Couldn't access file.\n");
		    break;
		}
		break;

	    case 't':
		if (*++p == 0) p = NextField(&string);
		if ((p == NULL) || (sscanf(p, "%f", &threshold) != 1))
		{
		    printf("No threshold value given, using 0.\n");
		    break;
		}
		break;
	
	    default:
		printf("Unknown switch %c ignored.\n", *p);
		break;
	}
    }
    if (file != NULL) GrBegin(file, 0);

    HashInit(&ht, 16);
    nDups = 0;
    if (p == NULL) p = "*";
    while (p != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != NULL)
	{
	    if (n->n_C < threshold) continue;
	    (void) sprintf(text, "%.3f", n->n_C);
	    h = HashFind(&ht, text);
	    i = (int) HashGetValue(h) + 1;
	    HashSetValue(h, i);
	    if (i > RepeatLimit)
	    {
		nDups += 1;
		continue;
	    }
	    if (file == NULL)
	        printf("%s pf capacitance at %s.\n", text, PrintNode(n));
	    else
	    {
		PrintNodeXY(n, &terminal, &x, &y);
		sprintf(label, "%s=%s:%spf", n->n_name, terminal, text);
		GrPrint(file, x, y, 2.0, 2.0, label);
	    }
	}
	p = NextField(&string);
    }
    HashKill(&ht);
    if (file != NULL)
    {
	GrEnd(file);
	(void) fclose(file);
    }
    if (nDups != 0)
	printf("%d nodes ignored because of duplicate values.\n", nDups);
}


PrintRes(string)
char *string;		/* Describes what nodes to print. */

/*---------------------------------------------------------
 *	This routine prints out all nodes with whose internal
 *	resistance is greater than a given threshold.  String
 *	contains switches (-c for Caesar command file, -t for
 *	threshold value, default 0) followed by an optional list
 *	of nodes.  If no nodes are specified, than "*" is used.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Stuff is output on stdout, and perhaps also to a Caesar
 *	command file.
 *---------------------------------------------------------
 */

{
    float threshold, x, y;
    char *p, text[20], *terminal, label[50];
    Node *n;
    HashTable ht;
    HashEntry *h;
    int i, nDups;
    FILE *file;

    /* See if there are any switches. */

    file = NULL;
    threshold = 0.0;
    while ((p = NextField(&string)) != NULL)
    {
	if (*p != '-') break;
	p += 1;
	switch (*p)
	{
	    case 'g':
		if (*++p == 0) p = NextField(&string);
		if (p == NULL)
		{
		    printf("No file name given for graphical output.\n");
		    break;
		}
		file = VOVfopen(p, "w");
		if (file == NULL)
		{
		    printf("Couldn't access file.\n");
		    break;
		}
		break;

	    case 't':
		if (*++p == 0) p = NextField(&string);
		if ((p == NULL) || (sscanf(p, "%f", &threshold) != 1))
		{
		    printf("No threshold value given, using 0.\n");
		    break;
		}
		break;
	
	    default:
		printf("Unknown switch %c ignored.\n", *p);
		break;
	}
    }
    if (file != NULL) GrBegin(file, 0);

    HashInit(&ht, 16);
    nDups = 0;
    if (p == NULL) p = "*";
    while (p != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != NULL)
	{
	    if (n->n_R < threshold) continue;
	    (void) sprintf(text, "%.0f", n->n_R);
	    h = HashFind(&ht, text);
	    i = (int) HashGetValue(h) + 1;
	    HashSetValue(h, i);
	    if (i > RepeatLimit)
	    {
		nDups += 1;
		continue;
	    }
	    if (file == NULL)
	        printf("%s ohms resistance at %s.\n", text, PrintNode(n));
	    else
	    {
		PrintNodeXY(n, &terminal, &x, &y);
		sprintf(label, "%s=%s:%sohms", n->n_name, terminal, text);
		GrPrint(file, x, y, 2.0, 2.0, label);
	    }
	}
	p = NextField(&string);
    }
    HashKill(&ht);
    if (file != NULL)
    {
	GrEnd(file);
	(void) fclose(file);
    }
    if (nDups != 0)
	printf("%d nodes ignored because of duplicate values.\n", nDups);
}


PrFETsCmd(string)
char *string;

/*---------------------------------------------------------
 *	This routine prints info about one or more FETs.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	For each node listed in string, info is printed about
 *	all FETs whose gates attach to that node.  If no
 *	nodes are given, then all FETs are printed.
 *---------------------------------------------------------
 */

{
    Node *n;
    FET *f;
    Pointer *p;
    char *s;

    for ( ; isspace(*string); string++) /* do nothing */;
    if (*string == 0) string = "*";
    while ((s = NextField(&string)) != NULL)
    {
	ExpandInit(s);
	while ((n = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	{
	    for (p = n->n_pointer; p != NULL; p = p->p_next)
	    {
		f = p->p_fet;
		if (f->f_gate != n) continue;
		printf("%s, aspect=%.2f area=%.1f\n", PrintFET(f),
		    f->f_aspect, f->f_area/(PrintUnits*PrintUnits));
		printf("    Type = %s", TypeTable[f->f_typeIndex].t_name);
		if (f->f_flags & FET_FLOWFROMSOURCE)
		    printf(", flowFromSource");
		if (f->f_flags & FET_FLOWFROMDRAIN)
		    printf(", flowFromDrain");
		if (f->f_flags & FET_FORCEDON) printf(", forcedOn");
		if (f->f_flags & FET_FORCEDOFF) printf(", forcedOff");
		if (f->f_flags & FET_ONALWAYS) printf(", onAlways");
		if (f->f_flags & FET_ON0) printf(", on0");
		if (f->f_flags & FET_ON1) printf(", on1");
		if (f->f_flags & FET_NOSOURCEINFO)
		    printf(", noSourceInfo");
		if (f->f_flags & FET_NODRAININFO)
		    printf(", noDrainInfo");
		if (f->f_flags & FET_SPICE)
		    printf(", spice");
		printf("\n");
	    }
	}
    }
}


PrintNodes (string)
char *string;

/*---------------------------------------------------------
 *	This routine prints info on all nodes.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	For each node listed in string, print info about it.
 *	If no nodes are given, then all nodes are printed.
 *---------------------------------------------------------
 */

{
    Node *n;
    char *p;

    /* See if there are any switches. */

    for ( ; isspace(*string); string++) /* do nothing */;
    if ((string == NULL) || (*string == 0)) p = "*";
    else p = NextField(&string);

    while (p != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != NULL)
	{
	    if ((strcmp (n->n_name, GroundName) == 0)
		|| (strcmp (n->n_name, VddName) == 0)) continue;
	    printf("Node %s set to %.2fns up, %.2fns down.\n", PrintNode(n),
		n->n_hiTime, n->n_loTime);
	}
	p = NextField(&string);
    }
}
