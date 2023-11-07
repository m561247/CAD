/* mkcp.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)mkcp.c	1.12 (Berkeley) 1/29/84"
 *
 * This file contains the entire mkcp program.  Mkcp runs SPICE
 * in order to generate Crystal parameters for the slope model.
 */

#include "port.h"

/* Library imports: */

extern char *NextField();
extern char *mktemp();


main(argc, argv)
int argc;
char **argv;

/*---------------------------------------------------------
 *	This is the main program for mkcp.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Lots.  See the man page for details.
 *
 *	Overall Structure:
 *	The basic idea of this program is that it runs SPICE
 *	many times with a different value of the c1 capacitance.
 *	After each SPICE run it reads in SPICE's results and
 *	records the delay time for one stage and the input and
 *	output slopes for the stage.  This information is then
 *	output in the format Crystal likes for parameter information.
 *---------------------------------------------------------
 */

{
#define MAXCAPS 20
    double cap[MAXCAPS];	/* Different c1 values to try. */
    int nCaps;			/* Number of different c1 values given. */
    double c1;			/* Current c1 value in pfs. */

    char *spiceDeckName;	/* Name of SPICE input file. */
    char *spiceResultsName;	/* Name of SPICE output file. */
    FILE *f;

#define MAXLINES 200
    char *(lines[MAXLINES]);	/* Internal storage for template SPICE deck. */
    int nLines;			/* Actual number of lines. */
#define MAXLINESIZE 200
    char curLine[MAXLINESIZE];	/* Temporary buffer for lines. */
    int gotc1, gotc2, gotTran;	/* Used to remember whether or not certain
				 * lines have been seen in the SPICE deck.
				 */
    double extraTime = 2.0;	/* A scale factor determining how much
				 * simulation time to allow in a run.  If
				 * one run doesn't complete, this number
				 * gets increased.
				 */

    /* The following arrays are used to hold results read from the
     * SPICE output.  We look at two signals:  the input and the output
     * of the stage being measured.  For each of these we record three
     * times:  two are used to compute slope, and the third is the
     * time when the signal changes logical value.  All times are recorded
     * in nanoseconds.
     */

#define NTIMES 3
    static double volts[NTIMES]		/* Voltage levels we want times for. */
	= {2.0, 2.2, 2.4};
    static double inTimes[NTIMES];	/* Times for the input signal. */
    static double outTimes[NTIMES];	/* Times for the output signal. */
    double lastTime;			/* Used for interpolation. */
    double lastInVolts;			/* Also used for interpolation. */
    double lastOutVolts;		/* Also used for interpolation. */
    double time, in, out;		/* Times from current line. */
    double latestTime = 0;		/* Last significant time from the last
					 * SPICE run (used to guess how many ns
					 * the next SPICE run will last).
					 */
    double prevLatestTime;		/* Same thing, from two runs ago. */

    /* Random temporary variables: */

    int i, j, gotDot;
    double d, tranTime, c2, inES, outES, nativeOutES, scale;
    char s1[40], s2[40], *s, *capString;

    /* The command line either contains no arguments, or three.
     * The arguments are the voltages to use for computing delays
     * and edgeSpeeds.  The middle voltage is Vinv, the logic
     * threshold.  The first and last voltages are used to compute
     * edge speed by assuming linear rise or fall time between them.
     */

    if (argc == 4)
    {
	volts[0] = atof(argv[1]);
	volts[1] = atof(argv[2]);
	volts[2] = atof(argv[3]);
    }
    else if (argc != 1)
    {
	fprintf(stderr,
	    "Mkcp takes zero arguments or three (Vlow, Vinv, Vhigh).\n");
	return;
    }

    /* Read the template SPICE deck into main memory and
     * parse off the values to be used for c1.  Then
     * generate the temporary file names to be used for
     * communication with SPICE.
     */

    for (nLines = 0; nLines < MAXLINES; nLines += 1)
    {
	if (fgets(curLine, MAXLINESIZE, stdin) == NULL) break;
	if (sscanf(curLine, "c1 %39s %39s", s1, s2) == 2)
	{
	    s = curLine;
	    (void) NextField(&s);
	    (void) NextField(&s);
	    (void) NextField(&s);
	    for (nCaps = 0; nCaps < MAXCAPS; nCaps++)
	    {
		capString = NextField(&s);
		if (capString == NULL) break;
		(void) sscanf(capString, "%Fp", &(cap[nCaps]));
	    }
	    if (NextField(&s) != NULL)
		fprintf(stderr, "Only %d c1 values can be given at once.\n",
		    MAXCAPS);
	    (void) sprintf(curLine, "c1 %s %s", s1, s2);
	}
	lines[nLines] = malloc((unsigned) (strlen(curLine) + 1));
	strcpy(lines[nLines], curLine);
    }
    spiceDeckName = mktemp("/usr/tmp/spiceInXXXXXX");
    spiceResultsName = mktemp("/usr/tmp/spiceOutXXXXXX");

    /* Enter the outer loop to run SPICE. */

    for (j = 0; j < nCaps; j += 1)
    {
	/* Step 1: output the SPICE deck for this run.  In the
	 * line starting with "c1" substitute for the current
	 * capacitance value.  In the line starting with ".tran",
	 * update the maximum time if necessary (use the latest
	 * time from the last run, plus the change in capacitance,
	 * to guess how many ns this run will last.  Also grab up
	 * the "c2" value to use in computing effective resistance.
	 */

	createSpiceInput:
	f = fopen(spiceDeckName, "w");
	if (f == NULL)
	{
	    fprintf(stderr, "Couldn't open %s for writing.\n", spiceDeckName);
	    goto allDone;
	}
	gotc1 = gotc2 = gotTran = 0;
	for (i=0; i<nLines; i+=1)
	{
	    if (sscanf(lines[i], "c1 %39s %39s", s1, s2) == 2)
	    {
		gotc1 = 1;
		fprintf(f, "%s %.8fp\n", lines[i], cap[j]);
		/* printf("%s %.8fp\n", lines[i], cap[j]); */
	    }
	    else if (sscanf(lines[i], "c2 %39s %39s %Fp", s1, s2, &d) == 3)
	    {
		gotc2 = 1;
		fputs(lines[i], f);
		c2 = d;
	    }
	    else if (strncmp(lines[i], ".tran", 5) == 0)
	    {
		gotTran = 1;
		if (latestTime == 0) fputs(lines[i], f);
		else
		{
		    s1[0] = 0;
		    (void) sscanf(lines[i], "%*s %*s %*s %[^]", s1);
		    tranTime = latestTime *
			(c2 + cap[j])*extraTime/(c2 + cap[j-1]);
		    if (tranTime < 2.0) tranTime = 2.0;
		    d = tranTime/180;

		    /* The following code is to round the time delta off
		     * to a value with only one significant digit.  This
		     * is done to avoid roundoff error in the times that
		     * SPICE prints out, which only have a few significant
		     * digits.
		     */

		    scale = 1.0;
		    while (d > 10)
		    {
			d /= 10.0;
			scale *= 10.0;
		    }
		    while (d < 1)
		    {
			d *= 10.0;
			scale /= 10.0;
		    }
		    (void) sprintf(curLine, "%.0f", d+.5);
		    (void) sscanf(curLine, "%F", &d);
		    d *= scale;
		    fprintf(f, ".tran %.2fns %.2fns %s\n", d, tranTime, s1);
		    /* printf(".tran %.2fns %.2fns %s\n", d, tranTime, s1); */
		}
	    }
	    else fputs(lines[i], f);
	}
	fclose(f);
	if (!gotc1)
	{
	    fprintf(stderr, "No c1 line in the SPICE deck.\n");
	    goto allDone;
	}
	if (!gotc2)
	{
	    fprintf(stderr, "No c2 line in the SPICE deck.\n");
	    goto allDone;
	}
	if (!gotTran)
	{
	    fprintf(stderr, "No .tran line in the SPICE deck.\n");
	    goto allDone;
	}

	/* Now run SPICE. */

	sprintf(curLine, "spice < %s > %s", spiceDeckName, spiceResultsName);
	system(curLine);

	/* Read back the SPICE results. */

	f = fopen(spiceResultsName, "r");
	if (f == NULL)
	{
	    fprintf(stderr, "Couldn't read results from %s.\n",
		spiceResultsName);
	    goto allDone;
	}

	/* The output table starts just after the first line whose
	 * first character is X.  Thank goodness for little signals
	 * like this.
	 */
	
	for (;;)
	{
	    if (fgets(curLine, MAXLINESIZE, f) == NULL)
	    {
		fprintf(stderr, "SPICE output didn't contain any ");
		fprintf(stderr, "results at c1 = %.8f pf.\n", cap[j]);
		goto allDone;
	    }
	    if (curLine[0] == 'X') break;
	}

	/* Now use interpolation to find the times for each of input
	 * and output (the 2nd and 3rd columns of each line) corresponding
	 * to each of the voltages in the voltage table.
	 */
	
	lastTime = -1;
	prevLatestTime = latestTime;
	latestTime = 0;
	for (i=0; i<NTIMES; i+=1)
	{
	    inTimes[i] = -1;
	    outTimes[i] = -1;
	}
	while (fgets(curLine, MAXLINESIZE, f) != NULL)
	{
	    /* Sigh... replace any d's in the line by E's, so scanf
	     * can parse it OK.  Also, fill in empty spaces between
	     * decimal points and the "d"s.
	     */
	    
	    for (s = curLine; *s != 0; s += 1)
	    {
		if (*s != ' ') gotDot = 0;
		else if (gotDot) *s = '0';
		if (*s == '.') gotDot = 1;
		if (*s == 'd') *s = 'E';
	    }
	    if (sscanf(curLine, " %F %F %F", &time, &in, &out) != 3) continue;
	    time *= 1e9;
	    if (lastTime >= 0) for (i=0; i<NTIMES; i+=1)
	    {
		if (((in < volts[i]) && (lastInVolts >= volts[i]))
		    || (in > volts[i]) && (lastInVolts <= volts[i]))
		{
		    if (inTimes[i] != -1)
		    {
			fprintf(stderr, "Input signal oscillates");
			fprintf(stderr, " around %f volts.\n", volts[i]);
		    }
		    inTimes[i] = lastTime + ((volts[i]-lastInVolts)
			*(time-lastTime)/(in-lastInVolts));
		    if (inTimes[i] > latestTime) latestTime = inTimes[i];
		}
		if (((out < volts[i]) && (lastOutVolts >= volts[i]))
		    || (out > volts[i]) && (lastOutVolts <= volts[i]))
		{
		    if (outTimes[i] != -1)
		    {
			fprintf(stderr, "Output signal oscillates");
			fprintf(stderr, " around %f volts.\n", volts[i]);
		    }
		    outTimes[i] = lastTime + ((volts[i]-lastOutVolts)
			*(time-lastTime)/(out-lastOutVolts));
		    if (outTimes[i] > latestTime) latestTime = outTimes[i];
		}
	    }
	    lastTime = time;
	    lastInVolts = in;
	    lastOutVolts = out;
	}
	fclose(f);

	/* Make sure we got all the time information we need. */

	for (i=0; i<NTIMES; i+=1)
	{
	    if (inTimes[i] == -1)
	    {
		if ((j == 0) || (extraTime > 20))
		{
		    fprintf(stderr, "Input signal never crossed ");
		    fprintf(stderr, "%f volts at c1 = %.8f pf.\n",
			volts[i], cap[j]);
		    goto allDone;
		}
		else
		{
		    latestTime = prevLatestTime;
		    extraTime *= 1.5;
		    /* printf("Retrying c1 = %.8f pf.\n", cap[j]); */
		    goto createSpiceInput;
		}
	    }
	    if (outTimes[i] == -1)
	    {
		if (j == 0)
		{
		    fprintf(stderr, "Output signal never crossed ");
		    fprintf(stderr, "%f volts at c1 = %.8f pf.\n",
			volts[i], cap[j]);
		    goto allDone;
		}
		else
		{
		    latestTime = prevLatestTime;
		    extraTime *= 1.5;
		    /* printf("Retrying c1 = %.8f pf.\n", cap[j]); */
		    goto createSpiceInput;
		}
	    }
	}

	/* Output information about this run. */

	/* printf("Input capacitance: %.3f pf.\n", cap[j]);
	printf("Output capacitance: %.3f pf.\n", c2);
	printf("Input times: %.1f ns, %.1f ns, %.1f ns.\n",
	    inTimes[0], inTimes[1], inTimes[2]);
	printf("Output times: %.1f ns, %.1f ns, %.1f ns.\n",
	    outTimes[0], outTimes[1], outTimes[2]);
	*/
	inES = (inTimes[2] - inTimes[0])/(volts[2] - volts[0]);
	if (inES < 0.0) inES = -inES;
	outES = (outTimes[2] - outTimes[0])/(volts[2] - volts[0]);
	if (outES < 0.0) outES = -outES;
	if (j == 0) nativeOutES = outES;
	printf("Input edge speed: %.1f ns/volt.\n", inES);
	printf("Edge speed ratio: %.3f\n", inES/nativeOutES);
	printf("Effective resistance: %.1f kohms.\n",
	    (outTimes[1] - inTimes[1])/c2);
	printf("Output edge speed: %.1f ns/volt/pf.\n\n", outES/c2);
	
	/* Delete the results after each run so we'll know if no
	 * results are generated by the next run.
	 */

	unlink(spiceResultsName);
    }

    /* Before returning, delete the temporary files. */

    allDone: unlink(spiceDeckName);
    unlink(spiceResultsName);
    exit(0);
}
