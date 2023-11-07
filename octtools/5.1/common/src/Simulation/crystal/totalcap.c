/* *************************************************************** */
/* Fred W. Obermeier (c)                          Created 11/26/86 */
/* *************************************************************** */
/* All rights reserved.                                            */
/* *************************************************************** */
/* FILE: totalcap.c                                                */
/* SCCS: @(#)	totalcap.c	Version	1.1	updated_1/29/87		   */
/* *************************************************************** */
/* WARNING:  Under development.					   */
/* WARNING:  Do not distribute or modify without author's consent. */
/* *************************************************************** */

/* This file runs through the node list and totals all the cap's into
 * the following categories:
 *
 * C_input	-- Input cap. (Power comes from outside modules.)
 * C_driven	-- Cap. to GND, Vdd and other nodes.
 */

#include "crystal.h"
#include "hash.h"

float C_input, C_driven;

extern Node *VddNode, *GroundNode;
extern Node *FirstNode;

/* Run through all nodes and total all C's into C_input, C_gnd, C_vdd, and
 * C_other.
 */
TotalCap()
{
	Node *n;

	C_input = 0;
	C_driven = 0;

	n = FirstNode;
	while (n != NULL) {
		/* Skip Vdd and GND nodes.*/
		if ((n != VddNode) && (n != GroundNode)) {
			if (connected_to_source_or_drain (n)) {
#ifdef DEBUG
				printf ("Driven Node: '%s'\t\tCap.: %f\n",
					n->n_name, n->n_C);
#endif
				C_driven = C_driven + n->n_C;
			} else {
#ifdef DEBUG
				printf ("Input Node:  '%s'\t\tCap.: %f\n",
					n->n_name, n->n_C);
#endif
				C_input = C_input + n->n_C;
			}
		}

		n = n->n_next;	/* Traverse all nodes in order allocated */
				/* so we can search them all without gross */
				/* memory thrashing.*/
	}

	printf ("C_input:  %fpf\n", C_input);
	printf ("C_driven: %fpf\n", C_driven);
}

/* Returns TRUE if node is connected to a transistor source or drain.*/
/* (a driven node.)*/
connected_to_source_or_drain (n)
Node *n;
{
	Pointer *p;

	p = n->n_pointer;
	while ((p!=NULL) && (p->p_fet->f_source!=n) && (p->p_fet->f_drain!=n))
		p=p->p_next;	/* march through connected fet transistors .*/
	if (p==NULL) return (FALSE);	/* If not driven, return FALSE*/
	else return (TRUE);
}
