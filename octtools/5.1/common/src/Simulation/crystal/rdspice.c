/* rdspice.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid = "@(#)rdspice.c	1.3 (Berkeley) 8/20/83";
 *
 * This file contains the entire rdspice program.  Rdspice
 * reads a spice file from stdin, and prints out delays and
 * edge speeds for each column (up to 10) in the SPICE output.
 */

#include "port.h"

/* Library imports: */


main(argc, argv)
int argc;
char **argv;

/*---------------------------------------------------------
 *	This is the main program for rdspice.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	Stuff gets printed on stdout.
 *
 *---------------------------------------------------------
 */

{
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
    static double outTimes[10][NTIMES]; /* Times for the output signals. */
    double lastTime;			/* Used for interpolation. */
    double lastVolts[10];		/* Also used for interpolation. */
    double time, col[10];		/* Fields from current line. */
    int colsUsed;			/* Actual number of columns in file. */

    /* Random temporary variables: */

    int i, j, gotDot;
    double d;
#define MAXLINESIZE 200
    char curLine[MAXLINESIZE], *s;

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
	    "Rdspice takes zero arguments or three (Vlow, Vinv, Vhigh).\n");
	return;
    }

    /* The output table starts just after the first line whose
     * first character is X.  Thank goodness for little signals
     * like this.
     */
    
    for (;;)
    {
	if (fgets(curLine, MAXLINESIZE, stdin) == NULL)
	{
	    fprintf(stderr, "SPICE output didn't contain any results.\n");
	    exit(0);
	}
	if (curLine[0] == 'X') break;
    }

    /* Now use interpolation to find the times for each of columns 1-n
     * corresponding to each of the voltages in the voltage table.  The
     * first column gives times.
     */
    
    lastTime = -1;
    colsUsed = 1;
    for (i=0; i<NTIMES; i+=1) for (j=0; j<10; j+= 1)
    {
	outTimes[j][i] = -1;
    }
    while (fgets(curLine, MAXLINESIZE, stdin) != NULL)
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
	i = sscanf(curLine, " %F %F %F %F %F %F %F %F %F %F %F",
	    &time, &col[0], &col[1], &col[2],
	    &col[3], &col[4], &col[5], &col[6],
	    &col[7], &col[8], &col[9]);
	i = i-1;
	if (i < colsUsed) continue;
	if (colsUsed < i) colsUsed = i;
	time *= 1e9;
	if (lastTime >= 0)
	    for (i=0; i<NTIMES; i+=1) for (j=0; j<colsUsed; j+= 1)
	{
	    if (((col[j] < volts[i]) && (lastVolts[j] >= volts[i]))
		|| (col[j] > volts[i]) && (lastVolts[j] <= volts[i]))
	    {
		if (outTimes[j][i] != -1)
		{
		    fprintf(stderr, "Column %d oscillates", j);
		    fprintf(stderr, " around %f volts.\n", volts[i]);
		}
		outTimes[j][i] = lastTime + ((volts[i]-lastVolts[j])
		    *(time-lastTime)/(col[j]-lastVolts[j]));
	    }
	}
	lastTime = time;
	for (j=0; j<10; j++) lastVolts[j] = col[j];
    }

    /* Output results. */

    for (i=0; i<colsUsed; i+=1)
    {
	if ((outTimes[i][0] == -1) || (outTimes[i][1] == -1)
	    || (outTimes[i][2] == -1))
	{
	    printf("Column %d:  didn't settle.\n", i);
	    continue;
	}
	d = (outTimes[i][2] - outTimes[i][0]) / (volts[2] - volts[0]);
	if (d < 0) d = -d;
	printf("Column %d: %.2f ns, %.2f ns/volt\n", i, outTimes[i][1], d);
    }
}
