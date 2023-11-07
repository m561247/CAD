/* build.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)build.c	2.15 (Berkeley) 10/14/88"
 *
 * This file contains code to read in a .sim file and build up
 * the network structure for Crystal.
 */

#include "crystal.h"
#include "hash.h"

/* Library imports: */

/* Imports from other crystal modules: */

extern char *program_name;	/* Import from main.c */

extern char *PrintFET();
extern float DiffCPerArea, PolyCPerArea;
extern float MetalCPerArea, DiffCPerPerim, DiffR, PolyR, MetalR;
extern float Quad();
extern char *NextField();
extern Type TypeTable[];

/* The following globals are initialized by the BuildNet routine
 * to hold the node names and flow names, and to provide pointers
 * to the Vdd and Ground nodes.
 */

HashTable NodeTable, FlowTable;
Node *VddNode, *GroundNode;
Node *FirstNode = NULL;		/* All nodes are chained together in list.
				 * This is the head of the list.
				 */

char *VddName = "Vdd";
char *GroundName = "GND";

/* The following counters are used to record statistics about the
 * circuit.
 */

int BuildNodeCount = 0;
int BuildFlowCount = 0;
int BuildFETCount = 0;

/* The following variables are used to keep from printing too many
 * messages about negative fet lengths and widths.
 */

int negCount = 0;
int negLimit = 50;


Node *
BuildNode(name)
char *name;		/* Name of a node. */

/*---------------------------------------------------------
 *	BuildNode sees if a given node exists, and makes
 *	a new one if necessary.
 *
 *	Results:
 *	The return value is a pointer to a node of the given
 *	name.
 *
 *	Side Effects:
 *	If the node already existed, then there are no side
 *	effects.  Otherwise, a new node is created.
 *---------------------------------------------------------
 */

{
    Node *n;
    HashEntry *h;

    /* Find the hash entry for the node, then see if the
     * node itself exists.  If not, create a new one.
     */

    h = HashFind(&NodeTable, name);
    n = (Node *) HashGetValue(h);
    if (n != 0) return n;

    n = (Node *) malloc(sizeof(Node));
    n->n_name = h->h_name;
    n->n_C = n->n_R = 0.0;
    n->n_hiTime = n->n_loTime = -1.0;
    n->n_flags = 0;
    n->n_pointer = (Pointer *) NULL;
    n->n_count = 0;

    HashSetValue(h, n);
    n->n_next = FirstNode;
    FirstNode = n;
    BuildNodeCount += 1;
    return n;
}


BuildPointer(n, f)
Node *n;
FET *f;

/*---------------------------------------------------------
 *	This routine links a transistor into the list of
 *	those that connect to a node.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A new Pointer object is created.
 *---------------------------------------------------------
 */

{
    Pointer *p;

    p = (Pointer *) malloc(sizeof(Pointer));
    p->p_fet = f;
    p->p_next = n->n_pointer;
    n->n_pointer = p;
}


Flow *
BuildFlow(name)
char *name;

/*---------------------------------------------------------
 *	This procedure makes sure that a flow indicator by a
 *	given name exists in the flow table.
 *
 *	Results:
 *	The return value is a pointer to the flow.
 *
 *	Side Effects:
 *	A new Flow record is added to the flow hash
 *	table if necessary.
 *---------------------------------------------------------
 */

{
    Flow *flow;
    HashEntry *h;

    /* Make sure there's already a hash table entry for the
     * flow, then allocate the flow itself if there
     * isn't one already.
     */

    h = HashFind(&FlowTable, name);
    flow = (Flow *) HashGetValue(h);
    if (flow == 0)
    {
	flow = (Flow *) malloc(sizeof(Flow));
	flow->fl_name = h->h_name;
	flow->fl_entered = (Node *) NULL;
	flow->fl_left = (Node *) NULL;
	flow->fl_flags = 0;
	HashSetValue(h, flow);
	BuildFlowCount += 1;
    }
    return flow;
}


/*ARGSUSED*/
char *
parseNodeAtt(node, string)
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
parseFETAtt(fet, string)
FET *fet;
char *string;

/*---------------------------------------------------------
 *	AttParseFET parses an attribute string specified for
 *	a transistor.  The attribute string must be of the
 *	form "x=a,b,c,...", where x is "s", "d", or "g" to
 *	indicate that the attribute pertains to the transistor
 *	source, drain or gate, and each of the comma-separated
 *	strings a, b, c, ... is of the form "Crystal:goo" or
 *	"Cr:goo", where	goo is the attribute information we store.
 *
 *	Results:
 *	The return value is NULL if the attribute was parsed
 *	successfully.  Otherwise, an error string is returned.
 *	Attributes not pertaining to Crystal are ignored.
 *
 *	Side Effects:
 *	If the attribute is parsed correctly, transistor flags
 *	may be set, and/or a flow record may be created.
 *---------------------------------------------------------
 */

{
    static char msg[100];
    char *next;
    FPointer *fp;
    Flow *flow;
    int aloc, typeIndex;	/* What the attribute refers to (s, d, g). */

    /* Make sure the attribute has the "x=" stuff at the front. */

    switch (*string)
    {
	case 's': aloc = FP_SOURCE;  break;
	case 'd': aloc = FP_DRAIN; break;
	case 'g': aloc = -1; break;
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

	/* If this attribute is attached to the transistor gate,
	 * it is the name of the transistor's type.  Look up the
	 * name and set the type.
	 */
	
	if (aloc < 0)
	{
	    typeIndex = ModelNameToIndex(string);
	    if (typeIndex < 0) {
		sprintf(msg, "unknown transistor type: %s", string);
		return msg;
	    }
	    else fet->f_typeIndex = typeIndex;
	    continue;
	}

	/* If the attribute is "In" or "Out", just set a flag in
	 * the transistor.  Otherwise, make a flow record for the
	 * name and add it to the list for the transistor.
	 */
	
	if (strcmp(string, "In") == 0)
	{
	    if (aloc == FP_SOURCE) fet->f_flags |= FET_NODRAININFO;
	    else fet->f_flags |= FET_NOSOURCEINFO;
	}
	else if (strcmp(string, "Out") == 0)
	{
	    if (aloc == FP_SOURCE) fet->f_flags |= FET_NOSOURCEINFO;
	    else fet->f_flags |= FET_NODRAININFO;
	}
	else
	{
	    flow = BuildFlow(string);
	    fp = (FPointer *) malloc(sizeof(FPointer));
	    fp->fp_flow = flow;
	    fp->fp_flags = aloc;
	    fp->fp_next = fet->f_fp;
	    fet->f_fp = fp;
	}
    }
    return NULL;
}


int
BuildNet(name)
char *name;		/* Name of file to read. */

/*---------------------------------------------------------
 *	The BuildNet procedure reads a file and constructs
 *	the network.
 *
 *	Results:
 *	The return value is TRUE if everything went OK.  Otherwise,
 *	error messages are output on standard output, and FALSE
 *	is returned.
 *
 *	Side Effects:
 *	The NodeTable and FlowTable are created, and VddNode and
 *	GroundNode are initialized.  The caller should have initialized
 *	NodeTable and FlowTable.
 *---------------------------------------------------------
 */

{
#define LINELENGTH 500
#define NAMELENGTH 100
    FILE *f;
    register FET *fet;
    char line[LINELENGTH], *node1, *node2, *node3, *a1, *a2, *a3;
    char *ptr, *tmp;
    char *errMsg;
    float length, width;
    float xloc, yloc;
    float units, cap, res;
    float ma, mp, da, dp, pa, pp;
    short type;
    Type *typeEntry;
    int lineNumber, result;
    Node *n;
    Pointer *p;
    static gotFile = FALSE;

    if (gotFile)
    {
#ifdef NOT_RESTARTABLE
	printf("Sorry, can't read more than one .sim file per run.\n");
	return FALSE;
#else
	printf ("DEBUG: restarting: %s %s\n", program_name, name);
	execlp (program_name, program_name, name, 0); /* Easy way to restart.*/
#endif
    }
    result = TRUE;
    negCount = 0;
    units = 1.0;
    f = fopen(name, "r");
    if (f == NULL)
    {
	printf("Couldn't open .sim file %s.\n", name);
	return FALSE;
    }

    /* Initialize the global values. */

    VddNode = BuildNode(VddName);
    VddNode->n_flags |= NODEISINPUT | NODE1ALWAYS;
    GroundNode = BuildNode(GroundName);
    GroundNode->n_flags |= NODEISINPUT | NODE0ALWAYS;

    /* Process the file one line at a time.  The first character
     * of each line determines the significance of the line.
     */

    lineNumber = 0;
    while (fgets(line, LINELENGTH, f) != NULL)
    {
	lineNumber += 1;
	switch (line[0])
	{
	    /* Transistor: e/d gate drain source length width x y.
	     * There may be up to three additional attribute strings at the
	     * end of the line.  E means nMOS enhancement, d means
	     * nMOS depletion.
	     */

	    case 'e':
	    case 'd':
	    case 'n':
	    case 'p':

		if (line[0] == 'd') type = FET_NDEP;
		else if (line[0] == 'e') type = FET_NENH;
		else if (line[0] == 'n') type = FET_NCHAN;
		else if (line[0] == 'p') type = FET_PCHAN;

		/* This routine parses the line itself rather than
		 * calling sscanf, because it saves huge amounts of time.
		 */

		ptr = &line[1];
		if ((node1 = NextField(&ptr)) == NULL)
		{
		    err1: printf( "Not enough info for");
		    printf(" transistor on line %d.\n", lineNumber);
		    result = FALSE;
		    continue;
		}
		if ((node2 = NextField(&ptr)) == NULL) goto err1;
		if ((node3 = NextField(&ptr)) == NULL) goto err1;
		if ((tmp = NextField(&ptr)) == NULL) goto err1;
		length = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err1;
		width = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err1;
		xloc = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err1;
		yloc = atof(tmp);
		a1 = NextField(&ptr);
		a2 = NextField(&ptr);
		a3 = NextField(&ptr);

		BuildFETCount += 1;
		fet = (FET *) malloc(sizeof(FET));
		fet->f_gate = BuildNode(node1);
		fet->f_source = BuildNode(node2);
		fet->f_drain = BuildNode(node3);
		
		/* Renormalize distances to micron units. */

		if (length < 0)
		{
		    if (negCount <= negLimit)
		    {
		        printf("Warning: transistor has negative ");
		        printf("length on line %d.\n", lineNumber);
			if (negCount == negLimit)
			{
			    printf("    No more messages of this kind ");
			    printf("will be printed...\n");
			}
			negCount += 1;
		    }
		    length = - length;
		}
		if (width < 0)
		{
		    if (negCount <= negLimit)
		    {
		        printf("Warning: transistor has negative ");
		        printf("width on line %d.\n", lineNumber);
			if (negCount == negLimit)
			{
			    printf("    No more messages of this kind ");
			    printf("will be printed...\n");
			}
			negCount += 1;
		    }
		    width = - width;
		}
		fet->f_area = (length*units) * (width*units);
		fet->f_aspect = length/width;
		fet->f_xloc = xloc*units;
		fet->f_yloc = yloc*units;
		fet->f_typeIndex = type;
		fet->f_flags = 0;
		fet->f_fp = NULL;

		/* See if there is already a transistor of the same type
		 * between the same three nodes.  If so, just lump the
		 * two transistors into a single bigger transistor (this
		 * handles pads with multiple driving transistors in
		 * parallel).  If not, then create pointers to the fet
		 * from each of the nodes it connects to.  Note:  it doesn't
		 * matter which terminal's list we examine, but pick one
		 * other than Vdd or Ground, or this will take a LONG time.
		 */
		
		if ((fet->f_source != VddNode)
		    && (fet->f_source != GroundNode))
		    p = fet->f_source->n_pointer;
		else if ((fet->f_drain != VddNode)
		    && (fet->f_drain != GroundNode))
		    p = fet->f_drain->n_pointer;
		else if ((fet->f_gate != VddNode)
		    && (fet->f_gate != GroundNode))
		    p = fet->f_gate->n_pointer;
		else p = NULL;

		while (p != NULL)
		{
		    FET *f2;
		    f2 = p->p_fet;
		    if ((f2->f_gate == fet->f_gate)
			&& (f2->f_source == fet->f_source)
			&& (f2->f_drain == fet->f_drain)
			&& (f2->f_typeIndex == fet->f_typeIndex))
		    {
			f2->f_area += fet->f_area;
			f2->f_aspect = 1.0/(1.0/fet->f_aspect
			    + 1.0/f2->f_aspect);
			free((char *) fet);
			fet = f2;
			break;
		    }
		    p = p->p_next;
		}
		    
		if (p == NULL)
		{
		    BuildPointer(fet->f_gate, fet);
		    if (fet->f_drain != fet->f_gate)
			BuildPointer(fet->f_drain, fet);
		    if ((fet->f_source != fet->f_gate)
			&& (fet->f_source != fet->f_drain))
			BuildPointer(fet->f_source, fet);
		}

		/* Handle attributes, if any.  It's important to do
		 * this here, because the attributes may change the
		 * type of the transistor.
		 */

		if (a1 != NULL)
		{
		    errMsg = parseFETAtt(fet, a1);
		    if (errMsg != NULL)
		    {
			printf("Error in 1st attribute on line %d: %s.\n",
			    lineNumber, errMsg);
			result = FALSE;
		    }
		}
		if (a2 != NULL)
		{
		    errMsg = parseFETAtt(fet, a2);
		    if (errMsg != NULL)
		    {
			printf("Error in 2nd attribute on line %d: %s.\n",
			    lineNumber, errMsg);
			result = FALSE;
		    }
		}
		if (a3 != NULL)
		{
		    errMsg = parseFETAtt(fet, a3);
		    if (errMsg != NULL)
		    {
			printf("Error in 3rd attribute on line %d: %s.\n",
			    lineNumber, errMsg);
			result = FALSE;
		    }
		}

		/* Figure out which transistors are loads, and mark them
		 * as such.
		 */

		if (fet->f_typeIndex == FET_NDEP)
		{
		    if (fet->f_source == VddNode)
		    {
			fet->f_typeIndex = FET_NLOAD;
			if (fet->f_gate != fet->f_drain)
			    fet->f_typeIndex = FET_NSUPER;
		    }
		    else if (fet->f_drain == VddNode)
		    {
			fet->f_typeIndex = FET_NLOAD;
			if (fet->f_gate != fet->f_source)
			    fet->f_typeIndex = FET_NSUPER;
		    }
		}
		typeEntry = &(TypeTable[fet->f_typeIndex]);
		fet->f_flags |= typeEntry->t_flags;

		/* If the gate doesn't connect to either source or drain,
		 * then add its gate capacitance onto the gate node.
		 * Also add in source and drain overlap capacitances,
		 * if the relevant terminal doesn't connect to the gate.
		 * Be careful to recompute the fet area, because we
		 * may have merged two transistors.
		 */
		
		if ((fet->f_gate != fet->f_source)
		    && (fet->f_gate != fet->f_drain))
		    fet->f_gate->n_C +=
			width*length*units*units*typeEntry->t_cPerArea;
		cap = width*units*typeEntry->t_cPerWidth;
		if (fet->f_drain != fet->f_gate)
		{
		    fet->f_drain->n_C += cap;
		    fet->f_gate->n_C += cap;
		}
		if (fet->f_source != fet->f_gate)
		{
		    fet->f_source->n_C += cap;
		    fet->f_gate->n_C += cap;
		}
		break;

	    /* Node area information:  compute capacitance and resistance. */

	    case 'N':
		/* Once again, parse here rather than calling sscanf.
		 * This saves time.
		 */

		ptr = &line[1];
		if ((node1 = NextField(&ptr)) == NULL)
		{
		    err2: printf("Not enough node info");
		    printf(" on line %d.\n", lineNumber);
		    result = FALSE;
		    continue;
		}
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		da = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		dp = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		pa = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		pp = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		ma = atof(tmp);
		if ((tmp = NextField(&ptr)) == NULL) goto err2;
		mp = atof(tmp);

		n = BuildNode(node1);
		if (da != 0)
		{
		    da *= units*units;
		    dp *= units;
		    n->n_C += da*DiffCPerArea;
		    n->n_C += dp*DiffCPerPerim;
		    n->n_R += DiffR * Quad(da, dp);
		}
		if (pa != 0)
		{
		    pa *= units*units;
		    pp *= units;
		    n->n_C += pa*PolyCPerArea;
		    n->n_R += PolyR * Quad(pa, pp);
		}
		if (ma != 0)
		{
		    ma *= units*units;
		    mp *= units;
		    n->n_C += ma*MetalCPerArea;
		    n->n_R += MetalR * Quad(ma, mp);
		}
		break;

	    /* Capacitance information:  "C node1 node2 femtofarads" */

	    case 'C':
		/* As usual, parse locally to save time. */

		ptr = &line[1];
		if ((node1 = NextField(&ptr)) == NULL)
		{
		    err3: printf("Not enough capacitance info on line %d.\n",
			lineNumber);
		    result = FALSE;
		    continue;
		}
		if ((node2 = NextField(&ptr)) == NULL) goto err3;
		if ((tmp = NextField(&ptr)) == NULL) goto err3;
		cap = atof(tmp);
		cap /= 1000;
		BuildNode(node1)->n_C += cap;
		BuildNode(node2)->n_C += cap;
		break;
		
	    /* Explicit resistance information from the extractor:
	     * "R node ohms".  The amount is added to whatever was
	     * already there for the node.
	     */

	    case 'R':
		/* As usual, parse locally to save time. */

		ptr = &line[1];
		if ((node1 = NextField(&ptr)) == NULL)
		{
		    err4: printf("Not enough resistance info on line %d.\n",
			lineNumber);
		    result = FALSE;
		    continue;
		}
		if ((tmp = NextField(&ptr)) == NULL) goto err4;
		res = atof(tmp);
		BuildNode(node1)->n_R += res;
		break;
		
	    /* Header information with technology and other junk.  Get
	     * unit scale factor. */

	    case '|':
		if (sscanf(line, "| units: %f", &units) != 1)
		{
		    printf("Funny looking header line in .sim file.\n");
		    result = FALSE;
		    units = 1.0;
		}
		units = units/100.0;
	    	break;
	
	    /* Node attribute information: A node attribute */

	    case 'A':
		if (sscanf(&line[1], " %s %s", node1, a1) != 2)
		{
		    printf("Not enough info for attribute on line %d.\n",
			lineNumber);
		    result = FALSE;
		    continue;
		}
		/* errMsg = AttParseNode(BuildNode(node1), a1);
		if (errMsg != NULL)
		{
		    printf("Error in attribute on line %d: %s.\n",
			lineNumber, errMsg);
		    result = FALSE;
		}
		*/
		break;

	    /* An unknown line.  Ignore it. */

	    default: break;
	}
    }

    /* Make sure that there was a Vdd node in the file and a
     * ground node too.
     */
    
    if (VddNode->n_pointer == NULL)
    {
	printf("No Vdd node!  (make sure CIF file has labels)\n");
	result = FALSE;
    }
    if (GroundNode->n_pointer == NULL)
    {
	printf("No GND node!  (make sure CIF file has labels)\n");
	result = FALSE;
    }
    gotFile = TRUE;
    fclose (f);
    return result;
}


AliasCmd(string)
char *string;			/* Command parameters.  Must contain
				 * name of alias file.
				 */

/*---------------------------------------------------------
 *	This procedure reads in an alias file and adds the
 *	aliases to the node name hash table.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	NodeTable is expanded.  From here on, names in the
 *	alias file may be used to refer to nodes.
 *---------------------------------------------------------
 */

{
    FILE *f;
    char line[LINELENGTH], *realName, *aliasName, *ptr, *fileName;
    Node *realNode, *aliasNode;
    HashEntry *h;
    int lineNumber;

    fileName = NextField(&string);
    if (fileName == NULL)
    {
	printf("No alias file name given.\n");
	return;
    }
    f = fopen(fileName, "r");
    if (f == NULL)
    {
	printf("Couldn't open alias file %s.\n", fileName);
	return;
    }

    lineNumber = 0;
    while (fgets(line, LINELENGTH, f) != NULL)
    {
	lineNumber += 1;
	if (line[0] == '\n') continue;
	if (line[0] != '=')
	{
	    printf("Line %d of alias file has bad format; line ignored.\n",
	        lineNumber);
	    continue;
	}
	ptr = &line[1];
	realName = NextField(&ptr);
	if (realName == NULL)
	{
	    printf("Line %d of alias file has no node names.\n",
		lineNumber);
	    continue;
	}

	h = HashFind(&NodeTable, realName);
	realNode = (Node *) HashGetValue(h);
	if (realNode == 0)
	{
	    printf("Node %s doesn't exist! (see line %d)\n",
		realName, lineNumber);
	    continue;
	}

	while ((aliasName = NextField(&ptr)) != NULL)
	{
	    h = HashFind(&NodeTable, aliasName);
	    aliasNode = (Node *) HashGetValue(h);
	    if (aliasNode == 0) HashSetValue(h, realNode);
	    else if (aliasNode != realNode)
	    {
		printf("Node %s is already aliased to %s (see line %d).\n",
		    aliasName, aliasNode->n_name);
		continue;
	    }
	}
    }
    fclose (f);
}


BuildStats()

/*---------------------------------------------------------
 *	Prints out statistics about the circuit.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Statistics saved up while the module executed are
 *	printed on standard output.
 *---------------------------------------------------------
 */

{
    printf("Number of nodes: %d.\n", BuildNodeCount);
    printf("Number of transistors: %d.\n", BuildFETCount);
    printf("Number of distinct flows: %d.\n", BuildFlowCount);
}
