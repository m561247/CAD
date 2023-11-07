/* prsmodel.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)prsmodel.c	2.5 (Berkeley) 10/27/84"
 *
 * This file contains routines to do delay calculations based
 * on slope models that include edge rise and fall speeds.  It
 * also contains the Penfield-Rubinstein approximations to
 * handle distributed networks.  The average of the PR minimum
 * and maximum delays is used.
 */

#include "crystal.h"

/* Imports from other Crystal modules: */

extern Type TypeTable[];
extern float Vinv, Vdd;
extern float RUpFactor, RDownFactor;
extern char *PrintNode();

/* See slopemodel.c for a description of how the slope model
 * works.  In this model, we use the slope model except instead
 * of summing all the R's, all the C's, and then multiplying,
 * we take the integral of Rdc where R is the resistance to
 * a point in the network and dc is the differential charge
 * at that point.
 */

/* The following stuff is used to keep from printing zillions of
 * messages when the tables overflow a lot.
 */

static float maxUpRatio[MAXTYPES];
static float maxDownRatio[MAXTYPES];


PRSlopeDelay(stage)
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
    float r, c, tmp2, es, resVolts, rcIntegral, thisC, thisR;
    float esIntegral, rFactor;
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

    /* Go through the two pieces of the path, computing the
     * integral of RdC through the path.  There are a couple
     * of hitches in this.  First of all, don't count any
     * capacitance in piece1:  we assume it's been drained away
     * already.  Also, we have to leave out the resistance
     * of the trigger for now;  it will be interpolated later.
     * To add in its value later, we have to remember the total
     * capacitance of the path.  Also, leave out the capacitance
     * and resistance of the node that is the source of the signal:
     * this node is supposed to have infinite capacitance and
     * zero resistance.
     */
    
    /* While summing up delay contributions, also sum up edge speed
     * speed contributions in the same way.
     */

    r = 0.0;
    c = 0.0;
    rcIntegral = 0.0;
    es = 0.0;
    esIntegral = 0.0;
    for (i=0; i<stage->st_piece1Size; i+=1)
    {
	if (i != 0)
	{
	    f = stage->st_piece1FET[i];
	    tt = &(TypeTable[f->f_typeIndex]);
	    if (stage->st_rise)
	    {
		tmp = tt->t_upREff[0];
		es += tt->t_upOutES[0] * f->f_aspect;
	    }
	    else
	    {
		tmp = tt->t_downREff[0];
		es += tt->t_downOutES[0] * f->f_aspect;
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
	    es += tmp/(resVolts*1000);
	}
    }
    for (i=0; i<stage->st_piece2Size; i+=1)
    {
	/* When the stage is load-driven, we don't consider the
	 * first transistor (it will be interpolated below), or
	 * the first node in piece2, since it is the signal source.
	 */

	if ((i == 0) && isLoad) continue;

	/* If no piece1, then first node in piece2 supplies signal,
	 * so don't use the capacitance, resistance, or edge speed
	 * associated with the node.
	 */

	if ((i != 0) || (stage->st_piece1Size != 0))
	{
	    thisR = stage->st_piece2Node[i]->n_R;
	    thisC = stage->st_piece2Node[i]->n_C;
	    r += thisR * rFactor;
	    c += thisC;
	    rcIntegral += thisC*r;
	    es += thisR/(resVolts*1000);
	    esIntegral += thisC*es;
	}

	f = stage->st_piece2FET[i];
	if (f != NULL)
	{
	    tt = &(TypeTable[f->f_typeIndex]);
	    if (stage->st_rise)
	    {
		thisR = tt->t_upREff[0];
		es += tt->t_upOutES[0] * f->f_aspect;
	    }
	    else
	    {
		thisR = tt->t_downREff[0];
		es += tt->t_downOutES[0] * f->f_aspect;
	    }
	    if (thisR == 0.0) return;
	    r += thisR*f->f_aspect;
	    thisC = f->f_area * TypeTable[f->f_typeIndex].t_cPerArea;
	    c += thisC;
	    rcIntegral += thisC*r;
	    esIntegral += thisC*es;
	}
    }

    /* If this stage is a carry-over from a bus (no piece1 and not
     * a load), then we're done;  return the RC integral as the delay,
     * and add the edgeSpeed from the previous stage to our edgeSpeed
     * (because the stages are really hooked together).  If this isn't
     * a bus carryover, then figure out which is the transistor that
     * is used for interpolation.
     */

    if (stage->st_piece1Size == 0)
    {
	if (!isLoad)
	{
	    stage->st_time = rcIntegral/1000.0 + stage->st_prev->st_time;
	    stage->st_edgeSpeed = esIntegral + stage->st_prev->st_edgeSpeed;
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
	/(esIntegral + c*f->f_aspect*outESTbl[0]);
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
    
/* Skip the message printing.... it just makes users nervous. 

    if (stage->st_rise)
    {
	if (tmp > maxUpRatio[f->f_typeIndex])
	{
	    maxUpRatio[f->f_typeIndex] = tmp;
    	    printf("Warning: ran off end of %s up slope table with\n",
		TypeTable[f->f_typeIndex].t_name);
	    printf("    edge speed ratio %f.  Maybe you should expand\n", tmp);
	    printf("    the range of the table.\n");
	}
    }
    else if (tmp > maxDownRatio[f->f_typeIndex])
    {
	maxDownRatio[f->f_typeIndex] = tmp;
	printf("Warning: ran off end of %s down slope table with\n",
	    TypeTable[f->f_typeIndex].t_name);
	printf("    edge speed ratio %f.  Maybe you should expand\n", tmp);
	printf("    the range of the table.\n");
    }
*/
    ratioTbl--;
    rEffTbl--;
    outESTbl--;

    gotPoint:
    tmp = (tmp - *ratioTbl)/(ratioTbl[1] - *ratioTbl);
    tmp2 = f->f_aspect * (*rEffTbl + tmp * (rEffTbl[1] - *rEffTbl));
    rcIntegral += tmp2*c;
    stage->st_time = rcIntegral/1000.0 + stage->st_prev->st_time;
    tmp2 = f->f_aspect * (*outESTbl + tmp * (outESTbl[1] - *outESTbl));
    stage->st_edgeSpeed = esIntegral + tmp2*c;
}
