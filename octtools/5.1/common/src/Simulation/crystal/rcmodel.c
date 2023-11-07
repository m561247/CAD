/* rcmodel.c -
 *
 * Copyright (C) 1983 John K. Ousterhout
 * sccsid "@(#)rcmodel.c	2.5 (Berkeley) 12/14/83"
 *
 * This file contains routines to do delay calculations based
 * on simple RC models as in Mead and Conway.
 */

#include "crystal.h"

/* Imports from othe Crystal modules: */

extern Type TypeTable[];
extern float RUpFactor, RDownFactor;


RCDelay(stage)
register Stage *stage;		/* The stage whose delay is to be
				 * evaluated.  This stage must have a st_prev,
				 * describing the driving stage.
				 */

/*---------------------------------------------------------
 *	This routine computes delays using the simple RC
 *	model.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	The st_time value in the stage is filled in with the
 *	times at which the stage's destination	will rise or
 *	fall.  If a transistion can't occur, then the
 *	st_time value is set to -1.
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
    float r, c, tmp, rFactor;
    int i;

    /* Make sure that there is stuff in the path. */

    stage->st_time = -1.0;
    if ((stage->st_piece1Size == 0) && (stage->st_piece2Size <= 1))
    {
	printf("Crystal bug: nothing in stage!\n");
	return;
    }

    /* Go through the two pieces of the path, summing all the
     * resistances in both pieces and the capacitances in piece2
     * (I assume that all the capacitance in piece1 has already
     * been drained away).  If a zero FET resistance is found, it
     * means that the transistor can't drive in the given direction,
     * so quit.
     */
    
    r = 0.0;
    c = 0.0;
    if (stage->st_rise) rFactor = RUpFactor;
    else rFactor = RDownFactor;
    for (i=0; i<stage->st_piece1Size; i+=1)
    {
	f = stage->st_piece1FET[i];
	if (stage->st_rise)
	    tmp = TypeTable[f->f_typeIndex].t_rUp;
	else tmp = TypeTable[f->f_typeIndex].t_rDown;
	if (tmp == 0.0) return;
	r += tmp * f->f_aspect;
	r += stage->st_piece1Node[i]->n_R * rFactor;
    }
    for (i=0; i<stage->st_piece2Size; i+=1)
    {
	f = stage->st_piece2FET[i];
	if (f != NULL)
	{
	    if (stage->st_rise)
		tmp = TypeTable[f->f_typeIndex].t_rUp;
	    else tmp = TypeTable[f->f_typeIndex].t_rDown;
	    if (tmp == 0.0) return;
	    r += tmp * f->f_aspect;
	    c += f->f_area * TypeTable[f->f_typeIndex].t_cPerArea;
	}
	r += stage->st_piece2Node[i]->n_R * rFactor;
	c += stage->st_piece2Node[i]->n_C;
    }

    /* Throw out the capacitance and resistance from the node that
     * is supplying the signal (either the first node in piece1, or
     * the first node in piece2 if there isn't a piece1).  This node
     * is treated here as an infinite source of charge.
     */

    if (stage->st_piece1Size != 0)
	r -= stage->st_piece1Node[stage->st_piece1Size - 1]->n_R * rFactor;
    else
    {
	r -= stage->st_piece2Node[0]->n_R * rFactor;
	c -= stage->st_piece2Node[0]->n_C;
    }

    stage->st_time = (r*c)/1000.0 + stage->st_prev->st_time;
    stage->st_edgeSpeed = 0.0;
}
