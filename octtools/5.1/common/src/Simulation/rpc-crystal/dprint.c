/* dprint.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)dprint.c	2.20 (Berkeley) 10/21/84"
 *
 * This file contains routines that record and print information
 * about long delay paths through the circuit.
 */

#include "crystal.h"
#include "cOct.h"

/* Imports from other Crystal modules: */

extern char *PrintNode(), *PrintFET(), *NextField();

/* Imports from the cOct module						*/
extern unsigned cOctStatus;

/* The following information is used to record information about
 * the long delay paths seen so far.  Information is recorded in
 * the form of a chain of delay stages, from last node to first
 * node in the path.  We can record up to MAXPATHS paths for memory
 * nodes, MAXPATHS for "watched" nodes, and MAXPATHS paths for
 * arbitrary nodes.  The information in each array is kept in
 * sorted order, with the fastest paths in the lowest index
 * positions.
 */

int DPNumMemPaths = 5;
int DPNumAnyPaths = 5;
int DPNumWatchPaths = 5;
#define MAXPATHS 100
int DPMaxPaths = MAXPATHS;

static Stage *(memPaths[MAXPATHS]), *(anyPaths[MAXPATHS]);
static Stage *(watchPaths[MAXPATHS]);

/* allows Oct module access to critical paths 				*/
Stage **OctCritPaths = anyPaths;

/* The thresholds below are exported to the outside world
 * to save procedure invocation overhead.  Delays less than
 * the threshold values cannot possibly be among the longest
 * seen so far.
 */

float DPMemThreshold = 0.0;
float DPAnyThreshold = 0.0;
float DPWatchThreshold = 0.0;

/* The numbers below are kept for the "statistics" command. */

static int DPnAnyRecords = 0;
static int DPnMemRecords = 0;
static int DPnWatchRecords = 0;
static int DPnDups = 0;

/* Flag controlling whether to print edge speeds. */

int DPrintEdgeSpeeds = FALSE;


putPath(src, pDest)
Stage *src;			/* Path to be copied. */
Stage **pDest;			/* Address of pointer to fill in with copy. */

/*---------------------------------------------------------
 *	PutPath makes a copy of src at the pointer referred
 *	to by pDest.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	If pDest currently is non-NULL, it is assumed to refer to
 *	a list of Stages;  the memory in the list is recycled to
 *	the storage allocator.  Then a copy is made of the list
 *	at src and a pointer to the copy is placed at pDest.
 *	If src is NULL, then there is nothing to copy, so all that
 *	happens is to free up the old list at pDest and NULL out
 *	pDest.
 *---------------------------------------------------------
 */

{
    Stage *stage, *next;
    
    /* Recycle the old path. */

    for (stage = *pDest; stage != NULL; stage = next)
    {
	next = stage->st_prev;
	free((char *) stage);
    }

    /* Create a copy of the new path. */

    while (src != NULL)
    {
	*pDest = (Stage *) malloc(sizeof(Stage));
	**pDest = *src;
	src = src->st_prev;
	pDest = &((*pDest)->st_prev);
    }
    *pDest = NULL;
}


DPRecord(stage, list)
Stage *stage;			/* List of stages to be recorded. */
int list;			/* 0 means record on "any" list, 1
				 * means record on memory list, 2 means
				 * record on watch list.
				 */

/*---------------------------------------------------------
 *	This procedure is invoked to (possibly) save delay
 *	information about a path.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	If this path is one of the slowest seen so far, it is
 *	copied into one of our arrays.  If the time in this
 *	path is very close (.1%) to a time that is already
 *	recorded, then it is ignored.
 *---------------------------------------------------------
 */

{
    int i, max;
    Stage **array;
    register Stage *cur;
    float bot, top;

    switch (list)
    {
	case 0:
	DPnAnyRecords += 1;
	array = anyPaths;
	max = DPNumAnyPaths;
	if (stage->st_time < DPAnyThreshold) return;
	break;

	case 1:
	DPnMemRecords += 1;
	array = memPaths;
	max = DPNumMemPaths;
	if (stage->st_time < DPMemThreshold) return;
	break;

	case 2:
	DPnWatchRecords += 1;
	array = watchPaths;
	max = DPNumWatchPaths;
	if (stage->st_time < DPWatchThreshold) return;
	break;
    }

    /* See if this path's delay duplicates a delay already in
     * the array.
     */

    bot = stage->st_time * .999;
    top = stage->st_time * 1.001;

    for (i=0; i<max; i+=1)
    {
	cur = array[i];
	if (cur == NULL) continue;
	if (cur->st_time > top) break;
	if ((cur->st_time >= bot) && (cur->st_rise == stage->st_rise))
	{
	    DPnDups += 1;
	    return;
	}
    }

    /* Delete the old first element in the array, since it is
     * going to be replaced.
     */

    putPath((Stage *) NULL, &(array[0]));

    /* Find the slot in which to store the new path. */

    for (i=1; i<max; i+=1)
    {
	cur = array[i];
	if (cur == NULL) continue;
	if (stage->st_time < cur->st_time) break;
	array[i-1] = cur;
    }
    array[i-1] = (Stage *) NULL;
    putPath(stage, &(array[i-1]));

    /* Find the new smallest time in this array for use as threshold. */

    if (array[0] == NULL) bot = 0.0;
    else bot = 1.001 * array[0]->st_time;
    switch (list)
    {
	case 0:
	    DPAnyThreshold = bot;
	    break;
	case 1:
	    DPMemThreshold = bot;
	    break;
	case 2:
	    DPWatchThreshold = bot;
	    break;
    }
}

DPrintStage(file, stage)
FILE *file;			/* Where to print. */
Stage *stage;			/* Stage to be printed. */

/*---------------------------------------------------------
 *	This routine prints out the information in a stage
 *	in human-readable form.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	None, except that the information in the stage is printed.
 *---------------------------------------------------------
 */

{
    int first = TRUE;
    int i;
    char *direction, *format;
    float x, y;

    if (stage == NULL)
    {
	fprintf(file, "No such critical path.\n");
	return;
    }
    for ( ; stage != NULL; stage = stage->st_prev)
    {
	if (stage->st_rise) direction = "high";
	else direction = "low";
	if (!first)
	    format = " after\n    %s is driven %s at %.2fns";
	else
	{
	    format = "Node %s is driven %s at %.2fns";
	    first = FALSE;
	}
	fprintf(file, format,
	    stage->st_piece2Node[stage->st_piece2Size-1]->n_name,
	    direction, stage->st_time);
	if (DPrintEdgeSpeeds)
	    printf(" with edge speed %.3f ns/volt", stage->st_edgeSpeed);
	for (i=stage->st_piece2Size-2; i>=0; i-=1)
	{
	    PrintFETXY(stage->st_piece2FET[i], &x, &y);
	    fprintf(file, "\n      ...through fet at (%.0f, %.0f) to %s",
		x, y, stage->st_piece2Node[i]->n_name);
	}
	for (i=0; i<stage->st_piece1Size; i+=1)
	{
	    PrintFETXY(stage->st_piece1FET[i], &x, &y);
	    fprintf(file, "\n      ...through fet at (%.0f, %.0f) to %s",
		x, y, stage->st_piece1Node[i]->n_name);
	}
    }
    fprintf(file, "\n");
}


DPClear()

/*---------------------------------------------------------
 *	DPClear is called to reset all the saved delay information.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The delay arrays and thresholds are cleared.
 *---------------------------------------------------------
 */

{
    int i;

    for (i=0; i<MAXPATHS; i+=1)
    {
	putPath((Stage *) NULL, &(anyPaths[i]));
	putPath((Stage *) NULL, &(memPaths[i]));
	putPath((Stage *) NULL, &(watchPaths[i]));
    }
    DPAnyThreshold = 0.0;
    DPMemThreshold = 0.0;
    DPWatchThreshold = 0.0;
}


DPrintCaesar(file, stage)
FILE *file;			/* Where to print Caesar commands. */
Stage *stage;			/* Last stage in path to be printed. */

/*---------------------------------------------------------
 *	This procedure generates Caesar commands to identify
 *	a delay path.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A collection of Caesar commands is generated to highlight
 *	the transistors along the path.
 *---------------------------------------------------------
 */

{
    int nStages, i, totalChunks;
    float x, y;
    Stage *s2;
    char *direction, text[100];

    if (stage == NULL)
    {
	printf("No such critical path.\n");
	return;
    }

    /* First, count up the number of stages in the path so we can
     * number them correctly.
     */
    
    nStages = 0;
    totalChunks = 0;
    for (s2 = stage; s2 != NULL; s2 = s2->st_prev)
    {
	nStages += 1;
	totalChunks += s2->st_piece1Size + s2->st_piece2Size - 1;
    }

    /* Now work backwards through the stage, putting out a label,
     * a splotch of the "error" layer, and a stacked box.
     */
    
    GrBegin(file, totalChunks);
    for ( ; stage != NULL; stage = stage->st_prev)
    {
	nStages -= 1;
	if (stage->st_rise) direction = ",rise";
	else direction = ",fall";
	for (i = stage->st_piece2Size-2; i >= 0; i -= 1)
	{
	    PrintFETXY(stage->st_piece2FET[i], &x, &y);
	    sprintf(text, "[%d]%.1fns%s", nStages,
		stage->st_time, direction);
	    GrPrint(file, x, y, 2.0, 2.0, text);
	}
	for (i = 0; i < stage->st_piece1Size; i += 1)
	{
	    PrintFETXY(stage->st_piece1FET[i], &x, &y);
	    sprintf(text, "[%d]%.1fns%s", nStages,
		stage->st_time, direction);
	    GrPrint(file, x, y, 2.0, 2.0, text);
	}
    }
    GrEnd(file);
}


DPStats()

/*---------------------------------------------------------
 *	This routine prints out statistics recorded by
 *	this file.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Information is printed on standard output.
 *---------------------------------------------------------
 */

{
    printf("Number of calls to DPRecord for arbitrary nodes: %d\n",
	DPnAnyRecords);
    printf("Number of calls to DPRecord for memory nodes: %d.\n",
	DPnMemRecords);
    printf("Number of calls to DPRecord for watched nodes: %d.\n",
	DPnWatchRecords);
    printf("Number of duplicates eliminated: %d\n", DPnDups);
}


Stage *
DPGetCrit(string)
char *string;			/* Name of critical path.  Consists of
				 * a number, followed optionally by an
				 * "m" to signify memory path or a "w"
				 * to signify a watched node.
				 */

/*---------------------------------------------------------
 *	Given a string, this procedure returns the critical path
 *	corresponding to that string.
 *
 *	Results:
 *	NULL is returned if the desired critical path doesn't exist
 *	or if the format is bad.  Otherwise, the return value is
 *	a pointer to the latest stage in the named critical path.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    int index, memory, watched;
    char *p;

    memory = watched = FALSE;
    if (!isdigit(*string)) return (Stage *) NULL;
    for (p = string+1; isdigit(*p); p += 1) /* do nothing */;
    if (*p == 'm') memory = TRUE;
    else if (*p == 'w') watched = TRUE;
    else if ((*p != 0) && !isspace(*p)) return NULL;
    index = atoi(string);

    if (memory)
    {
	if (index > DPNumMemPaths) return NULL;
	else return memPaths[DPNumMemPaths - index];
    }
    else if (watched)
    {
	if (index > DPNumWatchPaths) return NULL;
	else return watchPaths[DPNumWatchPaths - index];
    }
    else
    {
	if (index > DPNumAnyPaths) return NULL;
	else return anyPaths[DPNumAnyPaths - index];
    }
}


DPrint(string)
char *string;			/* Contains parameters. */

/*-----------------------------------------------------------------------------
 *	This procedure prints out the critical path in any of several
 *	forms.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	String is of the form "[-g file] [-s file] [file] [pathnum][m]".
 *	If a pathnum is specified, then the given path number (ranked
 *	from 1 = slowest) is printed.  Adding an "m" to the end of the
 *	number specifies that the path is a memory critical path.  Adding
 *	a "w" to the end of the number indicates that the path is one
 *	to a watched node.  If no pathnum is given, it defuaults to "1".
 *	The -g and -s switches are used to output graphical or Spice
 *	information to files.
 *-----------------------------------------------------------------------------
 */

{
    char *p;
    char *caesarFile, *spiceFile, *textFile;
    FILE *file;
    Stage *crit;

    /* Scan off all of the arguments. */

    textFile = caesarFile = spiceFile = NULL;
    crit = anyPaths[DPNumAnyPaths-1];
    while ((p = NextField(&string)) != NULL)
    {
	if (*p != '-')
	{
	    if (isdigit(*p))
		crit = DPGetCrit(p);
	    else textFile = p;
	    continue;
	}
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
		caesarFile = p;
		break;
		 
	    case 's':
		if (*++p == 0) p = NextField(&string);
		if (p == NULL)
		{
		    printf("No file name given for SPICE deck.\n");
		    break;
		}
		spiceFile = p;
		break;

	    default:
		printf("Unknown switch %c ignored.\n", *p);
		break;
	}
    }

    if (crit == NULL)
    {
	printf("Sorry, but that critical path doesn't exist.\n");
	return;
    }

    /* store the critical paths into the oct cell			*/
    if(( cOctStatus & COCT_ON) && (cOctStatus &  COCT_PENDING_CRIT_UPDATE)){
	cOctStoreCrit( TRUE);
	cOctStatus &= ~(COCT_PENDING_CRIT_UPDATE);
    }

    if ((textFile != NULL) || ((caesarFile == NULL) && (spiceFile == NULL)))
    {
	if (textFile != NULL) file = fopen(textFile, "w");
	else file = stdout;
	if (file == NULL) printf("Couldn't open file.\n");
	else
	{
	    DPrintStage(file, crit);
	    if (textFile != NULL) fclose(file);
	}
    }

    if (caesarFile != NULL)
    {
	file = fopen(caesarFile, "w");
	if (file == NULL) printf("Couldn't open file for graphical output.\n");
	else
	{
	    DPrintCaesar(file, crit);
	    fclose(file);
	}
    }

    if (spiceFile != NULL)
    {
	file = fopen(spiceFile, "w");
	if (file == NULL) printf("Couldn't open file for SPICE commands.\n");
	else
	{
	    SpiceDeck(file, crit);
	    fclose(file);
	}
    }
}


RecomputeCmd()

/*---------------------------------------------------------
 *	This command recomputes all the saved delays using
 *	the current model.  It's used to compare different
 *	models over exactly the same paths.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	All delays in all recorded paths are recomputed.
 *---------------------------------------------------------
 */

{
    int i;
    extern recompute();

    for (i=0; i<DPNumAnyPaths; i++) recompute(anyPaths[i]);
    for (i=0; i<DPNumMemPaths; i++) recompute(memPaths[i]);
    for (i=0; i<DPNumWatchPaths; i++) recompute(watchPaths[i]);
}

recompute(stage)
Stage *stage;
{
    if ((stage == NULL) || (stage->st_prev == NULL)) return;
    else recompute(stage->st_prev);
    ModelDelay(stage);
}


/*---------------------------------------------------------
 *	This page contains utility routines used to
 *	print out information in the dumpcritical command.
 *---------------------------------------------------------
 */

dumpFET(file, fet)
FILE *file;			/* Place to print information. */
FET *fet;			/* Transistor to print out. */
{
    fprintf(file, "        fet %d %o %f %f %f %f\n",
        fet->f_typeIndex, fet->f_flags, fet->f_area,
	fet->f_aspect, fet->f_xloc, fet->f_yloc);
}

dumpNode(file, node)
FILE *file;			/* Place to print information. */
Node *node;			/* Node to print out. */
{
    fprintf(file, "        node %o %s %f %f %f %f\n",
	node->n_flags, node->n_name, node->n_C, node->n_R,
	node->n_hiTime, node->n_loTime);
}

dumpStage(file, stage)
FILE *file;			/* Place to print information. */
Stage *stage;			/* Stage to print out. */
{
    int i;

    if (stage->st_prev != NULL) dumpStage(file, stage->st_prev);
    fprintf(file, "    stage %d %d %d %f %f\n", stage->st_piece1Size,
	stage->st_piece2Size, stage->st_rise, stage->st_time,
	stage->st_edgeSpeed);
    for (i=0; i<stage->st_piece1Size; i+=1)
    {
	dumpFET(file, stage->st_piece1FET[i]);
	dumpNode(file, stage->st_piece1Node[i]);
    }
    for (i=0; i<stage->st_piece2Size; i+=1)
    {
	if (i < stage->st_piece2Size-1)
	    dumpFET(file, stage->st_piece2FET[i]);
	dumpNode(file, stage->st_piece2Node[i]);
    }
}


DumpCmd(string)
char *string;			/* Contains name of file. */

/*---------------------------------------------------------
 *	This procedure dumps out all of the known critical
 *	paths into a file.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	String contains the name of a file.  All critical paths
 *	are dumped into that file, in a format that can be read
 *	back later with the undump command.
 *---------------------------------------------------------
 */

{
    int i;
    FILE *f;
    char *fileName;

    fileName = NextField(&string);
    if (fileName == NULL)
    {
	printf("No file name given.\n");
	return;
    }
    f = fopen(fileName, "w");
    if (f == NULL)
    {
	printf("Couldn't open %s for writing.\n", fileName);
	return;
    }

    for (i=0; i<DPNumAnyPaths; i++)
    {
	if (anyPaths[i] == NULL) continue;
	fprintf(f, "Any %d\n", DPNumAnyPaths - i);
	dumpStage(f, anyPaths[i]);
    }
    for (i=0; i<DPNumMemPaths; i++)
    {
	if (memPaths[i] == NULL) continue;
	fprintf(f, "Memory %d\n", DPNumMemPaths - i);
	dumpStage(f, memPaths[i]);
    }
    for (i=0; i<DPNumWatchPaths; i++)
    {
	if (watchPaths[i] == NULL) continue;
	fprintf(f, "Watched %d\n", DPNumWatchPaths - i);
	dumpStage(f, watchPaths[i]);
    }
    fclose(f);
}


/*---------------------------------------------------------
 *	This page contains utility routines used to
 *	read back in dumped critical paths.
 *---------------------------------------------------------
 */

FET *
undumpFET(file)
FILE *file;				/* File from which to read FET. */
{
    FET *fet;
    int typeIndex, flags;

    fet = (FET *) malloc(sizeof(FET));
    fet->f_source = fet->f_drain = fet->f_gate = NULL;
    fscanf(file, " fet %d %o %f %f %f %f", &typeIndex, &flags,
	&fet->f_area, &fet->f_aspect, &fet->f_xloc, &fet->f_yloc);
    fet->f_typeIndex = typeIndex;
    fet->f_flags = flags;
    return fet;
}

Node *
undumpNode(file)
FILE *file;				/* File from which to read node. */
{
    Node *node;
    char name[100];

    node = (Node *) malloc(sizeof(Node));
    node->n_pointer = NULL;
    node->n_next = NULL;
    fscanf(file, " node %o %s %f %f %f %f", &node->n_flags,
	name, &node->n_C, &node->n_R, &node->n_hiTime,
	&node->n_loTime);
    node->n_name = malloc((unsigned) strlen(name) + 1);
    strcpy(node->n_name, name);
    return node;
}

Stage *
undumpStage(file)
FILE *file;				/* File from which to read stage. */
{
    Stage *stage;
    int i;

    stage = (Stage *) malloc(sizeof(Stage));
    fscanf(file, " %d %d %d %f %f", &stage->st_piece1Size,
	&stage->st_piece2Size, &stage->st_rise,
	&stage->st_time, &stage->st_edgeSpeed);
    for (i=0; i<stage->st_piece1Size; i++)
    {
	stage->st_piece1FET[i] = undumpFET(file);
	stage->st_piece1Node[i] = undumpNode(file);
    }
    for (i=0; i<stage->st_piece2Size; i++)
    {
	if (i < stage->st_piece2Size-1)
	    stage->st_piece2FET[i] = undumpFET(file);
	else stage->st_piece2FET[i] = NULL;
	stage->st_piece2Node[i] = undumpNode(file);
    }
    return stage;
}


UndumpCmd(string)
char *string;				/* Contains file name argument. */

/*---------------------------------------------------------
 *	This routine reads back in critical paths that were
 *	dumped with the dump command.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The file contained in string is read, and critical paths
 *	are regenerated from the information in it.  Note:  the
 *	database is not regenerated, ONLY the critical paths.
 *	This is done in a very sloppy fashion, and should be used
 *	ONLY for testing out new models.
 *---------------------------------------------------------
 */

{
    Stage *prev, *next;
    char symbol[20], *fileName;
    int index, memory, watched, i;
    FILE *file;

    fileName = NextField(&string);
    if (fileName == NULL)
    {
	printf("No file name given.\n");
	return;
    }
    file = fopen(fileName, "r");
    if (file == NULL)
    {
	printf("Couldn't open %s for reading.\n", fileName);
	return;
    }

    for (i=0; i<MAXPATHS; i++)
    {
	anyPaths[i] = NULL;
	memPaths[i] = NULL;
    }

    fscanf(file, "%s", symbol);
    while (TRUE)
    {
	if (fscanf(file, "%d", &index) == EOF) break;
	memory = FALSE;
	watched = FALSE;
	if (strcmp(symbol, "Memory") == 0) memory = TRUE;
	else if (strcmp(symbol, "Watched") == 0) watched = TRUE;

	prev = NULL;
	while (TRUE)
	{
	    if (fscanf(file, "%s", symbol) == EOF) break;
	    if (strcmp(symbol, "stage") != 0) break;
	    next = undumpStage(file);
	    next->st_prev = prev;
	    prev = next;
	}
	if (memory)
	{
	    if (index <= DPNumMemPaths)
		memPaths[DPNumMemPaths - index] = prev;
	}
	else if (watched)
	{
	    if (index <= DPNumWatchPaths)
		watchPaths[DPNumWatchPaths - index] = prev;
	}
	else if (index <= DPNumAnyPaths)
	    anyPaths[DPNumAnyPaths - index] = prev;
    }
}
