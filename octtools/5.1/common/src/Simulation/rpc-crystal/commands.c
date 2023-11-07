/* commands.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)commands.c	2.36 (Berkeley) 10/14/88"
 *
 * This file contains routines to read in the Crystal script and
 * process the commands it contains.  The script is read from
 * standard input.  Each line contains a command name followed
 * by additional parameters, if necessary.
 */

#include "crystal.h"
#include "hash.h"

/* Imports from other Crystal modules: */

extern char *RunStats(), *PrintFET(), *ExpandNext(), *NextField();
extern char *RunStatsSince();
extern HashTable NodeTable;
extern Type TypeTable[];
extern int DelayPrint, DelayPrintAll, MarkPrint, MarkPrintAll;
extern int FlowPrint, FlowPrintAll;
extern int DPrintEdgeSpeeds, RatioLimit, RatioTotalLimit;
extern int MarkPrintDynamic, DelayLimit;
extern float DelayBusThreshold, PrintUnits;
extern int DPMaxPaths, DPNumMemPaths, DPNumAnyPaths, DPNumWatchPaths;
extern Node *VddNode, *GroundNode;
extern char *GrGetEditor();

/* The following two arrays hold the names of the commands, and
 * the routine to be invoked to process each command.
 */

char *(cmds[]) = {
    "alias file",
    "block node node ...",
    "build file",
    "bus node node ...",
    "capacitance pfs node node ...",
    "check",
    "clear",
    "critical [file] [-g file] [-s spicefile] [number][m]",
    "delay node risetime falltime",
    "dump file",
    "fillin time/edgeSpeed inFile outFile keyword path path ...",
    "flow in/out/ignore/off/normal attribute attribute ...",
    "help",
    "inputs node node ...",
    "markdynamic node value node value ...",
    "model name",
    "options [name [value]] [name [value]] ...",
    "outputs node node ...",
    "parameter [name] [value]",
    "prcapacitance [-g file] [-t threshold(pf)] node node ...",
    "precharged node node ...",
    "predischarged node node ...",
    "prfets node node ...",
    "prnodes node node ...",
    "prresistance [-g file] [-t threshold(ohms)] node node ...",
    "quit",
    "ratio [limit value] [limit value] ...",
    "recompute",
    "resistance ohms node node ...",
    "set 0/1 node node ...",
    "source file",
    "statistics",
    "totalcapacitance",
    "transistor [name [field value] [field value] ...]",
    "undump file",
    "watch node node ...",
    NULL};

/* Compiler barfs if command procedures aren't declared here. */

extern MarkFromString(), MarkInput(), MarkBus(),
    Help(), Unused(), Classify(), DelaySetFromString(),
    Source(), DPrint(), ModelSet(), ClearCmd(),
    PrintCap(), OptionCmd(), BuildCmd(), RatioCmd(),
    MarkPrecharged(), PrintNodes (), PrintRes(), SetRes(), MarkOutput(),
    SetCap(), FlowCmd(), MarkBlock(), MarkDynamicCmd(), ModelParm(),
    SpiceCmd(), MarkFlow(), Check(), Stats(), TotalCap(), TransistorCmd(),
    QuitCmd(), PrFETsCmd(), RecomputeCmd(), AliasCmd(),
    DumpCmd(), UndumpCmd(), FillinCmd(), MarkPredischarged(),
    MarkWatched();

int (*(rtns[]))() = {
    AliasCmd,			/* alias */
    MarkBlock,			/* block */
    BuildCmd,			/* build */
    MarkBus,			/* bus */
    SetCap,			/* capacitance */
    Check,			/* check */
    ClearCmd,			/* clear */
    DPrint,			/* critical */
    DelaySetFromString, 	/* delay */
    DumpCmd,			/* dump */
    FillinCmd,			/* fillin */
    FlowCmd,			/* flow */
    Help,			/* help */
    MarkInput,			/* inputs */
    MarkDynamicCmd,		/* markdynamic */
    ModelSet,			/* model */
    OptionCmd,			/* options */
    MarkOutput,			/* output */
    ModelParm,			/* parameter */
    PrintCap,			/* prcap */
    MarkPrecharged,		/* precharged */
    MarkPredischarged,		/* predischarged */
    PrFETsCmd,			/* prfets */
    PrintNodes,			/* prnodes */
    PrintRes,			/* prresistance */
    QuitCmd,			/* quit */
    RatioCmd,			/* ratio */
    RecomputeCmd,		/* recompute */
    SetRes,			/* resistance */
    MarkFromString,		/* set */
    Source,			/* source */
    Stats,			/* statistics */
    TotalCap,			/* total capacitance */
    TransistorCmd,		/* transistor */
    UndumpCmd,			/* undump */
    MarkWatched,		/* watch */
    NULL};


/* The following table defines the class of each command, so that we
 * can make sure that commands are issued in the right order.
 */

#define CLASS_MODEL 0
#define CLASS_AFTERCLEAR 1
#define CLASS_CIRCUIT 2
#define CLASS_DYNAMIC 3
#define CLASS_CHECK 4
#define CLASS_SETUP 5
#define CLASS_DELAY 6
#define CLASS_CLEAR 7
#define CLASS_ANYTIME 8

int curClass = 0;		/* Used to detect out-of-order commands. */
int cmdMarked = FALSE;		/* TRUE means flow and power rails have been
				 * marked, FALSE means we have to do it at
				 * some point.
				 */

char *(ClassName[]) = {		/* Printable names for classes. */
    "model",
    "clear",
    "circuit",
    "markdynamic",
    "check",
    "setup",
    "delay",
    "clear",
    "miscellaneous",
    NULL};

int CmdClass[] = {
    CLASS_ANYTIME,		/* alias */
    CLASS_CIRCUIT,		/* block */
    CLASS_CIRCUIT,		/* build */
    CLASS_CIRCUIT,		/* bus */
    CLASS_CIRCUIT,		/* capacitance */
    CLASS_CHECK,		/* check */
    CLASS_CLEAR,		/* clear */
    CLASS_ANYTIME,		/* critical */
    CLASS_DELAY,	 	/* delay */
    CLASS_ANYTIME,		/* dump */
    CLASS_ANYTIME,		/* fillin */
    CLASS_SETUP,		/* flow */
    CLASS_ANYTIME,		/* help */
    CLASS_CIRCUIT,		/* inputs */
    CLASS_DYNAMIC,		/* markdynamic */
    CLASS_MODEL,		/* model */
    CLASS_ANYTIME,		/* options */
    CLASS_CIRCUIT,		/* output */
    CLASS_MODEL,		/* parameter */
    CLASS_ANYTIME,		/* prcap */
    CLASS_SETUP,		/* precharged */
    CLASS_SETUP,		/* predischarged */
    CLASS_ANYTIME,		/* prfets */
    CLASS_ANYTIME,		/* prnodes */
    CLASS_ANYTIME,		/* prresistance */
    CLASS_ANYTIME,		/* quit */
    CLASS_CHECK,		/* ratio */
    CLASS_ANYTIME,		/* recompute */
    CLASS_CIRCUIT,		/* resistance */
    CLASS_SETUP,		/* set */
    CLASS_ANYTIME,		/* source */
    CLASS_ANYTIME,		/* statistics */
    CLASS_ANYTIME,		/* total capacitance*/
    CLASS_MODEL,		/* transistor */
    CLASS_ANYTIME,		/* undump */
    CLASS_ANYTIME,		/* watch */
    0};


CmdDo(line)
char *line;		/* A command to be processed. */

/*---------------------------------------------------------
 *	This procedure processes a single command.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Depends on command.  Line is a single command plus
 *	all of its arguments, without the newline.
 *---------------------------------------------------------
 */

{
    int class, index, nargs;
    char cmd[100], residue[200];

#ifdef OLD_SSCANF_FORMAT
    nargs = sscanf(line, " %99s %[^]", cmd, residue);
#else
#ifdef HARD_WAY
    /* Do it the hard way until SUN3.2 is fixed.*/
    char *i, *c, *r;

    c = cmd; r=residue;
    i = line;
    while((*i!='\0') && ((*i==' ') || (*i=='\t'))) i++;	/* Skip pre-white.*/
    while((*i!='\0') && (*i!=' ') && (*i!='\t')) {
	*c = *i;	/* Copy the cmd.*/
	c++; i++;
    }
    *c = '\0';
    while((*i!='\0') && ((*i==' ') || (*i=='\t'))) i++;	/* Skip white.*/
    while(*i!='\0') {
	*r = *i;	/* Copy the residue.*/
	r++; i++;
    }
    *r = '\0';
    nargs = 2;
    if (r == residue) nargs=1;
    if (c == cmd) nargs=0;
#else
    nargs = sscanf(line, " %99s %[^\n]", cmd, residue);
#endif
#endif
    if (nargs <= 0) return;
    if (cmd[0] == '!') return;
    if (nargs == 1) residue[0] = 0;
    index = Lookup(cmd, cmds);
    if (index == -1)
	printf("Ambiguous command: %s\n", cmd);
    else if (index == -2)
	printf("Unknown command: %s\n", cmd);
    else
    {
	/* Make sure the command is being given in the right order,
	 * and take of automatic flow and Vdd/Ground marking.
	 */
	
	class = CmdClass[index];
	if (class < curClass)
	{
	    printf("Warning: %s shouldn't be", cmd);
	    printf(" invoked after a %s command.\n", ClassName[curClass]);
	}

	switch (class)
	{
	    case CLASS_ANYTIME: break;

	    case CLASS_MODEL:
	    case CLASS_CIRCUIT:
		if (class > curClass) curClass = class;
		break;
	    
	    case CLASS_DYNAMIC:
	    case CLASS_CHECK:
	    case CLASS_SETUP:
	    case CLASS_DELAY:
		if (!cmdMarked)
		{
		    printf("Marking transistor flow...\n");
		    MarkFlow();
		    printf("Setting Vdd to 1...\n");
		    MarkNodeLevel(VddNode, 1, TRUE);
		    printf("Setting GND to 0...\n");
		    MarkNodeLevel(GroundNode, 0, TRUE);
		    cmdMarked = TRUE;
		}
		if (class > curClass) curClass = class;
		break;
		
	    case CLASS_CLEAR:
		cmdMarked = FALSE;
		curClass = CLASS_AFTERCLEAR;
		break;
	}
	(void) fflush(stdout);
	if (rtns[index] != NULL) (*(rtns[index]))(residue);
	printf("%s\n", RunStatsSince());
    }
}


Command(f)
FILE *f;		/* File descriptor from which to read. */

/*---------------------------------------------------------
 *	Command just reads commands from the script, and
 *	calls the corresponding routines.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Whatever the commands do.  This procedure continues
 *	until it hits end-of-file.
 *---------------------------------------------------------
 */

{
    char line[800], *p;
    int tty, length;

    tty = isatty(fileno(f));
    while (TRUE)
    {
	if (tty)
	{
	    fputs(": ", stdout);
	    (void) fflush(stdout);
	}
	p = line;
	*p = 0;
	length = 799;
	while (TRUE)
	{
	    if (fgets(p, length, f) == NULL) return;

	    /* Get rid of the stupid newline character.  If we didn't
	     * get a whole line, keep reading.
	     */

	    for ( ; (*p != 0) && (*p != '\n'); p++);
	    if (*p == '\n')
	    {
		*p = 0;
		if (p == line) break;
		if (*(p-1) != '\\') break;
		else p--;
	    }
	    length = 799 - strlen(line);
	    if (length <= 0) break;
	}
	if (line[0] == 0) continue;
	if (!tty) printf(": %s\n", line);
	CmdDo(line);
    }
}


Help()

/*---------------------------------------------------------
 *	This command routine just prints out the legal commands.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Valid command names are printed one per line on standard
 *	output.
 *---------------------------------------------------------
 */

{
    char **p;

    p = cmds;
    printf("Valid commands are:\n");
    while (*p != NULL)
    {
	printf("  %s\n", *p);
	p++;
    }
}


OptionCmd(string)
char *string;			/* List of options to set. */

/*---------------------------------------------------------
 *	This command routine sets Crystal internal options.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The string contains one or more options and/or settings.
 *	The indicated internal options are modified.
 *---------------------------------------------------------
 */

{
    char *p;
    int index, i;
    float f;
    int anyOptions = FALSE;
    static char *(optionTable[]) = {
	"bus",
	"graphics",
	"limit",
	"mempaths",
	"noprintedgespeeds",
	"noseedelays",
	"noseeflows",
	"noseedynamic",
	"noseesettings",
	"paths",
	"printedgespeeds",
	"ratiodups",
	"ratiolimit",
	"seealldelays",
	"seeallflows",
	"seeallsettings",
	"seedelays",
	"seeflows",
	"seedynamic",
	"seesettings",
	"units",
	"watchpaths",
	NULL};

    for (p = NextField(&string); p != NULL; p = NextField(&string))
    {
	anyOptions = TRUE;
	index = Lookup(p, optionTable);
	if (index == -1)
	{
	    printf("Ambiguous option name: %s\n", p);
	    continue;
	}
	else if (index == -2)
	{
	    printf("Unknown option name: %s\n", p);
	    continue;
	}
	switch (index)
	{
	    case 0:			/* bus threshold */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%f", &f) == 1))
		    DelayBusThreshold = f;
		else printf("No bus threshold value given\n");
		break;
	    
	    case 1:			/* graphical format */
		p = NextField(&string);
		if (p != NULL) GrSetEditor(p);
		else printf("No graphical style given\n");
		break;
	    
	    case 2:			/* delay limit */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		    DelayLimit = i;
		else printf("No delay limit value given\n");
		break;
	    
	    case 3:			/* mempaths */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		{
		    if ((i > 0) && (i <= DPMaxPaths)) DPNumMemPaths = i;
		    else printf("Bad memory path count.\n");
		}
		else printf("No memory path count given\n");
		break;
	    
	    case 4:			/* noprintedgespeeds*/
		DPrintEdgeSpeeds = FALSE;
		break;
	    
	    case 5:			/* noseedelays */
		DelayPrintAll = FALSE;
		DelayPrint = FALSE;
		break;
	    
	    case 6:			/* noseedynamic */
		MarkPrintDynamic = FALSE;
		break;
	    
	    case 7:			/* noseeflow */
		FlowPrintAll = FALSE;
		FlowPrint = FALSE;
		break;
	    
	    case 8:			/* noseesettings */
		MarkPrintAll = FALSE;
		MarkPrint = FALSE;
		break;

	    case 9:			/* paths */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		{
		    if ((i > 0) && (i <= DPMaxPaths)) DPNumAnyPaths = i;
		    else printf("Bad path count.\n");
		}
		else printf("No path count given\n");
		break;
	    
	    case 10:			/* printedgespeeds*/
		DPrintEdgeSpeeds = TRUE;
		break;
	    
	    case 11:			/* ratiodups */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		{
		    if (i > 0) RatioLimit = i;
		    else printf("Bad ratio duplicate limit.\n");
		}
		else printf("No ratio duplicate limit given.\n");
		break;
	    
	    case 12:			/* ratiolimit */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		{
		    if (i > 0) RatioTotalLimit = i;
		    else printf("Bad ratio error limit.\n");
		}
		else printf("No ratio error limit given.\n");
		break;
	    
	    case 13:			/* see all delays */
		DelayPrintAll = TRUE;
		DelayPrint = TRUE;
		break;

	    case 14:			/* see all flows */
		FlowPrintAll = TRUE;
		FlowPrint = TRUE;
		break;

	    case 15:			/* see all settings */
		MarkPrintAll = TRUE;
		MarkPrint = TRUE;
		break;
	    
	    case 16:			/* see delays for named nodes. */
		DelayPrint = TRUE;
		break;
	    
	    case 17:			/* see flows for named nodes. */
		FlowPrint = TRUE;
		break;

	    case 18:			/* see all nodes marked dynamic. */
		MarkPrintDynamic = TRUE;
		break;
	    
	    case 19:			/* see settings for named nodes. */
		MarkPrint = TRUE;
		break;
	    
	    case 20:			/* set units for printout. */
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%f", &f) == 1))
		    PrintUnits = f;
		else printf("No print units value given\n");
		break;
	    
	    case 21:			/* number of watched paths to record.*/
		p = NextField(&string);
		if ((p != NULL) && (sscanf(p, "%d", &i) == 1))
		{
		    if ((i > 0) && (i <= DPMaxPaths)) DPNumWatchPaths = i;
		    else printf("Bad path count.\n");
		}
		else printf("No path count given.\n");
		break;
	}
    }

    if (!anyOptions)
    {
	printf("Bus threshold is %.1f pf\n", DelayBusThreshold);
	printf("Current graphical format is %s\n", GrGetEditor());
	printf("Delay limit is %d stages\n", DelayLimit);
	printf("Print units are %.1f microns per lambda\n", PrintUnits);
	printf("Recording %d arbitrary paths, %d memory paths",
	    DPNumAnyPaths, DPNumMemPaths);
	printf(", %d watched paths\n", DPNumWatchPaths);
	printf("Ratio duplicate limit is %d, total error limit is %d\n",
	    RatioLimit, RatioTotalLimit);
	if (DelayPrintAll)
	    printf("Printing all new delays to nodes\n");
	else if (DelayPrint)
	    printf("Printing new delays to named nodes\n");
	else printf("Not printing new delays\n");
	if (MarkPrintDynamic)
	    printf("Printing all dynamic nodes as they are found\n");
	else printf("Not printing dynamic nodes\n");
	if (FlowPrintAll)
	    printf("Printing node flow directions for all nodes\n");
	else if (FlowPrint)
	    printf("Printing node flow directions for named nodes\n");
	else printf("Not printing node flow direction settings\n");
	if (MarkPrintAll)
	    printf("Printing node settings for all nodes\n");
	else if (MarkPrint)
	    printf("Printing node settings for named nodes\n");
	else printf("Not printing node settings\n");
	if (DPrintEdgeSpeeds)
	    printf("Printing edge speeds\n");
	else printf("Not printing edge speeds\n");
    }
}


MarkDynamicCmd(string)
char *string;			/* Contains parameters for command. */

/*---------------------------------------------------------
 *	This procedure is the main command routine for
 *	markdynamic.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Some nodes get marked as dynamic memory.  The string contains
 *	"node 0/1 node 0/1...." pairs.  The indicated nodes are set
 *	to the given values, the nodes that are thereby isolated
 *	are marked as dynamic, and then the clear routine is invoked
 *	to remove the 0/1 settings.
 *---------------------------------------------------------
 */

{
    Node *node;
    char *arg, *arg2;
    int val;

    while (TRUE)
    {
	arg = NextField(&string);
	if (arg == NULL) break;
	arg2 = NextField(&string);
	if (arg2 == NULL)
	{
	    printf("No value given for %s.\n", arg);
	    break;
	}
	val = atoi(arg2);
	if (((val != 0) && (val != 1)) || !isdigit(*arg2))
	{
	    printf("Bad value given for %s: %s.  Try 0 or 1.\n",
		arg, arg2);
	    continue;
	}
	ExpandInit(arg);
	while ((node = (Node *) ExpandNext(&NodeTable)) != (Node *) NULL)
	    MarkNodeLevel(node, val, TRUE);
    }

    MarkDynamic();
    ClearCmd();
    cmdMarked = FALSE;
}

QuitCmd()

/*---------------------------------------------------------
 *	This routine just returns from Crystal to the shell.
 *
 *	Results:	None -- it never returns.
 *
 *	Side Effects:	Crystal exits.
 *---------------------------------------------------------
 */

{
    printf("%s Crystal done.\n", RunStats());
    exit(0);
}
