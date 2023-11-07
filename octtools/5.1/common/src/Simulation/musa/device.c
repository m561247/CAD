/***********************************************************************
 * Device tables
 * Filename: device.c
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

#define DEVICE_MUSA
#include "musa.h"

/********************************************************************
 * NULL_ROUTINE
 */
int null_routine(inst)
    musaInstance *inst;
{
    return((int) inst);		/* make lint and saber shutup */
} /* null_routine... */

/***********************************************************************
 * IGNORE
 * A dumb routine to ignore some command line arguments
 * This is used only to make lint and saber shutup
 */
ignore(p)
    char *p;
{
    return((int) p);	
} /* ignore... */

/* declarations for terminal lists */
struct term_struct mos_terms [] = {
    "SOURCE", TERM_DLSIO,
    "DRAIN", TERM_DLSIO,
    "GATE", TERM_VSI,
};
struct term_struct pull_terms [] = {
    "OUTPUT", TERM_OUTPUT,
    "GATE", TERM_VSI,
};
struct term_struct logic_terms [] = {
    "OUTPUT", TERM_OUTPUT, 
    "INPUT", TERM_VSI,
};
struct term_struct latch_terms [] = {
    "CONTROL", TERM_VSI,
    "INPUT", TERM_VSI, 
    "OUTPUT", TERM_OUTPUT,
};

struct term_struct S_latch_terms [] = {
    "CONTROL", TERM_VSI,
    "SET", TERM_VSI,
    "INPUT", TERM_VSI,
    "OUTPUT", TERM_OUTPUT
};
/* Reset latch */
struct term_struct R_latch_terms [] = {
    "CONTROL", TERM_VSI,
    "RESET", TERM_VSI,
    "INPUT", TERM_VSI,
    "OUTPUT", TERM_OUTPUT
};
/* Set/Reset latch */

struct term_struct OneOutLatch [] = {
    "OUT" , TERM_OUTPUT,
    "IN"  , TERM_VSI
};

struct term_struct TwoOutLatch [] = {
    "OUT" , TERM_OUTPUT,
    "OUT2", TERM_OUTPUT,
    "IN"  , TERM_VSI
};

struct term_struct SR_latch_terms [] = {
    "CONTROL", TERM_VSI,
    "SET", TERM_VSI,
    "RESET", TERM_VSI,
    "INPUT", TERM_VSI,
    "OUTPUT", TERM_OUTPUT
};

struct term_struct tristate_terms [] = {
    "CONTROL", TERM_VSI,
    "INPUT", TERM_VSI,
    "OUTPUT", TERM_OUTPUT
};

struct term_struct ram_terms [] = {
    "READ" , TERM_VSI,
    "WRITE", TERM_VSI,
    "TERM" , TERM_VSI
  };

struct term_struct led_terms [] = {
    "ANODE", TERM_DSI,
    "CATHODE", TERM_DSI
  };

struct term_struct plot_terms [] = {
    "PROBE", TERM_DSI 
  };

struct term_struct seg_terms [] = {
    "COMMON",TERM_DSI,"DP",TERM_DSI,
	"A",TERM_DSI,"B",TERM_DSI,"C",TERM_DSI,"D",TERM_DSI,"E",TERM_DSI,
	"F",TERM_DSI,"G",TERM_DSI
  };
	/****************************************
	 *	Add new Terminal declarations here	*
	 ****************************************/


struct device_struct deviceTable[] = {
	"MODULE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/* bidirection transistor networks */
	"NMOS", 2, mos_terms, mos, 1, -1, -1, mos_info, null_routine,
	    mos_restart, null_routine, null_routine,
    "WNMOS", 2, mos_terms, wmos, 1, -1, -1, mos_info, null_routine,
	    mos_restart, null_routine, null_routine,
    "PMOS", 2, mos_terms, mos, 0, -1, -1, mos_info, null_routine,
	    mos_restart, null_routine, null_routine,
    "WPMOS", 2, mos_terms, wmos, 0, -1, -1, mos_info, null_routine,
	    mos_restart, null_routine, null_routine,
    /* depletion NMOS is special cased in the code to be WNMOS
     * that is always on */

    /* pullups and pulldowns are generated during readin */
    "N pulldown", 1, pull_terms, pull, 1, 0, 1, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "WN pulldown", 1, pull_terms, pull, 1, 0, 128, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "P pulldown", 1, pull_terms, pull, 0, 0, 1, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "WP pulldown", 1, pull_terms, pull, 0, 0, 128, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "N pullup", 1, pull_terms, pull, 1, 1, 1, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "WN pullup", 1, pull_terms, pull, 1, 1, 128, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "P pullup", 1, pull_terms, pull, 0, 1, 1, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,
    "WP pullup", 1, pull_terms, pull, 0, 1, 128, mos_info, 
	    inst_normalSchedule, pull_restart, null_routine, null_routine,

/* Instance type DEVTYPE_LOGIC_GATE declared by logic_read */
    "Logic gate", 1, logic_terms, logic, 0, 0, 0, logic_info,
	    inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,

    "Latch", 2,  0, sim_latch, 0, 0, 0, latch_info,
	    inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,
    "Tristate Buffer", 2, tristate_terms, sim_tristate, 0, 0, 0, tristate_info,
	    inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,
    "Directional Pass Gate", 2, tristate_terms, sim_tristate, 0, 0, 0, tristate_info,
	    inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,
            
    "RAM" , 2 , ram_terms, sim_ram , 0 , 0 , 0 , ram_info,
            null_routine, null_routine, null_routine, null_routine, 
    "LED" , 2 , led_terms , sim_led , 0 , 0 , 0 , led_info ,
            inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,
    "PLOT_PROBE", 1 , plot_terms , sim_plot, 0 , 0 , 0 , plot_info,
            null_routine, null_routine, null_routine, null_routine, 
	"Seven Segment" , 9, seg_terms, sim_seg , 0 , 0 , 0 , seg_info,
			inst_normalSchedule, inst_normalSchedule, null_routine, null_routine,
};

