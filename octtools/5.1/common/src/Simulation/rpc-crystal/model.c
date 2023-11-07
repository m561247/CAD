/* model.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)model.c	2.28 (Berkeley) 10/21/84"
 *
 * This file contains routines that provide the overall drivers
 * for the delay models.  The actual models are contained in
 * other files.
 */

#include "crystal.h"

/* Library imports: */

/* Imports from the rest of Crystal: */

extern char *NextField();

/* The following table gives the names of the valid models.
 * After that, the next tables contain procedure addresses
 * for the modelling routines to compute rise and fall times.
 */

static char *(models[]) = {
    "rc      (simple Mead/Conway RC model)",
    "prslope (slope model with Penfield-Rubinstein adjustments)",
    "slope   (based on RC with edge speed adjustments)",
    NULL};

extern RCDelay(), SlopeDelay(), PRSlopeDelay();

int (*(ModelDelayProc[]))() = {
    RCDelay,
    PRSlopeDelay,
    SlopeDelay,
    NULL};

int CurModel = 1;

/* The following table contains all the information about all
 * the various transistor types.  Initially, it describes only
 * the standard system types.
 */

Type TypeTable[MAXTYPES] =
{
    {TYPE_ON1, 2, 3, "nenh", .0004, .00025, 30000, 15000, 2, 'e'},
    {TYPE_ON1, 2, 3, "nenhp", .0004, .00025, 30000, 48000, 2, 'e'},
    {TYPE_ONALWAYS, 2, 3, "ndep", .0004, .00025, 50000, 7000, 2, 'd'},
    {TYPE_ONALWAYS, 2, 0, "nload", 0.0, .00025, 22000, 0, 2, 'd'},
    {TYPE_ONALWAYS, 2, 0, "nsuper", .0004, .00025, 5500, 5500, 2, 'd'},
    {TYPE_ON1, 2, 3, "nchan", .0004, .00025, 60000, 20000, 0, 'n'},
    {TYPE_ON0, 2, 2, "pchan", .0004, .00025, 40000, 60000, 1, 'p'}
};
int curNTypes = 7;

/* Constants that are used in models: */

float DiffCPerArea = .0001;	/* Pf per square micron. */
float PolyCPerArea = .00004;
float MetalCPerArea = .00003;
float DiffCPerPerim = .0001;	/* Pf per micron of perimeter. */

float DiffR = 10;		/* Resistance per square. */
float PolyR = 30;
float MetalR = .03;

float Vdd = 5.0;		/* Supply voltage.  Used to compute
				 * edge speeds for resistors.
				 */
float Vinv = 2.2;		/* Logic threshold voltage.  Used to
				 * compute edge speeds for resistors.
				 */

/* Tables defining the model parameters that are independent
 * of transistor type:
 */

static char *(parmNames[]) = {
    "diffcperarea     (diffusion substrate capacitance, pfs/sq. micron)",
    "diffcperperim    (diffusion sidewall capacitance, pfs/micron)",
    "diffresistance   (diffusion resistance, ohms/square)",
    "metalcperarea    (metal substrate capacitance, pfs/sq. micron)",
    "metalresistance  (metal resistance, ohms/square)",
    "polycperarea     (polysilicon substrate capacitance, pfs/sq. micron)",
    "polyresistance   (polysilicon resistance, ohms/square)",
    "vdd              (logic 1 voltage, used only in slope model)",
    "vinv             (logic threshold voltage, used only in slope model)",
    NULL};
static float *(parmLocs[]) = {
    &DiffCPerArea,
    &DiffCPerPerim,
    &DiffR,
    &MetalCPerArea,
    &MetalR,
    &PolyCPerArea,
    &PolyR,
    &Vdd,
    &Vinv,
    NULL};

/* The following two parameters are multiplicative factors applied
 * to parasitic resistances (i.e. effective resistance = parasitic
 * resistance times factor).  These factors are necessary because
 * the logic threshold isn't Vdd-Vdd/e (for rising transitions) or
 * Vdd/e for falling transitions.  Because of this, RDownFactor is
 * -ln(Vinv/Vdd) and RUpFactor is -ln((Vdd-Vinv)/Vdd).  These two
 * constants are precomputed to save time in the modelling routines.
 */

float RUpFactor, RDownFactor;


ModelInit()

/*---------------------------------------------------------
 *	This routine just initializes the model tables.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Slope model parameters are filled in for the standard
 *	system-defined transistor types.
 *---------------------------------------------------------
 */

{
    Type *t;
    int i, j;

    RUpFactor = -log((Vdd-Vinv)/Vdd);
    RDownFactor = -log(Vinv/Vdd);
    for (j=0; j<=FET_PCHAN; j++)
    {
	t = &(TypeTable[j]);
	for (i=0; i<MAXSLOPEPOINTS; i++)
	{
	    if (i==0)
	    {
		t->t_upESRatio[i] = 0;
		t->t_downESRatio[i] = 0;
	    }
	    else if (i==1)
	    {
		t->t_upESRatio[i] = 1000;
		t->t_downESRatio[i] = 1000;
	    }
	    else
	    {
		t->t_upESRatio[i] = -1;
		t->t_downESRatio[i] = -1;
	    }
	    t->t_upREff[i] = t->t_rUp;
	    t->t_upOutES[i] = t->t_rUp/(Vinv*1000);
	    t->t_downREff[i] = t->t_rDown;
	    t->t_downOutES[i] = t->t_rDown/(Vinv*1000);
	}
    }

    t = &(TypeTable[FET_NENH]);

    t->t_downESRatio[0]=0;	t->t_downREff[0]=16600;	t->t_downOutES[0]=7.2;
    t->t_downESRatio[1]=.091;	t->t_downREff[1]=17700;	t->t_downOutES[1]=7.2;
    t->t_downESRatio[2]=.242;	t->t_downREff[2]=19800;	t->t_downOutES[2]=7.2;
    t->t_downESRatio[3]=.759;	t->t_downREff[3]=27000;	t->t_downOutES[3]=7.7;
    t->t_downESRatio[4]=2.24;	t->t_downREff[4]=41800;	t->t_downOutES[4]=10.8;
    t->t_downESRatio[5]=7.42;	t->t_downREff[5]=70200;	t->t_downOutES[5]=20.9;
    t->t_downESRatio[6]=22.3;	t->t_downREff[6]=106000;
							t->t_downOutES[6]=44.1;
    t->t_downESRatio[7]=74;	t->t_downREff[7]=145000;
							t->t_downOutES[7]=110;
    t->t_downESRatio[8]= -1;

    t->t_upESRatio[0]=0;	t->t_upREff[0]=33300;	t->t_upOutES[0]=48.5;
    t->t_upESRatio[1]=.019;	t->t_upREff[1]=34400;	t->t_upOutES[1]=48.5;
    t->t_upESRatio[2]=.040;	t->t_upREff[2]=36900;	t->t_upOutES[2]=48.4;
    t->t_upESRatio[3]=.117;	t->t_upREff[3]=47100;	t->t_upOutES[3]=49.7;
    t->t_upESRatio[4]=.335;	t->t_upREff[4]=77800;	t->t_upOutES[4]=65.6;
    t->t_upESRatio[5]=1.10;	t->t_upREff[5]=173000;
							t->t_upOutES[5]=140;
    t->t_upESRatio[6]=3.28;	t->t_upREff[6]=415000;
							t->t_upOutES[6]=349;
    t->t_upESRatio[7]=10.9;	t->t_upREff[7]=1190000;
							t->t_upOutES[7]=1060;
    t->t_upESRatio[8]=109;	t->t_upREff[8]=10400000;
							t->t_upOutES[8]=9942;
    t->t_upESRatio[9]= -1;

    t = &(TypeTable[FET_NENHP]);
    t->t_downESRatio[0]=0;	t->t_downREff[0]=48000;	t->t_downOutES[0]=20.4;
    t->t_downESRatio[1]=.065;	t->t_downREff[1]=49300;	t->t_downOutES[1]=20.5;
    t->t_downESRatio[2]=.140;	t->t_downREff[2]=51400;	t->t_downOutES[2]=20.8;
    t->t_downESRatio[3]=.375;	t->t_downREff[3]=56500;	t->t_downOutES[3]=21.9;
    t->t_downESRatio[4]=1.013;	t->t_downREff[4]=65400;	t->t_downOutES[4]=24.4;
    t->t_downESRatio[5]=3.194;	t->t_downREff[5]=78600;	t->t_downOutES[5]=31.5;
    t->t_downESRatio[6]=9.467;	t->t_downREff[6]=80800;	t->t_downOutES[6]=47.5;
    t->t_downESRatio[7]=31.311;	t->t_downREff[7]=2000;	t->t_downOutES[7]=97.9;
    t->t_downESRatio[8]= -1;

    t = &(TypeTable[FET_NLOAD]);
    t->t_upESRatio[0]=0;	t->t_upREff[0]=21900;	t->t_upOutES[0]=13.3;
    t->t_upESRatio[1]=.406;	t->t_upREff[1]=21000;	t->t_upOutES[1]=13.3;
    t->t_upESRatio[2]=1.344;	t->t_upREff[2]=23800;	t->t_upOutES[2]=13.8;
    t->t_upESRatio[3]=4.027;	t->t_upREff[3]=31400;	t->t_upOutES[3]=21;
    t->t_upESRatio[4]=13.4;	t->t_upREff[4]=43300;	t->t_upOutES[4]=44.7;
    t->t_upESRatio[5]=40.3;	t->t_upREff[5]=57800;	t->t_upOutES[5]=107;
    t->t_upESRatio[6]= -1;

    t = &(TypeTable[FET_NSUPER]);
    t->t_upESRatio[0]=0;	t->t_upREff[0]=4800;	t->t_upOutES[0]=3.5;
    t->t_upESRatio[1]=.214;	t->t_upREff[1]=4700;	t->t_upOutES[1]=3.5;
    t->t_upESRatio[2]=.464;	t->t_upREff[2]=5400;	t->t_upOutES[2]=4.2;
    t->t_upESRatio[3]=1.139;	t->t_upREff[3]=6800;	t->t_upOutES[3]=6.7;
    t->t_upESRatio[4]=3.58;	t->t_upREff[4]=8500;	t->t_upOutES[4]=15.8;
    t->t_upESRatio[5]=10.96;	t->t_upREff[5]=8200;	t->t_upOutES[5]=42.9;
    t->t_upESRatio[6]=36.2;	t->t_upREff[6]=1900;	t->t_upOutES[6]=140;
    t->t_upESRatio[7]=108;	t->t_upREff[7]= -17600;	t->t_upOutES[7]=413;
    t->t_upESRatio[8]= -1;

    for (i=0; i<2; i++)
    {
	TypeTable[FET_NLOAD].t_downREff[i] = 0;
	TypeTable[FET_NLOAD].t_downOutES[i] = 0;

	TypeTable[FET_NDEP].t_upREff[i] = 50000;
	TypeTable[FET_NDEP].t_upOutES[i] = 75;
	TypeTable[FET_NDEP].t_downREff[i] = 23000;
	TypeTable[FET_NDEP].t_downOutES[i] = 9.5;

	TypeTable[FET_NSUPER].t_downREff[i] = 5100;
	TypeTable[FET_NSUPER].t_downOutES[i] = 2.5;

	TypeTable[FET_NENHP].t_upREff[i] = 100000;
	TypeTable[FET_NENHP].t_upOutES[i] = 44.0;
    }
}


ModelSet(string)
char *string;			/* Contains name of model to use from now on.*/

/*---------------------------------------------------------
 *	This routine sets a new model.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	If there is nothing in string, then the current
 *	model is printed, and information is printed about
 *	the available models.  Otherwise, if the string
 *	matches a model name, then that model is made
 *	the current one.
 *---------------------------------------------------------
 */

{
    char name[100];
    int index;

    if (string[0] == 0)
    {
	for (index=0; ; index++)
	{
	    if (models[index] == NULL) return;
	    if (index == CurModel) printf("** %s\n", models[index]);
	    else printf("   %s\n", models[index]);
	}
    }

    (void) sscanf(string, "%s", name);
    index = Lookup(name, models);
    if (index == -1)
	printf("Ambiguous model name: %s\n", name);
    else if (index == -2)
	printf("Unknown model name: %s\n", name);
    else CurModel = index;
    return;
}


ModelParm(string)
char *string;			/* Describes parameter and new value. */

/*---------------------------------------------------------
 *	This procedure is used to print out and change model
 *	parameters.  The string contains the name of a parameter
 *	followed by a new (floating-point) value for that
 *	parameter.  Both the name and the value may be omitted.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	If both name and value are supplied, the parameter's value
 *	is changed to the given value.  If just the name is supplied,
 *	then the current value is printed.  If no name is given,
 *	then all the parameters are printed.
 *---------------------------------------------------------
 */

{
    int index;
    char **cptr, *name, *value;
    float **fptr;

    name = NextField(&string);

    /* Handle the special case where we print out all the parameters. */

    if (name == NULL)
    {
	cptr = parmNames;
	fptr = parmLocs;
	while (*cptr != NULL)
	{
	    printf("%s = %f.\n", *cptr, **fptr);
	    cptr++;
	    fptr++;
	}
	return;
    }

    for (; name != NULL; name = NextField(&string))
    {
	/* Make sure that a value is given. */

	value = NextField(&string);
	if (value == NULL)
	{
	    printf("No value given for %s.\n", name);
	    break;
	}

	/* Look up the name in the parameter name table. */

	index = Lookup(name, parmNames);
	if (index == -2)
	{
	    printf("Parameter name not known: %s.\n", name);
	    continue;
	}
	if (index == -1)
	{
	    printf("Parameter name is ambiguous: %s.\n", name);
	    continue;
	}

	/* Printout or reset the value. */

	*(parmLocs[index]) = atof(value);
    }

    /* Recompute the resistance factors, just in case Vdd or Vinv changed. */

    RUpFactor = -log((Vdd-Vinv)/Vdd);
    RDownFactor = -log(Vinv/Vdd);
}


modelGetSlope(pESRatio, pREff, pOutES, maxPoints, fieldValue, pString)
float *pESRatio;		/* Pointer to 1st entry of ESRatio table. */
float *pREff;			/* Pointer to 1st entry of REff table. */
float *pOutES;			/* Pointer to 1st entry of OutES table. */
int maxPoints;			/* Size of the tables. */
char *fieldValue;		/* Current field to process. */
char **pString;			/* String to use to get additional fields. */

/*---------------------------------------------------------
 *	This is a utility routine used to read in the slope
 *	parameter tables.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The indicated parameter tables are filled in with consecutive
 *	fields from fieldValue and then pString.  If there aren't
 *	enough values to fill in the whole tables, the the last supplied
 *	values are duplicated for the remaining space.
 *---------------------------------------------------------
 */

{
    int i;

    for (i=0;  i<maxPoints;  i++)
    {
	if (i != 0)
	{
	    if (isdigit(**pString) || (**pString == '-')
		|| (**pString == '.'))
		fieldValue = NextField(pString);
	    else break;
	}
	*pESRatio = atof(fieldValue);
	if (*pESRatio < 0.0)
	{
	    printf("Can't have negative edge speed ratio %f.\n", *pESRatio);
	    *pESRatio = 0.0;
	}

	if (isdigit(**pString) || (**pString == '-')
	    || (**pString == '.'))
	    fieldValue = NextField(pString);
	else
	{
	    printf("Only one number supplied for last slope table entry.\n");
	    break;
	}
	*pREff = atof(fieldValue);

	if (isdigit(**pString) || (**pString == '-')
	    || (**pString == '.'))
	    fieldValue = NextField(pString);
	else
	{
	    printf("Only two numbers supplied for last slope table entry.\n");
	    break;
	}
	*pOutES = atof(fieldValue);
	if (*pOutES < 0.0)
	{
	    printf("Can't have negative output edge speed %f.\n", *pOutES);
	    *pOutES = 0.0;
	}

	/* Warn about decreasing values for the ratio. */

	if (i > 0)
	{
	    if (*pESRatio <= *(pESRatio-1))
	    {
		printf("Warning: edge speed ratio %f isn't", *pESRatio);
		printf(" monotonically increasing.\n");
		printf("    Interpolation table truncated.\n");
		break;
	    }
	}

	pESRatio++;
	pREff++;
	pOutES++;
    }

    /* Mark the end of the table (if it isn't completely full) with
     * a negative ratio.  If the table has only a single entry, add
     * a second entry so that interpolation and extrapolation will
     * work.
     */
    
    if (i==1)
    {
	*pESRatio = *(pESRatio-1) + 1000;
	*pREff = *(pREff-1);
	*pOutES = *(pOutES-1);
	pESRatio[1] = -1;
    }
    else if (i<maxPoints) *pESRatio = -1;
}


modelPrintSlope(pESRatio, pREff, pOutES, maxPoints)
float *pESRatio;		/* Pointer to 1st entry of ESRatio table. */
float *pREff;			/* Pointer to 1st entry of REff table. */
float *pOutES;			/* Pointer to 1st entry of OutRt table. */
int maxPoints;			/* Maximum number of points to print. */

/*---------------------------------------------------------
 *	This is a utility routine used to print out the values
 *	in slope tables.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The values in the three tables are printed, in a reasonably
 *	pretty fashion, two entries from each table per line.  If
 *	an entry is found that duplicates the previous entry, then
 *	printing stops.
 *---------------------------------------------------------
 */

{
    float t1, t2, t3;
    int i;

    t1 = t2 = t3 = -1;

    for (i=0; i<maxPoints; i++)
    {
	if (*pESRatio <= t1) break;
	t1 = *pESRatio++;
	t2 = *pREff++;
	t3 = *pOutES++;
	printf("    %8.3f, %8.0f, %8.3f;", t1, t2, t3);
	if (i & 1) printf("\n");
    }

    if (i & 1) printf("\n");
}


modelPrintType(type)
Type *type;			/* Type to be printed. */

/*---------------------------------------------------------
 *	This procedure prints out the transistor table
 *	information for a particular transistor type.
 *
 *	Results:	None.
 *
 *	Side Effects:	Stuff gets printed.
 *---------------------------------------------------------
 */

{
    printf("%s: ", type->t_name);
    if (type->t_flags & TYPE_ON0) printf("on=gate0");
    else if (type->t_flags & TYPE_ON1) printf("on=gate1");
    else if (type->t_flags & TYPE_ONALWAYS) printf("on=always");
    printf(" histrength=%d", type->t_strengthHi);
    printf(" lowstrength=%d", type->t_strengthLo);
    printf("\n    cperarea=%.5f cperwidth=%.5f",
	type->t_cPerArea, type->t_cPerWidth);
    printf(" rup=%.0f rdown=%.0f\n", type->t_rUp, type->t_rDown);
    printf("    slopeparmsup= (ratio, rEff, outES)\n");
    modelPrintSlope(type->t_upESRatio, type->t_upREff,
	type->t_upOutES, MAXSLOPEPOINTS);
    printf("    slopeparmsdown= (ratio, rEff, outES)\n");
    modelPrintSlope(type->t_downESRatio, type->t_downREff,
	type->t_downOutES, MAXSLOPEPOINTS);
    printf("    spicebody=%d spicetype=%c\n",
	type->t_spiceBody, type->t_spiceType);
}


TransistorCmd(string)
char *string;			/* Contains arguments to command. */

/*---------------------------------------------------------
 *	This routine implements the transistor command.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The string may contain nothing (in which case information
 *	is printed out about all known transistor types), or else
 *	it contains the name of a transistor type, followed by
 *	zero or more (name value) pairs giving information about
 *	the type.  The corresponding fields of the type are changed
 *	to the given values.  If the type name is not already in
 *	the table, then a new table entry is created.  If a transistor
 *	type is given without field names, then all information is
 *	printed for that specific type.
 *---------------------------------------------------------
 */

{
    char *typeName, *fieldName, *fieldValue;
    int i, index, firstTime;
    Type *type;
    float f;

    /* Table of valid field names: */

    static char *(fieldNames[]) =
    {
	"cperarea",
	"cperwidth",
	"histrength",
	"lowstrength",
	"on",
	"rdown",
	"rup",
	"slopeparmsdown",
	"slopeparmsup",
	"spicebody",
	"spicetype",
	NULL};

    /* See if there is anything in the string.  If not, print
     * out all the types.
     */
    
    typeName = NextField(&string);
    fieldName = NextField(&string);
    if (typeName == NULL)
    {
	for (i=0; i<curNTypes; i+=1) modelPrintType(&TypeTable[i]);
	return;
    }

    /* See if the type name exists. */

    type = NULL;
    for (i=0; i<curNTypes; i+=1)
    {
	if (strcmp(typeName, TypeTable[i].t_name) == 0)
	{
	    type = &(TypeTable[i]);
	    break;
	}
    }

    /* If the line contains only the transistor type name, then
     * print out info for that transistor type.
     */

    if (fieldName == NULL)
    {
	if (type == NULL)
	    printf("Transistor type %s doesn't exist.\n", typeName);
	else modelPrintType(type);
	return;
    }

    if (type == NULL)
    {
	printf("Transistor type %s doesn't exist;  creating new type.\n",
	    typeName);
	if (curNTypes == MAXTYPES)
	{
	    printf("Sorry, no room in table for more transistor types.\n");
	    return;
	}
	type = &(TypeTable[curNTypes]);
	curNTypes += 1;
	type->t_name = malloc(strlen(typeName) + 1);
	(void) strcpy(type->t_name, typeName);
	type->t_flags = TYPE_ON1;
	type->t_strengthHi = type->t_strengthLo = 0;
	type->t_cPerArea = 0;
	type->t_cPerWidth = 0;
	type->t_rUp = 0;
	type->t_rDown = 0;
	for (i=0;  i<MAXSLOPEPOINTS;  i++)
	{
	    type->t_upESRatio[i] = 0;
	    type->t_upREff[i] = 0;
	    type->t_upOutES[i] = 0;
	    type->t_downESRatio[i] = 0;
	    type->t_downREff[i] = 0;
	    type->t_downOutES[i] = 0;
	}
	type->t_spiceBody = 0;
	type->t_spiceType = 'X';
    }

    /* Process (field value) pairs. */

    firstTime = TRUE;
    while (TRUE)
    {
	/* Scan two more fields off the string. */

	if (!firstTime) fieldName = NextField(&string);
	firstTime = FALSE;
	if (fieldName == NULL) break;
	fieldValue = NextField(&string);
	if (fieldValue == NULL)
	{
	    printf("No value given for %s\n", fieldName);
	    break;
	}

	/* Make sure the field name is sensible. */

	index = Lookup(fieldName, fieldNames);
	if (index == -2)
	{
	    printf("Unknown field: %s\n", fieldName);
	    continue;
	}
	if (index == -1)
	{
	    printf("Ambiguous field name: %s\n", fieldName);
	    continue;
	}

	switch (index)
	{
	    case 0:			/* cperarea */
		f = atof(fieldValue);
		if (f < 0.0)
		    printf("Capacitance can't be negative.\n");
		else type->t_cPerArea = f;
		break;
	    
	    case 1:			/* cperwidth */
		f = atof(fieldValue);
		if (f < 0.0)
		    printf("Capacitance can't be negative.\n");
		type->t_cPerWidth = f;
		break;
	    
	    case 2:			/* histrength */
		i = atoi(fieldValue);
		if (i < 0)
		    printf("Strength can't be negative.\n");
		else type->t_strengthHi = i;
		break;
	    
	    case 3:			/* lostrength */
		i = atoi(fieldValue);
		if (i < 0)
		    printf("Strength can't be negative.\n");
		else type->t_strengthLo = i;
		break;
	    
	    case 4:			/* on */
		if (strcmp(fieldValue, "gate1") == 0)
		    type->t_flags = TYPE_ON1;
		else if (strcmp(fieldValue, "gate0") == 0)
		    type->t_flags = TYPE_ON0;
		else if (strcmp(fieldValue, "always") == 0)
		    type->t_flags = TYPE_ONALWAYS;
		else printf("Unknown \"on\" condition: %s\n", fieldValue);
		break;
	    
	    case 5:			/* rdown */
		f = atof(fieldValue);
		if (f < 0.0)
		    printf("Resistance can't be negative.\n");
		else type->t_rDown = f;
		break;
	    
	    case 6:			/* rup */
		f = atof(fieldValue);
		if (f < 0.0)
		    printf("Resistance can't be negative.\n");
		else type->t_rUp = f;
		break;
	    
	    case 7:			/* slopeparmsdown */
		modelGetSlope(&(type->t_downESRatio[0]),
		    &(type->t_downREff[0]), &(type->t_downOutES[0]),
		    MAXSLOPEPOINTS, fieldValue, &string);
		break;

	    case 8:			/* slopeparmsup */
		modelGetSlope(&(type->t_upESRatio[0]), &(type->t_upREff[0]),
		    &(type->t_upOutES[0]), MAXSLOPEPOINTS,
		    fieldValue, &string);
		break;
	    
	    case 9:			/* spicebody */
		i = atoi(fieldValue);
		if ((i<0) || (i>3))
		    printf("Body voltage source must be 0-3.\n");
		else type->t_spiceBody = i;
		break;
	    
	    case 10:			/* spicetype */
		if (fieldValue[1] != 0)
		    printf("Only first character of type will be used.\n");
		type->t_spiceType = fieldValue[0];
		break;
	}
    }
}


int
ModelNameToIndex(name)
char *name;			/* Name of a transistor type. */

/*---------------------------------------------------------
 *	This routine translates a text transistor type name
 *	into the numeric type table index used internally by
 *	Crystal.
 *
 *	Results:
 *	If the name is found in the type table, the return
 *	value is the transistor's index.  If the name isn't
 *	found, then -1 is returned.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    int i;

    for (i=0; i<curNTypes; i+=1)
	if (strcmp(name, TypeTable[i].t_name) == 0) return i;
    return -1;
}
