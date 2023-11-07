/* misc.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)misc.c	2.8 (Berkeley) 12/11/83"
 *
 * This file contains miscellaneous command routines for Crystal.
 */

#include "crystal.h"
#include "cOct.h"
#include "hash.h"
#include "vov.h"

extern Type TypeTable[];
extern char *NextField();
extern HashTable NodeTable;
extern char *PrintFET(), *ExpandNext();
extern Stage *DPGetCrit();
extern Node *VddNode, *GroundNode;
unsigned cOctStatus;


Source(name)
char *name;			/* File from which to read commands. */

/*---------------------------------------------------------
 *	This command routine merely reads other commands
 *	from a given file.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Whatever the commands in the file do.
 *---------------------------------------------------------
 */

{
    FILE *f;
    char string[100];

    if (name[0] == 0)
    {
	printf("No command file name given.\n");
	return;
    }
    sscanf(name, "%s", string);
    f = VOVfopen(string, "r");
    if (f == NULL)
    {
	printf("Couldn't find file %s.\n", string);
	return;
    }
    Command(f);
    fclose(f);
}


SetCap(string)
char *string;			/* Contains a capacitance value in pfs,
				 * followed by one or more node names.
				 */

/*---------------------------------------------------------
 *	This routine allows users to change the parasitic
 *	capacitances associated with nodes.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The named node(s) get a new capacitance value.
 *---------------------------------------------------------
 */

{
    char *p;
    float value;
    Node *n;

    p = NextField(&string);
    if ((p == NULL) || (sscanf(p, "%f", &value) == 0))
    {
	printf("No capacitance value given.\n");
	return;
    }

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	    n->n_C = value;
    }
}


SetRes(string)
char *string;			/* Contains a resistance value in ohms,
				 * followed by one or more node names.
				 */

/*---------------------------------------------------------
 *	This routine allows users to change the parasitic
 *	resistances associated with nodes.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The named node(s) get a new resistance value.
 *---------------------------------------------------------
 */

{
    char *p;
    float value;
    Node *n;

    p = NextField(&string);
    if ((p == NULL) || (sscanf(p, "%f", &value) == 0))
    {
	printf("No resistance value given.\n");
	return;
    }

    while ((p = NextField(&string)) != NULL)
    {
	ExpandInit(p);
	while ((n = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	    n->n_R = value;
    }
}


Stats()

/*---------------------------------------------------------
 *	Command routine to print statistics from all modules.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Statistics-printing routines are invoked from many
 *	other modules.
 *---------------------------------------------------------
 */

{
    BuildStats();
    MarkStats();
    DelayStats();
    DPStats();
}


BuildCmd(string)
char *string;			/* Contains the name of a file. */

/*---------------------------------------------------------
 *	This command invokes a routine to read in a .sim file
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A .sim file is read in.
 *---------------------------------------------------------
 */

{
    char name[100];
    if (sscanf(string, "%99s", name) != 1)
    {
	printf("No .sim file name given.\n");
	return;
    }
    if( cOctStatus & COCT_ON)
	(void) cOctBuildNet(name);
    else
	(void) BuildNet(name);
}


ExtractCmd(string)
char *string;			/* Contains command parameters. */

/*---------------------------------------------------------
 *	This command outputs a sed script that can be used
 *	to extract just a critical path from a .sim file.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Writes out one or more sed commands.  The parameters
 *	consist of a file name, followed by one or more critical
 *	path names.  For each critical path, commands are output
 *	in the file.
 *---------------------------------------------------------
 */

{
    FILE *f;
    char *fileName, *p;
    Stage *crit;
    FET *fet;
    int i;

    fileName = NextField(&string);
    if (fileName == NULL)
    {
	printf("No file name given.\n");
	return;
    }
    f = VOVfopen(fileName, "w");
    if (f == NULL)
    {
	printf("Couldn't open %s for writing.\n", fileName);
	return;
    }

    /* This is pretty straightforward:  just output sed commands to
     * print all lines containing one of the node names along the
     * critical path.  The only tricky point is that we have to also
     * grab information about nodes that drive transistors along the
     * path.  This is so that when the file is read back in again,
     * Crystal will be able to distinguish transistors whose gates
     * are pass-transistor-driven.
     */

    fprintf(f, "/^|/p\n");
    p = NextField(&string);
    if (p == NULL) p = "1";
    for ( ; p != NULL; p = NextField(&string))
    {
	crit = DPGetCrit(p);
	if (crit == NULL)
	{
	    printf("There isn't a critical path %s.\n", p);
	    continue;
	}
	for (; crit != NULL;  crit = crit->st_prev)
	{
	    for (i = crit->st_piece2Size-1; i >= 0;  i -= 1)
	    {
		fet = crit->st_piece2FET[i];
		if ((fet != NULL) && (fet->f_gate != fet->f_source)
		    && (fet->f_gate != fet->f_drain)
		    && (fet->f_gate != VddNode)
		    && (fet->f_gate != GroundNode))
		    fprintf(f, "/ %s /p\n", fet->f_gate->n_name);
		if ((i == 0) && (crit->st_piece1Size == 0)
		    && (crit->st_prev != NULL)) continue;
		fprintf(f, "/ %s /p\n", crit->st_piece2Node[i]->n_name);
	    }
	    for (i = 0; i < crit->st_piece1Size; i += 1)
	    {
		fet = crit->st_piece1FET[i];
		if ((i != 0) && (fet->f_gate != VddNode)
		    && (fet->f_gate != GroundNode))
		    fprintf(f, "/ %s /p\n", fet->f_gate->n_name);
		if (i == crit->st_piece1Size-1) continue;
		fprintf(f, "/ %s /p\n", crit->st_piece1Node[i]->n_name);
	    }
	}
    }
    fclose(f);
}


/*---------------------------------------------------------
 *	The procedures on this page provide utility routines
 *	to scan a file and replace keywords within the file
 *	with text supplied by calling routines.  subsInit
 *	is used to initialize, subsReplace is used to replace
 *	one keyword, and subsClose is used to close the file.
 *---------------------------------------------------------
 */

/* The following variables hold the current line from the file.
 * The general strategy is to read a line, substitute by copying
 * from one buffer to the other, then eventually write the line
 * back out again.
 */

#define LINESIZE 200
static char line1[LINESIZE], line2[LINESIZE];
static char *curLine, *nextLine;

/* ARGSUSED */

subsInit(inFile, outFile)
FILE *inFile;			/* File from which input is to be read. */
FILE *outFile;			/* File in which output will be produced. */
{
    curLine = fgets(line1, LINESIZE, inFile);
    nextLine = line2;
}

subsReplace(inFile, outFile, keyword, string)
FILE *inFile;			/* File from which input is to be read. */
FILE *outFile;			/* File in which output is written. */
char *keyword;			/* Keyword to be replaced. */
char *string;			/* Value to replace keyword. */
{
    char *p;
    int sLength = strlen(string);
    int kLength = strlen(keyword);
    int tmp;

    while (curLine != NULL)
    {
	p = curLine;
	while ((p = strchr(p, *keyword)) != 0)
	{
	    if (strncmp(p, keyword, kLength) != 0)
	    {
		p++;
		continue;
	    }
	    tmp = p-curLine;
	    strncpy(nextLine, curLine, tmp);
	    strncpy(nextLine+tmp, string, sLength);
	    strcpy(nextLine+tmp+sLength, p+kLength);
	    p = nextLine;
	    nextLine = curLine;
	    curLine = p;
	    return;
	}
	fputs(curLine, outFile);
	curLine = fgets(curLine, LINESIZE, inFile);
    }
}

/* ARGSUSED */

subsClose(inFile, outFile)
FILE *inFile;			/* File to read from. */
FILE *outFile;			/* File to write to. */
{
    while (curLine != NULL)
    {
	fputs(curLine, outFile);
	curLine = fgets(curLine, LINESIZE, inFile);
    }
}


FillinCmd(string)
char *string;			/* Contains parameters for command. */

/*---------------------------------------------------------
 *	This command fills in critical path information in
 *	a file.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The argument string contains "type inFile outFile keyword
 *	path path ...".  Type is one of "time", "edgeSpeed".
 *	The paths are scanned from beginning to end, and inFile is
 *	copied to outFile with the first occurrences of keyword
 *	replaced by the delay time to successive stages of the paths.
 *---------------------------------------------------------
 */

{
    FILE *inFile, *outFile;
    char *arg1, *arg2, *arg3, *keyword, *pathName;
    static char *(types[]) = {"edgeSpeed", "time", NULL};
    int time;
    Stage *path;

    /* Scan off the arguments. */

    arg1 = NextField(&string);
    arg2 = NextField(&string);
    arg3 = NextField(&string);
    keyword = NextField(&string);
    pathName = NextField(&string);
    if (keyword == NULL)
    {
	printf("Not enough arguments for fillin command.\n");
	return;
    }
    time = Lookup(arg1, types);
    if (time < 0)
    {
	printf("Type of fillin must be \"time\" or \"edgeSpeed\".\n");
	return;
    }
    inFile = VOVfopen(arg2, "r");
    if (inFile == NULL)
    {
	printf("Couldn't open %s for reading.\n", arg2);
	return;
    }
    outFile = VOVfopen(arg3, "w");
    if (outFile == NULL)
    {
	printf("Couldn't open %s for writing.\n", arg3);
	return;
    }
    if (pathName == NULL) pathName = "1";

    subsInit(inFile, outFile);
    for ( ; pathName != NULL; pathName = NextField(&string))
    {
	path = DPGetCrit(pathName);
	if (path == NULL)
	{
	    printf("Critical path %s doesn't exist.\n", pathName);
	    continue;
	}
	fillin(inFile, outFile, keyword, path, time);
    }
    subsClose(inFile, outFile);
    fclose(inFile);
    fclose(outFile);
}

/* The following recursive procedure is needed to print out
 * the stages in increasing order of delay.  Notice that the
 * very first stage in the path (the driver stage) isn't
 * printed.
 */

fillin(inFile, outFile, keyword, path, time)
FILE *inFile, *outFile;
char *keyword;
Stage *path;
int time;
{
    char value[50];

    if (path->st_prev != NULL)
	fillin(inFile, outFile, keyword, path->st_prev, time);
    else return;
    if (time) sprintf(value, "%.2f", path->st_time);
    else sprintf(value, "%.2f", path->st_edgeSpeed);
    subsReplace(inFile, outFile, keyword, value);
}
