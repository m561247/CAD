/* main.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)main.c	2.12 (Berkeley) 10/14/88"
 *
 * This file contains the main program for Crystal, a VLSI timing
 * analysis program.
 */

#include "hash.h"
#include "crystal.h"
#include "cOct.h"
#include "vov.h"

extern char *RunStats();
extern char *PrintFET(), *PrintNode();
extern float PrintUnits;
extern int MarkPrintAll, MarkPrint;
extern int MarkPrintDynamic;
extern int DelayLimit, DelayPrint, DelayPrintAll;
extern float DelayBusThreshold;
extern Node *BuildNode(), *VddNode, *GroundNode;
extern HashTable NodeTable, FlowTable;
extern char *VddName, *GroundName;

/* oct reader parameter							*/
extern unsigned cOctStatus;
extern int cOctExtTermCap;

/* The following integer is a limit of how many times a diagnostic
 * message gets printed with the same value (of resistance, delay, etc.)
 * This is to eliminate duplicate messages for different instances of
 * the same thing.
 */

int RepeatLimit = 1;

int argc;
char **argv;
char *program_name;	/* Save program name for build.c */

char
*getParm()

/*---------------------------------------------------------
 *	This procedure provides the parameter to the
 *	current command line switch.
 *
 *	Results:
 *	The return value is a pointer to paramter information.
 *	Switch parameters may be of two forms:  "-x parm" or
 *	"-xparm".  This routine finds the parameter, no matter
 *	which form is used.  If we are at the end of the argument
 *	information, then the returned string is empty.
 *
 *	Side Effects:
 *	Argv and argc are advanced as necessary.  On call, argv
 *	is assumed to point to a pointer to the switch value,
 *	which is assumed to be non-null.
 *---------------------------------------------------------
 */

{
    (*argv) += 1;
    if ((**argv == 0) && (--argc > 0)) argv++;
    return *argv;
}


main(argc, argv)
int argc;
char **argv;

/*---------------------------------------------------------
 *	This is the main program for Crystal.
 *
 *	Results:	None.
 *	Side Effects:	Lots.
 *---------------------------------------------------------
 */

{
    char *simFile, cmd[100];
    int i;

    /* Initialize the model tables. */

    ModelInit();

    /* There should be no more than one command line switch, and
     * it contains the name of the .sim file.
     */

    program_name = argv[0];	/* Save the program name for build.c */

    cOctStatus = 0;
    /* usage: crystal [file]
     *	      crystal -oct [-v] [-t] [facet]
     */

    simFile = (char *) 0;

    /* parse the command line						*/
    for( i = 1; i < argc; ++i){
	/* file/facet name must be last argument			*/
	if( simFile != (char *) 0){
	    printProgUsage();
	}

	if( !(strcmp(argv[i], "-oct"))){
	    cOctStatus |= COCT_ON;
	} else if( !strcmp( argv[i], "-v")){
	    cOctStatus |= COCT_VERBOSE;
	} else if( !strcmp( argv[i], "-t")){
	    cOctStatus |= COCT_TRACE;
	} else if( !strcmp( argv[i], "-T")){
	    cOctExtTermCap = FALSE;
	} else {
	    simFile = argv[i];
	}
    }

    VOVbegin( argc, argv );

    HashInit(&NodeTable, 1000);
    HashInit(&FlowTable, 10);
    if( !( cOctStatus & COCT_ON)){
	VddNode = BuildNode(VddName);
	GroundNode = BuildNode(GroundName);
    } else {
	VddNode = GroundNode = (Node *) 0;
    }

    printf("Crystal, v.4\n");
    if (simFile != (char *) 0)
    {
	printf(": build %s\n", simFile);
	sprintf(cmd, "build %s", simFile);
	CmdDo(cmd);
    }
    else ("No .sim file specified.\n");

    Command(stdin);

    printf("%s Crystal done.\n", RunStats());
    VOVend( 0 );
}


classifyNodes(threshold)
int threshold;

/*---------------------------------------------------------
 *	This utility routine classifies the nodes by how
 *	many connections they have.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Statistical information is printed about how many
 *	nodes have one connection, how many have two, and
 *	so on up to ten.  The connection count for the busiest
 *	node is also printed, as is the total number of nodes.
 *	All nodes with more than threshold connections are
 *	printed explicitly.
 *---------------------------------------------------------
 */

{
    HashSearch hs;
    HashEntry *h;
    Node *n;
    Pointer *p;
    int count[10], max, overflow;
    int i, totalNodes;

    HashStartSearch(&hs);
    for (i=0; i<10; i++) count[i] = 0;
    max = 0;
    overflow = 0;
    totalNodes = 0;
    while ((h = HashNext(&NodeTable, &hs)) != NULL)
    {
	n = (Node *) HashGetValue(h);
	if (n == 0) continue;
	totalNodes += 1;
	i = 0;
	p = n->n_pointer;
	while (p != NULL)
	{
	    i += 1;
	    p = p->p_next;
	}
	if (i < 10) count[i] += 1;
	else overflow += 1;
	if (i > max) max = i;
	if (i >= threshold)
	    printf("Node %s has %d connections.\n", n->n_name, i);
    }

    printf("Total number of nodes is %d.\n", totalNodes);
    printf("Busiest node has %d connections.\n", max);
    printf("%d nodes have 10 or more connections.\n", overflow);
    for (i=0; i<10; i++)
	printf("%d nodes have %d connections.\n", count[i], i);
}

printProgUsage()

/*---------------------------------------------------------
 *	This routine prints out the legal usage for crystal.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	    Print message and exit program.
 *---------------------------------------------------------
 */

{
    fprintf( stderr, "usage: %s [simfile]\nor   : %s -oct [-v] [-t] [facet]\n",
	program_name, program_name);
    exit( -1 );
}
