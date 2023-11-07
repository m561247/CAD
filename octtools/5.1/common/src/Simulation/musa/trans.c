/***********************************************************************
 * Routines to handle transistors. 
 * Filename: trans.c
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
 *	static long is_active() - check if transistor is active.
 *	int mos() - simulate mos transistor.
 *	int wmos() - simulate weak mos transistor.
 *	int pull() - simulate pulldown or pullup.
 *	int mos_restart() - restart mos transistor.
 *	int pull_restart() - restart pulldown/pullup.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define TRANS_MUSA
#include "musa.h"
#include "queue.h"

extern musaQueue queue1;
/*************************************************************************
 * IS_ACTIVE
 * Check if transistor network is active.
 *
 * Parameters:
 *	firstconn (connect *) - the connection to the first gate. (Input)
 *	param (int16) - 1=>nmos 0=>pmos. (Input)
 *	subtype (int) 
 */
static long is_active(firstconn, mostype, subtype)
    connect		*firstconn;		/* connect for the first gate */
    int16		mostype;			/* 1=>nmos,  0=>pmos */
    int			subtype;
{
    long mask = 1;

    /* make mask to look up answer */
    if (mostype == NMOS) {		/* NMOS */
	switch (network[subtype].numin) {
	case 5:
	    if (HIGH(firstconn[4].node)) {
		mask <<= 1;
	    }
	case 4:
	    if (HIGH(firstconn[3].node)) {
		mask <<= 2;
	    }
	case 3:
	    if (HIGH(firstconn[2].node)) {
		mask <<= 4;
	    }
	case 2:
	    if (HIGH(firstconn[1].node)) {
		mask <<= 8;
	    }
	case 1:
	    if (HIGH(firstconn[0].node)) {
		mask <<= 16;
	    }
	case 0: ;
	}	
    } else {			/* PMOS */
	switch (network[subtype].numin) {
	case 5:
	    if (LOW(firstconn[4].node)) {
		mask <<= 1;
	    } 
	case 4:
	    if (LOW(firstconn[3].node)) {
		mask <<= 2;
	    } 
	case 3:
	    if (LOW(firstconn[2].node)) {
		mask <<= 4;
	    } 
	case 2:
	    if (LOW(firstconn[1].node)) {
		mask <<= 8;
	    } 
	case 1:
	    if (LOW(firstconn[0].node)) {
		mask <<= 16;
	    } 
	case 0: ;
	}
    } 
    return(mask & network[subtype].active);
}			



#define SOURCE 0
#define DRAIN  1

void mos_do_drive( inst, ison, up )
    musaInstance* inst;
    int ison;
    int up;
{
    int out[2];
    int val[2];
    int thresh;
    int i;

    for ( i = SOURCE; i <= DRAIN; i++ ) {
	if (up) {
	    out[i] = inst->connArray[i].hout;
	    if ( inst->connArray[i].node->lowStrength == 0 ) { /* Is it to a LO rail? */
		val[i] = 255;	/* Value is weak. */
	    } else {
		val[i] = inst->connArray[i].node->highStrength;
	    }
	} else {
	    out[i] = inst->connArray[i].lout;
	    if ( inst->connArray[i].node->highStrength == 0 ) {	/* Is it to a HI rail */
		val[i] = 255;	/* make it weak. */
	    } else {
		val[i] = inst->connArray[i].node->lowStrength;
	    }
	}
    }

    if (ison) {
	for ( i = SOURCE; i <= DRAIN; i++ ) {
	    int source = i;
	    int drain = 1 - i;
	    if (out[source]) {	/* Terminal is driving node. */
		if (val[source] <= val[drain]) {	
		    if ( verbose ) printf( "Source of %s should not be output\n", inst->name );
		    if ( up ) {
			musaUndriveHigh(inst->connArray[source].node, &(inst->connArray[source]));
		    } else {
			musaUndriveLow(inst->connArray[source].node, &(inst->connArray[source]));
		    }
		    out[source] = 0;
		} else {
		    if ( up ) { 
			thresh = inst->connArray[source].highStrength;
		    } else {
			thresh = inst->connArray[source].lowStrength ;
		    }
		    if (val[drain]+1 < thresh ) {
			/* increase drive (decrease strength) */
			if (verbose) printf( "Term %d of %s drives %d\n", source, inst->name, up );
			if ( up ) {
			    inst->connArray[source].highStrength = val[drain]+1;
			    musaDriveHigh(inst->connArray[source].node, &(inst->connArray[source]));
			} else {
			    inst->connArray[source].lowStrength = val[drain]+1;
			    musaDriveLow(inst->connArray[source].node, &(inst->connArray[source]));
			}
		    }
		}	
	    }
	} 

	if ( out[0] == 0 && out[1] == 0 ) { /* No terminal is driving. */
	    /* determine which term (if any) to drive */
	    if ( val[0] == val[1] ) {
		return;
	    }
	    if ( val[0] >= 254  && val[1] >= 254 ) {
		return;
	    }
	    for ( i = SOURCE; i <= DRAIN; i++ ) {
		int source = i;
		int drain = 1 - i;

		if (val[drain] < val[source]) {
		    if (inst->connArray[drain].ttype != TERM_OUTPUT) {
			if (verbose) printf( "%s drives %d to %d\n", inst->name, source, up );
			if ( up ) {
			    inst->connArray[source].highStrength = val[drain]+1;
			    musaDriveHigh(inst->connArray[source].node, &(inst->connArray[source]));
			} else {
			    inst->connArray[source].lowStrength = val[drain]+1;
			    musaDriveLow(inst->connArray[source].node, &(inst->connArray[source]));
			}
		    }
		}
	    }
	}
    } else {			/* Gate is off -- undrive if necessary */
	for ( i = SOURCE; i <= DRAIN; i++ ) {	
	    if (out[i]) {
		if ( up ) {
		    musaUndriveHigh(inst->connArray[i].node, &(inst->connArray[i]));
		} else {
		    musaUndriveLow(inst->connArray[i].node, &(inst->connArray[i]));
		}
	    }
	}			
    }
}



/*********************************************************************
 * MOS
 * Regular old run-of-the-mill MOS transistor network
 *
 * Parameters:
 *	inst (musaInstance *) - the mos transistor instance. (Input)
 *	mostype (int16) - 1=>nmos 0=>pmos. (Input)
 */
int mos(inst, mostype)
	musaInstance	*inst;
	int16	mostype;		/* 1=>nmos,  0=>pmos */
{
    register int sval, dval;	/* level of source and drain */
    int sout, dout;		/* are source and drain outputs */
    long ison;			/* is transistor network on */
    long oldison;		/* former ison */

    oldison = inst->data & IS_ON;
    if ( inst->schedule & IS_SCHED3 ) {
	if ( inst->schedule & SKIP_SCHED3 ) {
	    if (verbose) printf( "Skip sched 3\n" );
	    return 0;		/* Skip processing. */
	}
	if (verbose) printf( "Sched3 %s\n", inst->name );
	ison = is_active(&(inst->connArray[2]), mostype, inst->data & SUBTYPE) ? IS_ON : 0;
	if (ison == oldison) { 
	    return 0;		/* No change in state. */
	}
	inst->data ^= IS_ON;	/* Then toggle the IS_ON */
    } else {
	/*
	 * This is a sched1 call -- this is different since the
	 * transistor may have been on, but NOT conducting as it should
	 */
	if (verbose) printf( "Sched1 %s\n", inst->name );
	inst->schedule |= SKIP_SCHED3; 	/* Any sched3 call is now unnecessary */
	ison = is_active(&(inst->connArray[2]), mostype, inst->data & SUBTYPE) ? IS_ON : 0;
	if ((ison == 0) && (oldison == 0)) {
	    return 0;		/* network was off, and remains off -- boring */
	}
	inst->data &= ~IS_ON;	/* Clear flag */
	inst->data |= ison;	/* Set it if nexcessary. */
    }	

    if (verbose) printf( "%s is %s\n", inst->name, ison ? "ON":"OFF" );
    mos_do_drive( inst, ison, 1 ); /* Drive/undrive 1 */
    mos_do_drive( inst, ison, 0 ); /* Drive/undrive 0 */

    return 1;
}				/* mos... */

/*******************************************************************
 * WMOS
 * weak MOS transistor
 * weak transistors: if source is < 128, drain gets 128
 * if source >= 128, drain gets source+1 (to keep levelization)
 * Note: levels of 128 and above are defined as weakly driven.
 *
 * Parameters:
 *	inst (musaInstance *) - the weak mos transistor instance. (Input)
 *	param (int16) - 1=>nmos 0=>pmos 2=>depl. (Input)
 */
int wmos(inst, mostype)
	musaInstance	*inst;
	int16	mostype;		/* 1=>nmos,  0=>pmos  2=>depl */
{
	int sout, dout;				    /* are source and drain outputs */
	register int sval, dval;	/* level of source and drain */
	int oval;					/* current level of output */
	long ison, oldison;			/* is transistor on */

	oldison = inst->data & IS_ON;
	if ( inst->schedule & IS_SCHED3  ) {
	    /* this is a sched3 call */
	    if ((inst->schedule & SKIP_SCHED3) != 0) return;
	    ison = is_active(&(inst->connArray[2]), mostype, inst->data & SUBTYPE) ? IS_ON : 0;
	    if (ison == oldison) {
		return 0; 		/* no change in state */
	    } 
	    inst->data ^= IS_ON;
	} else {
		/* this is a sched1 call -- this is different since the
		 * transistor may have been on, but NOT conducting as it should
		 */
		/* Any sched3 call is now unnecessary */
		inst->schedule |= SKIP_SCHED3;
		ison = is_active(&(inst->connArray[2]),mostype,inst->data & SUBTYPE) ? IS_ON : 0;
		if ((ison == 0) && (oldison == 0)) {
			/* network was off, and remains off -- boring */
			return;
		} /* if... */
		inst->data &= ~IS_ON;
		inst->data |= ison;
	} /* if... */

	sout = inst->connArray[0].hout;
	dout = inst->connArray[1].hout;
	/* sval and dval get level at node, unless node is a rail */
	sval = (inst->connArray[0].node->lowStrength == 0) ? 255 : inst->connArray[0].node->highStrength;
	dval = (inst->connArray[1].node->lowStrength == 0) ? 255 : inst->connArray[1].node->highStrength;

	if (ison) {
		if (sout) {
			if (sval <= dval) {
				/* source should not be output */
				musaUndriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
				sout = 0;
			} else {
				oval = inst->connArray[0].highStrength;
				if ((oval > 128) && (dval+1 < oval)) {
					/* increase drive (decrease level) */
					inst->connArray[0].highStrength = MAX(dval+1, 128);
					musaDriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
				} /* if... */
			} /* if... */
		} else if (dout) {
			if (dval <= sval) {
				/* drain should not be output */
				musaUndriveHigh(inst->connArray[1].node, &(inst->connArray[1]));
				dout = 0;
			} else {
				oval = inst->connArray[1].highStrength;
				if ((oval > 128) && (sval+1 < oval)) {
					/* increase drive (decrease level) */
					inst->connArray[1].highStrength = MAX(sval+1, 128);
					musaDriveHigh(inst->connArray[1].node, &(inst->connArray[1]));
				} /* if... */
			} /* if... */
		} /* if... */
		if (!sout && !dout) {
			/* determine which term (if any) to drive */
			if ((dval == sval) || ((dval >= 254) && (sval >= 254))) {
				goto lowdrive;
			}
			if (dval < sval) {
				if (inst->connArray[1].ttype != TERM_OUTPUT) {
					inst->connArray[0].highStrength = MAX(dval+1, 128);
					musaDriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
				} /* if... */
			} else {
				if (inst->connArray[0].ttype != TERM_OUTPUT) {
					inst->connArray[1].highStrength = MAX(sval+1, 128);
					musaDriveHigh(inst->connArray[1].node, &(inst->connArray[1]));
				} /* if... */
			} /* if... */
		} /* if... */
	} else {
		/* gate is off -- undrive if necessary */
		if (sout) {
			musaUndriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
		} else if (dout) {
			musaUndriveHigh(inst->connArray[1].node, &(inst->connArray[1]));
		} /* if... */
	} /* if... */

lowdrive:
	sout = inst->connArray[0].lout;
	dout = inst->connArray[1].lout;
	/* sval and dval get level at node, unless node is a rail */
	sval = (inst->connArray[0].node->highStrength == 0) ? 255 : inst->connArray[0].node->lowStrength;
	dval = (inst->connArray[1].node->highStrength == 0) ? 255 : inst->connArray[1].node->lowStrength;

	if (ison) {
		if (sout) {
			if (sval <= dval) {
				/* source should not be output */
				musaUndriveLow(inst->connArray[0].node, &(inst->connArray[0]));
				sout = 0;
			} else {
				oval = inst->connArray[0].lowStrength;
				if ((oval > 128) && (dval+1 < oval)) {
					/* increase drive (decrease level) */
					inst->connArray[0].lowStrength = MAX(dval+1, 128);
					musaDriveLow(inst->connArray[0].node, &(inst->connArray[0]));
				} /* if... */
			} /* if... */
		} else if (dout) {
			if (dval <= sval) {
				/* drain should not be output */
				musaUndriveLow(inst->connArray[1].node, &(inst->connArray[1]));
				dout = 0;
			} else {
				oval = inst->connArray[1].lowStrength;
				if ((oval > 128) && (sval+1 < oval)) {
					/* increase drive (decrease level) */
					inst->connArray[1].lowStrength = MAX(sval+1, 128);
					musaDriveLow(inst->connArray[1].node, &(inst->connArray[1]));
				} /* if... */
			} /* if... */
		} /* if... */
		if (!sout && !dout) {
			/* determine which term (if any) to drive */
			if ((dval == sval) || ((dval >= 254) && (sval >= 254))) {
				return;
			} /* if... */
			if (dval < sval) {
				if (inst->connArray[1].ttype != TERM_OUTPUT) {
					inst->connArray[0].lowStrength = MAX(dval+1, 128);
					musaDriveLow(inst->connArray[0].node, &(inst->connArray[0]));
				} /* if... */
			} else {
				if (inst->connArray[0].ttype != TERM_OUTPUT) {
					inst->connArray[1].lowStrength = MAX(sval+1, 128);
					musaDriveLow(inst->connArray[1].node, &(inst->connArray[1]));
				} /* if... */
			} /* if... */
		} /* if... */
	} else {
		/* gate is off -- undrive if necessary */
		if (sout) {
			musaUndriveLow(inst->connArray[0].node, &(inst->connArray[0]));
		} else if (dout) {
			musaUndriveLow(inst->connArray[1].node, &(inst->connArray[1]));
		} /* if... */
	} /* if... */
} /* wmos... */

/*********************************************************************
 * PULL
 * pullup or pulldown network -- uni-directional!!
 *
 * Parameters:
 *	inst (musaInstance) - the pullup or pulldown instance. (Input)
 *	param (int16) - 1=>nmos 0=>pmos. (Input)
 *	param2 (int16) - 0=>pulldown 1=>pullup. (Input)
 *	param3 (int16) - 1=>normal 128=>weak. (Input)
 */
int pull(inst, mostype, param2, param3)
	musaInstance	*inst;
	int16	mostype;		/* 1=>nmos  0=>pmos */
	int16	param2;		/* 0=>pulldown  1=>pullup */
	int16	param3;		/* 1=>normal  128=>weak */
{
	long	ison;		/* is transistor network on */
	long	oldison;	/* former ison */

	oldison = inst->data & IS_ON;

	ison = is_active(&(inst->connArray[1]), mostype, inst->data & SUBTYPE) ? IS_ON : 0;
	if (ison == oldison) {
		/* no change in state */
		return;
	} /* if... */
	inst->data ^= IS_ON;

	if (ison) {
		if (param2) {
			inst->connArray[0].highStrength = param3;
			musaDriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
		} else {
			inst->connArray[0].lowStrength = param3;
			musaDriveLow(inst->connArray[0].node, &(inst->connArray[0]));
		} /* if... */
	} else {
		if (param2) {
			musaUndriveHigh(inst->connArray[0].node, &(inst->connArray[0]));
		} else {
			musaUndriveLow(inst->connArray[0].node, &(inst->connArray[0]));
		} /* if... */
	} /* if... */
} /* pull... */

/*********************************************************************
 * MOS_RESTART
 * Routine to recover bidirectional mos devices after data read
 *
 * Parameters:
 *	inst (musaInstance *) - the mos instance. (Input)
 */
int mos_restart(inst)
    musaInstance *inst;
{
    inst->connArray[0].hout = inst->connArray[1].hout = 0;
    inst->connArray[0].lout = inst->connArray[1].lout = 0;
    inst_driveSensitiveSchedule(inst);
} 

/********************************************************************
 * PULL_RESTART
 * Routine to recover pulls after data read
 *
 * Parameters:
 *	inst (musaInstance *) - the pull up/down instance. (Input)
 */
int pull_restart(inst)
    musaInstance *inst;
{
    deviceType type;

    inst->connArray[0].hout = inst->connArray[0].lout = 0;
    type = inst->type;
    inst->data &= ~IS_ON;
    pull(inst, deviceTable[(int) type].param, deviceTable[(int) type].param2,
	 deviceTable[(int) type].param3);
}			

