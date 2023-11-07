
/***********************************************************************
 * Process OCT Latch Gates
 * Filename: latch.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define LATCH_MUSA
#include "musa.h"

/* data inside of latch instance */
#define STORED	0x3	/* voltage stored in latch (for master-slave) */
#define O1INV	0x4	/* output1 is inverting */
#define O2INV	0x8	/* output2 exists and is inverting */
#define ACTIVE_LOW	0x10	/* latch is Active Low (master is AL) */

/* Terminal types detected */
#define ITERM	1	/* input */
#define OTERM	2	/* output */
#define OBTERM	4	/* outputbar */
#define CTERM	8	/* control */
#define OTERMS	6	/* both outputs */

#define CONTROL_TERM 0
#define INPUT_TERM   1
#define OUTPUT_TERM  2

/************************************************************************
 * TRISTATE_READ
 * Read a latch from Oct
 */
int16 tristate_read(ifacet, inst, celltype)
	octObject *ifacet, *inst;
	char *celltype;
{
	musaInstance *newmusaInstance;
	octObject control, input, output, outputbar; /* terminals */
	octObject term, prop;
	octGenerator termgen;
	deviceType type;		/* instance type */
	int checklist = 0;		/* terminals found */
	int inst_data = L_x;	/* mask of instance data */

	(void) ignore(celltype);		/* to make saber and lint shut up */

	/* identify latch type */
	if (ohGetByPropName(ifacet, &prop, "SYNCHMODEL") != OCT_OK) {
		return(FALSE);
	} /* if... */
	if (StrPropEql(&prop, "TRANSMISSION-GATE") ) {
		type = DEVTYPE_PASS_GATE;
	} else if (StrPropEql(&prop,"TRISTATE") || StrPropEql(&prop,"TRISTATE-DRIVER")) {
		type = DEVTYPE_TRISTATE_BUFFER;
	} else {
		return(FALSE);
	} /* if... */
	if (ohGetByPropName(ifacet, &prop, "SYNCHACTIVE") == OCT_OK) {
		if (strcmp(prop.contents.prop.value.string, "LOW") == 0) {
			inst_data |= ACTIVE_LOW;
		} else if (strcmp(prop.contents.prop.value.string, "HIGH") != 0) {
			(void) sprintf(errmsg,"Illegal value (%s) for SYNCHACTIVE in %s:%s. Legal values are LOW or HIGH",
				prop.contents.prop.value.string,
				ifacet->contents.facet.cell,
				ifacet->contents.facet.view);
			Warning(errmsg);
		} /* if... */
	} /* if... */

	OH_ASSERT(octInitGenContents(ifacet, OCT_TERM_MASK, &termgen));
	while (octGenerate(&termgen, &term) == OCT_OK) {
		if (ohGetByPropName(&term, &prop, "SYNCHTERM") == OCT_OK) {
			if (strcmp(prop.contents.prop.value.string, "INPUT") == 0) {
				OH_ASSERT(ohGetByTermName(inst, &input, term.contents.term.name));
				checklist |= ITERM;
			} else if (strcmp(prop.contents.prop.value.string, "OUTPUT") == 0) {
				OH_ASSERT(ohGetByTermName(inst, &output, term.contents.term.name));
				checklist |= OTERM;
			} else if (strcmp(prop.contents.prop.value.string, "OUTPUTBAR") == 0) {
				OH_ASSERT(ohGetByTermName(inst, &outputbar, term.contents.term.name));
				checklist |= OBTERM;
			} else if ((strcmp(prop.contents.prop.value.string, "CONTROL") == 0)) {
				if ( !(checklist & CTERM)) {
					OH_ASSERT(ohGetByTermName(inst, &control, term.contents.term.name));
					checklist |= CTERM;
				} /* if... */
			} else if ((strcmp(prop.contents.prop.value.string, "CONTROLBAR") == 0)) {
				if (!(checklist & CTERM)) {
					OH_ASSERT(ohGetByTermName(inst, &control, term.contents.term.name));
					checklist |= CTERM;
					inst_data |= ACTIVE_LOW;
				} /* if... */
			} else {
				(void) sprintf(errmsg,"\n\tUnknown value (%s) for SYNCHTERM\n\ton %s",prop.contents.prop.value.string,ohFormatName(&term));
				Warning(errmsg);
			} /* if... */
		} else {
			/* The terminal does not have SYNCHTERM, Can we really ignore this fact ?*/
			if (ohGetByPropName(&term, &prop, "LOGICFUNCTION") != OCT_OK) {
				if (ohGetByPropName(&term, &prop, "TERMTYPE") != OCT_OK || (!(StrPropEql(&prop,"SUPPLY") || StrPropEql(&prop,"GROUND")))) {
					(void) sprintf(errmsg,"\n\
\tTerm %s has no SYNCHTERM property\n\
\tnor the LOGICFUNCTION property\n\
\tnor is it a SUPPLY or a GROUND terminal.\n\
\tThis it too much for me. I am sorry but I must abort.",
					ohFormatName(&term));
					FatalError(errmsg);
				} /* if... */
			} /* if... */
		} /* if... */
	} /* while... */

    if ((checklist & ITERM) == 0) {
		(void) sprintf(errmsg,"Bad tristate or transmission gate %s:%s detected -- Missing input",ifacet->contents.facet.cell,ifacet->contents.facet.view);
		FatalError(errmsg);
	} /* if... */
	if ((checklist & CTERM) == 0) {
		(void) sprintf(errmsg,"Bad tristate or transmission gate %s:%s detected -- Missing control",ifacet->contents.facet.cell,ifacet->contents.facet.view);
		FatalError(errmsg);
	} /* if... */
	switch (checklist & OTERMS) {
		case 0:
			(void) sprintf(errmsg,"Bad tristate or transmission gate %s:%s detected -- Missing output",ifacet->contents.facet.cell,ifacet->contents.facet.view);
			FatalError(errmsg);
		case OTERM:
			break;
		case OBTERM:
			inst_data |= O1INV;
			break;
		case OTERMS:
			inst_data |= O2INV;
			break;
	} /* switch... */
	if (inst_data & O2INV) {
		make_leaf_inst(&newmusaInstance, type, inst->contents.instance.name, 4);
	} else {
		make_leaf_inst(&newmusaInstance, type, inst->contents.instance.name, 3);
	} /* if... */
	connect_term(newmusaInstance, &control, 0);
	connect_term(newmusaInstance, &input, 1);
	if (inst_data & O1INV) {
		connect_term(newmusaInstance, &outputbar, 2);
	} else {
		connect_term(newmusaInstance, &output, 2);
		if (inst_data & O2INV) {
			connect_term(newmusaInstance, &outputbar, 3);
		} /* if... */
	} /* if... */
	newmusaInstance->data |= inst_data;
	return(TRUE);
} /* tristate_read... */

/***************************************************************************
 * TRISTATE_INFO
 */
tristate_info(inst)
	musaInstance *inst;
{
	MUSAPRINT("\n\t");
	if (inst->data & O1INV) {
		MUSAPRINT("OUTPUT1 is inverted;   ");
	} else if (inst->data & O2INV) {
		MUSAPRINT("OUTPUT2 is inverted;   ");
	} /* if... */
	if (inst->data & ACTIVE_LOW) {
		MUSAPRINT("CONTROL is active low\n");
	} else {
		MUSAPRINT("CONTROL is active high\n");
	} /* if... */
	if (inst->data & O2INV) {
		ConnectInfo(inst, 4);
	} else {
		ConnectInfo (inst, 3);
	} /* if... */
} /* tristate_info... */


static int inv [] = { L_ill, L_i, L_o, L_x};

/**************************************************************************
 * SIM_TRISTATE
 */
sim_tristate(inst)
	musaInstance *inst;
{
	int outputVoltage = 0x3;			/* level to drive */
	int active;			/* tlatch or master is active */

	active = (inst->data & ACTIVE_LOW) ? LOW(inst->connArray[0].node) : HIGH(inst->connArray[0].node);
	if (active) {
		/* pass input to output */
		outputVoltage = inst->connArray[1].node->voltage;
		if (inst->data & O1INV) {
			drive_output(&(inst->connArray[2]), inv[outputVoltage]);
		} else {
			drive_output(&(inst->connArray[2]), outputVoltage);
			if (inst->data & O2INV) {
				drive_output(&(inst->connArray[3]), inv[outputVoltage]);
			} /* if... */
		} /* if... */
	} else {
		undrive_output(&(inst->connArray[2]));
		if (inst->data & O2INV) {
			undrive_output(&(inst->connArray[3]));
		} /* if... */
	} /* if... */
} /* sim_tristate... */
