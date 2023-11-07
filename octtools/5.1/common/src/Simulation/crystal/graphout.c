/* graphout.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)graphout.c	2.4 (Berkeley) 10/18/84"
 *
 * This file contains routines to print out Crystal results
 * in a graphical form suitable for processing by various
 * layout editors.
 */

#include "port.h"

/* Forward references: */

extern CaesarBegin(), CaesarPrint(), NullRtn(), SquidBegin(), SquidPrint();
extern MagicPrint(), MagicEnd();

/* The following tables define the various forms of graphical
 * output that we understand.
 */

int curEditor = 0;

char *(editorNames[]) = {
    "caesar",
    "magic",
    "squid",
    NULL};

/* Outputs initial information at beginning of file. */

int (*(beginRoutine[]))() = {
    CaesarBegin,
    NullRtn,
    SquidBegin,
    NULL};

/* Outputs an area and associated text. */

int (*(printRoutine[]))() = {
    CaesarPrint,
    MagicPrint,
    SquidPrint,
    NULL};

/* Outputs end information (but doesn't close file). */

int (*(endRoutine[]))() = {
    NullRtn,
    MagicEnd,
    NullRtn,
    NULL};

/* Used by the Squid output routines to increment a unique id. */

int SquidId;


GrSetEditor(name)
char *name;			/* Name of editor for which to generate
				 * graphical output.
				 */

/*---------------------------------------------------------
 *	Sets the style for graphical editor output.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Name is looked up in the table of valid output styles.
 *	If it matches, the style is changed so that the -g
 *	switch uses this style.
 *---------------------------------------------------------
 */

{
    int index;

    index = Lookup(name, editorNames);
    if (index == -1)
	printf("Ambiguous style: %s\n", name);
    else if (index == -2)
	printf("Unknown style: %s\n", name);
    else curEditor = index;
}

char *GrGetEditor()
{
    return editorNames[curEditor];
}


/*---------------------------------------------------------
 *	This page contains driver routines for graphical
 *	output.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The routines here do nothing but look up the right
 *	routine to do the job for the current output style,
 *	and then invoke that routine.
 *---------------------------------------------------------
 */

GrBegin(file, totalChunks)
FILE *file;
{
    (*beginRoutine[curEditor])(file, totalChunks);
}

GrPrint(file, xbot, ybot, xsize, ysize, text)
FILE *file;
float xbot, ybot, xsize, ysize;
char *text;
{
    (*printRoutine[curEditor])(file, xbot, ybot, xsize, ysize, text);
}

GrEnd(file)
FILE *file;
{
    (*endRoutine[curEditor])(file);
}


NullRtn()

/*---------------------------------------------------------
 *	This is a dummy routine that does nothing.
 *
 *	Results:	None.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    return;
}


CaesarBegin(file)
FILE *file;			/* File to hold Caesar command info. */

/*---------------------------------------------------------
 *	This routine outputs initial statements at the
 *	beginning of Caesar command files.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A Caesar command is output to make labels visible.
 *---------------------------------------------------------
 */

{
    fprintf(file, "vis +l\n");
}

CaesarPrint(file, xbot, ybot, xsize, ysize, text)
FILE *file;			/* File to hold Caesar command info. */
float xbot, ybot;		/* A rectangular area to be labelled. */
float xsize, ysize;
char *text;			/* A text label for the area. */

/*---------------------------------------------------------
 *	This routine outputs commands to label an area of
 *	layout with the error layer and a text string.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Caesar commands are output to the file.
 *---------------------------------------------------------
 */

{
    fprintf(file, "push %.0f %.0f %.0f %.0f\n", xbot, ybot, xsize, ysize);
    fprintf(file, "paint e\n");
    fprintf(file, "label %s\n", text);
}


SquidBegin(file, totalChunks)
FILE *file;			/* File to hold Squid info. */
int totalChunks;		/* Total number of highlights to be output
				 * to file.
				 */

/*---------------------------------------------------------
 *	This routine outputs initial statements at the
 *	beginning of Squid command files.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A Caesar command is output to make labels visible.
 *---------------------------------------------------------
 */

{
    fprintf(file, "SQUID\n");
    fprintf(file, "PUT VIEW \"\" \"\" \"\" \"squidNextObjectID\" INT %d\n",
	totalChunks);
    SquidId = 1;
}

SquidPrint(file, xbot, ybot, xsize, ysize, text)
FILE *file;			/* File to hold Squid command info. */
float xbot, ybot;		/* A rectangular area to be labelled. */
float xsize, ysize;
char *text;			/* A text label for the area. */

/*---------------------------------------------------------
 *	This routine outputs commands to label an area of
 *	layout with the error layer and a text string.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Squid commands are output to the file.
 *---------------------------------------------------------
 */

{
    fprintf(file, "MK RECT %d FRAME LAYER \"CRYSTAL\" FILL", SquidId);
    fprintf(file, " LB %.0f %.0f RT %.0f %.0f\n", xbot, ybot,
	xbot+xsize, ybot+ysize);
    fprintf(file, "PUT RECT %d \"Crystal info\" STRING", SquidId);
    fprintf(file, " \"%s\"\n", text);
    SquidId += 1;
}

MagicPrint(file, xbot, ybot, xsize, ysize, text)
FILE *file;			/* File to hold Magic command info. */
float xbot, ybot;		/* A rectangular area to be labelled. */
float xsize, ysize;
char *text;			/* Explanatory message. */

/*---------------------------------------------------------
 *	This routine outputs commands to create a feedback
 *	area with the given location and explanation.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A feedback creation command sequence is output.
 *---------------------------------------------------------
 */

{
    fprintf(file, "box %.0f %.0f %.0f %.0f\n", xbot, ybot,
	xbot+xsize, ybot+ysize);
    fprintf(file, "feedback add \"%s\"\n", text);
}

MagicEnd(file)
FILE *file;			/* File to hold Magic commands. */

/*---------------------------------------------------------
 *	This routine outputs a few closing Magic commands to
 *	tell the user what to do with his feedback.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	More commands are output.
 *---------------------------------------------------------
 */

{
    fprintf(file, "echo \"Crystal information entered as feedback.\"\n");
}
