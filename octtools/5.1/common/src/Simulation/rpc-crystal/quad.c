/* quad.c -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)quad.c	2.1 (Berkeley) 4/9/83"
 *
 * This file contains a single routine used to change an area
 * and perimeter into a length and width.
 */

#include "port.h"

float
Quad(a, p)
float a;			/* Area. */
float p;			/* Perimeter. */

/*---------------------------------------------------------
 *	This routine solves the quadratic formula.
 *
 *	Results:
 *	The return value is the length/width ratio of a rectangle
 *	whose perimeter is p and area is a.  The return value is
 *	always greater than or equal to 1.
 *
 *	Side Effects:	None.
 *---------------------------------------------------------
 */

{
    double P, A, root;
    float result;

    P = p;
    A = a;
    root = sqrt((P*P) - (16*A));
    result = (P + root)/(P - root);
    return result;
}
