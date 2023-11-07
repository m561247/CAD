/* slopemodel.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)slopemodel.c	2.1 (Berkeley) 10/27/84"
 *
 * This file contains routines to do delay calculations based
 * on slope models that include edge rise and fall speeds.
 */

#include "crystal.h"

/* Imports from other Crystal modules: */

extern Type TypeTable[];
extern float Vinv, Vdd;
extern float RUpFactor, RDownFactor;
extern char *PrintNode();

/* The RC model is inaccurate when the trigger for a stage turns
 * on or off very slowly.  A slow edge on the trigger causes it
 * to have a different effective resistance value than if it turns
 * on or off very quickly (the voltage on the gate when the transistor
 * does all of its work is much less).  To correct for this effect,
 * the slope model used in this file characterizes a transistor not
 * by a single effective resistance, but by a resistance whose value
 * varies with the input edge speed.  A set of tables is used to represent
 * this variation.  Actually, what matters is the ratio of the input
 * edge speed to the native output edge speed of the stage (the edge speed
 * that would occur with an infinitely fast input edge).  If the input edge
 * is much faster than the native output edge speed, then the resistance of
 * the transistor should be about what the RC model predicts.  If the
 * input edge speed is noticeable in comparison to the output native
 * edge speed, then the effective resistance changes.
 *
 * Three tables are used by this file for each transistor type to
 * describe its behavior when pulling up, and three more tables to
 * describe its behavior when pulling down.  The three tables are
 * similar for the up and down cases.  The first table of each group,
 * upESRatio or downESRatio, gives edge speed ratios:  the ratio
 * of the input edge speed to the native output edge speed.  The
 * entries in the second table of each group, upREff or downREff,
 * give the effective resistance corresponding to the entries in
 * upESRatio or downESRatio.  What this means is that if the transistor
 * is one square in size, the table value is the effective resistance
 * of the transistor.  If the transistor isn't one square in size,
 * multiply the table value by the length/width of the transistor.
 * Linear interpolation is used for in-between edge speed ratios.
 * The third table in each group, upOutES or downOutES, is used to
 * compute the edge speed of the transistor's output.  Its entries
 * correspond to the entries in upESRatio or downESRatio, respectively.
 * For a one-square transistor driving a one pf load, the table entries
 * give the output edge speed, which is used to calculate the effective
 * resistance in later stages.  If different size transistors or loads
 * are used, scale the table value by the aspect ratio of the transistor
 * and the size of the load in pfs.  All the values in this table are
 * absolute values:  we always work with positive edge speeds, even when
 * the signals are actually falling.
 *
 * Note:  the 0th values in the OutES and REff tables give the
 * native values for the stage.  The 1st entry in the ESRatio tables
 * must be zero.
 *
 * There are several assumptions in all of this:
 *
 * 1. The table values scale perfectly for different size transistors
 *    and loads.
 * 2. If there are several transistors in series in the stage, only
 *    the effective resistance of the trigger is other than its native
 *    value.  The overall effective resistance is computed by summing
 *    effective resistances computed individually for the transistors.
 * 3. If there are several transistors in series in the stage, the
 *    output edge speed is computed by summing output edge speeds
 *    computed individually for each transistor and resistor in the stage.
 */

/* The following stuff is used to keep from printing zillions of
 * messages when the tables overflow a lot.
 */

static float maxUpRatio[MAXTYPES];
static float maxDownRatio[MAXTYPES];


SlopeDelay(stage)
register Stage *stage;		/* The stage whose delay is to be
				 * evaluated.  This stage must have a st_prev,
				 * describing the driving stage.
				 */

/*---------------------------------------------------------
 *	This routine computes delays using the fancy slope
 *	model.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The st_time value in the stage is filled in with the
 *	time at which the stage's destination will rise or fall.
 *	The st_edgeSpeed value is filled in with the absolute
 *	value of the speed for the destination's edge.  If a
 *	transition cannot occur, the st_time is set to -1 and
 *	st_edgeSpeed to 0.
 *
 *	Approximations:
 *	No side paths are considered:  only the direct path from
 *	trigger to destination is considered for resistance and
 *	capacitance.  Resistances and capacitances are lumped,
 *	which is a bit pessimistic.
 *---------------------------------------------------------
 */

{
    register FET *f;
    register float *ratioTbl, tmp;
    register Type *tt;
    float *rEffTbl, *outESTbl;
    float r, c, tmp2, edgeSpeed, resVolts, rFactor;
    int i, isLoad;

    /* Make sure that there is stuff in the path. */

    stage->st_time = -1.0;
    stage->st_edgeSpeed = 0;
    if ((stage->st_piece1Size == 0) && (stage->st_piece2Size <= 1))
    {
	printf("Crystal bug: nothing in stage!\n");
	return;
    }

    /* Precompute the voltage drop used to compute resistor edge speeds. */

    if (stage->st_rise)
    {
	resVolts = Vdd-Vinv;
	rFactor = RUpFactor;
    }
    else
    {
	resVolts = Vinv;
	rFactor = RDownFactor;
    }

    /* Figure out if there's a load driving this stage (we know it's
     * a load if there's no piece1 and the first node in piece2 is
     * fixed in value).
     */
    
    if ((stage->st_piece1Size == 0)
	&& (stage->st_piece2Node[0]->n_flags & (NODE1ALWAYS|NODE0ALWAYS)))
	isLoad = TRUE;
    else isLoad = FALSE;

    /* Go through the two pieces of the path, summing all the
     * resistances in both pieces and the capacitances in piece2
     * (I assume that all the capacitance in piece1 has already
     * been drained away).  Also sum all the edge speed components.
     * If a zero FET resistance is found, it means that the transistor
     * can't drive in the given direction, so quit.  Leave out the
     * resistance and edge speed contributions of the trigger; these
     * will be computed by interpolation later.  Also leave out the
     * capacitance, resistance, and edge speed contributions of the
     * node that is the source of the signal:  this node is supposed
     * to be infinitely capacitive and have zero series resistance.
     */
    
    r = 0.0;
    c = 0.0;
    edgeSpeed = 0.0;
    for (i=0; i<stage->st_piece1Size; i+=1)
    {
	if (i != 0)
	{
	    f = stage->st_piece1FET[i];
	    tt = &(TypeTable[f->f_typeIndex]);
	    if (stage->st_rise)
	    {
		tmp = tt->t_upREff[0];
		edgeSpeed += tt->t_upOutES[0] * f->f_aspect;
	    }
	    else
	    {
		tmp = tt->t_downREff[0];
		edgeSpeed += tt->t_downOutES[0] * f->f_aspect;
	    }
	    if (tmp == 0.0) return;
	    r += tmp * f->f_aspect;
	}

	/* The last node in piece1 is the signal source, so don't
	 * consider it.
	 */

	if (i != (stage->st_piece1Size-1))
	{
	    tmp = stage->st_piece1Node[i]->n_R;
	    r += tmp * rFactor;
	    edgeSpeed += tmp/(resVolts*1000);
	}
    }
    for (i=0; i<stage->st_piece2Size; i+=1)
    {
	/* When the stage is load-driven, we don't consider the
	 * first transistor (it will be interpolated below), or
	 * the first node in piece2, since it is the signal source.
	 */

	if ((i == 0) && isLoad) continue;
	f = stage->st_piece2FET[i];
	if (f != NULL)
	{
	    tt = &(TypeTable[f->f_typeIndex]);
	    if (stage->st_rise)
	    {
		tmp = tt->t_upREff[0];
		edgeSpeed += tt->t_upOutES[0] * f->f_aspect;
	    }
	    else
	    {
		tmp = tt->t_downREff[0];
		edgeSpeed += tt->t_downOutES[0] * f->f_aspect;
	    }
	    if (tmp == 0.0) return;
	    r += tmp * f->f_aspect;
	    c += f->f_area * TypeTable[f->f_typeIndex].t_cPerArea;
	}

	/* If no piece1, then first node in piece2 supplies signal,
	 * so don't use its capacitance, resistance, or edge speed.
	 */

	if ((i == 0) && (stage->st_piece1Size == 0)) continue;
	tmp = stage->st_piece2Node[i]->n_R;
	r += tmp * rFactor;
	edgeSpeed += tmp/(resVolts*1000);
	c += stage->st_piece2Node[i]->n_C;
    }

    /* If this stage is a carry-over from a bus (no piece1 and not
     * a load), then we're done;  return the RC constant as the delay,
     * and add the edgeSpeed from the previous stage to our edgeSpeed
     * (because the stages are really hooked together).  If this isn't
     * a bus carryover, then figure out which is the transistor that
     * is used for interpolation.
     */

    if (stage->st_piece1Size == 0)
    {
	if (!isLoad)
	{
	    stage->st_time = (r*c)/1000.0 + stage->st_prev->st_time;
	    stage->st_edgeSpeed = edgeSpeed*c + stage->st_prev->st_edgeSpeed;
	    return;
	}
	f = stage->st_piece2FET[0];
    }
    else f = stage->st_piece1FET[0];

    /* Now do linear interpolation in the slope tables to compute
     * the effective resistance of the trigger or load transistor.
     */
    
    tt = &(TypeTable[f->f_typeIndex]);
    if (stage->st_rise)
    {
	ratioTbl = tt->t_upESRatio;
	rEffTbl = tt->t_upREff;
	outESTbl = tt->t_upOutES;
    }
    else
    {
	ratioTbl = tt->t_downESRatio;
	rEffTbl = tt->t_downREff;
	outESTbl = tt->t_downOutES;
    }

    /* Zero resistance means this transistor can't drive in this
     * direction, so return immediately.  Zero capacitance means
     * the node transits instantly.
     */

    if (*rEffTbl == 0) return;
    if (c == 0.0)
    {
	printf("Zero capacitance in stage leading to %s!\n",
	    PrintNode(stage->st_piece2Node[stage->st_piece2Size - 1]));
	stage->st_time = stage->st_prev->st_time;
	stage->st_edgeSpeed = 0;
	return;
    }

    tmp = stage->st_prev->st_edgeSpeed
	/(c*(edgeSpeed + (outESTbl[0]*f->f_aspect)));
    for (i=1; i<MAXSLOPEPOINTS; i++)
    {
	if (tmp <= ratioTbl[1]) goto gotPoint;
	if (ratioTbl[1] <= *ratioTbl) break;
	ratioTbl++;
	rEffTbl++;
	outESTbl++;
    }

    /* We ran off the top of the table.  Use the last two
     * values in the table for extrapolation.
     */
    
/* Skip the message printing stuff... it just makes users nervous.

    if (stage->st_rise)
    {
	if (tmp > maxUpRatio[f->f_typeIndex])
	{
	    float x, y;
	    PrintFETXY(f, &x, &y);
	    maxUpRatio[f->f_typeIndex] = tmp;
    	    printf("Warning: ran off end of %s up slope table with edge\n",
		TypeTable[f->f_typeIndex].t_name);
	    printf("    speed ratio %f for fet at (%.1f, %.1f).\n",
		tmp, x, y);
	    printf("    Maybe you should expand the range of the table.\n");
	}
    }
    else if (tmp > maxDownRatio[f->f_typeIndex])
    {
	float x, y;
	PrintFETXY(f, &x, &y);
	maxDownRatio[f->f_typeIndex] = tmp;
	printf("Warning: ran off end of %s down slope table with edge\n",
	    TypeTable[f->f_typeIndex].t_name);
	printf("    speed ratio %f for fet at (%.1f, %.1f).\n",
	    tmp, x, y);
	printf("    Maybe you should expand the range of the table.\n");
    }
*/
    ratioTbl--;
    rEffTbl--;
    outESTbl--;

    gotPoint:
    tmp = (tmp - *ratioTbl)/(ratioTbl[1] - *ratioTbl);
    tmp2 = f->f_aspect * (*rEffTbl + tmp * (rEffTbl[1] - *rEffTbl));
    stage->st_time = c*(r + tmp2)/1000.0 + stage->st_prev->st_time;
    tmp2 = f->f_aspect * (*outESTbl + tmp * (outESTbl[1] - *outESTbl));
    stage->st_edgeSpeed = c*(edgeSpeed + tmp2);
}
