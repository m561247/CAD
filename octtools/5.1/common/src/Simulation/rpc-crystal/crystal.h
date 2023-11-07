/* crystal.h -
 *
 * Copyright (C) 1982 John K. Ousterhout
 * sccsid "@(#)crystal.h	2.21 (Berkeley) 10/14/88"
 *
 * This file contains overall definitions for Crystal, a timing
 * analysis program.
 */

#include "port.h"

#define TRUE 1
#define FALSE 0

/* There are three main elements in the data structure describing
 * a circuit network.  Objects of type Node correspond to the nets.
 * Each Node contains some intrinsic resistance and capacitance,
 * as well as pointers to all the transistors that connect to
 * that node.  The pointers to transistors are stored in a linked
 * list of Pointer objects.  The actual transistors are described in
 * FET objects.  Besides information describing the transistor,
 * each FET points to the nodes that connect to its source, gate,
 * and drain.
 *
 * There are also extra data structures besides those describing the
 * circuit network.  Objects of type Type contain information about a
 * particular class of transistor.  Flow and FlowPointer objects are
 * used to control bidirectional flow through pass transistor arrays.
 * Stage objects describe a single stage in the delay analysis, and
 * are linked together to describe paths through the circuit.
 */

typedef struct n1 {
    int n_flags;		/* Various flag bits, see below. */
    char *n_name;		/* Name of node:  watch out, since this
				 * dynamically-allocated string is also
				 * pointed to by the hash entry.
				  */
    float n_C;			/* Intrinsic capacitance. */
    float n_R;			/* Distributed resistance. */
    float n_hiTime;		/* Latest time when node can go hi. */
    float n_loTime;		/* Latest time when node can go low. */
				/* -1 for either of these times means node
				 * doesn't (yet) appear to change at all.
				 */
    struct p1 * n_pointer;	/* Head of pointer list. */
    int n_count;		/* Used for debugging purposes to find
				 * nasty loops.
				 */
    struct n1 *n_next;		/* Used to link all nodes together in the
				 * order allocated so we can search them
				 * all without gross memory thrashing.
				 */
    } Node;

/* Flag definitions:
 *  NODE0ALWAYS means that the node is always zero.
 *  NODE1ALWAYS means that the node is always one.
 *  NODEISINPUT means that the node is forced to zero or one by
 *	the outside world.  Nothing on-chip can change the node.
 *  NODEISOUTPUT means that the value of the node is used by
 *	the outside world.
 *  NODEISBUS means that the node is highly capacitive: any delay
 *	through the bus can be considered as a delay TO the bus
 *	followed by a separate delay FROM the bus.
 *  NODEINPATH is set during marking and delay tracing to avoid
 *	circular traces.
 *  NODEISPRECHARGED means that the node is set initially to one.
 *	Thus, there is never a zero->one transistion on it.
 *  NODEISPREDISCHARGED means that the node is set initially to zero.
 *	Thus, there is never a one-zero transition on it.
 *  NODEISDYNAMIC means that when all clocks are off, the node has
 *	no driver.  This is used in figuring out worst-case delays.
 *  NODERATIOERROR is set when a ratio error is found for this
 *	node.  It is used by the "ratio" command to avoid printing
 *	thousands of messages for a single badly-sized transistor.
 *  NODEISWATCHED means that the user has asked that we watch for
 *	slow delays on this node (and other watched nodes) and record
 *	the slowest in a separate list.
 */


#define	NODE0ALWAYS		     1
#define	NODE1ALWAYS		     2
#define	NODEISINPUT		     4
#define	NODEISBUS		   010
#define	NODEINPATH		   020
#define	NODEISPRECHARGED	   040
#define	NODEISDYNAMIC		  0100
#define	NODEISOUTPUT		  0400
#define	NODERATIOERROR		 01000
#define	NODEISPREDISCHARGED	 02000
#define	NODEISWATCHED		 04000
#define	NODEISBLOCKED		010000

typedef struct p1 {
    struct f1 *p_fet;		/* Pointer to transistor. */
    struct p1 *p_next;		/* Next pointer in list, or NULL. */
    } Pointer;

typedef struct f1 {
    short f_typeIndex;		/* Index of transistor type (must be 0-31). */
    short f_flags;		/* Random flag bits, see defs. below. */
    Node * f_source;		/* Pointer to nodes connecting */
    Node * f_drain;		/* to transistor. */
    Node * f_gate;
    float f_area;		/* Transistor area (square microns). */
    float f_aspect;		/* Transistor length/width. */
    float f_xloc;		/* Location of transistor (microns). */
    float f_yloc;
    struct f3 *f_fp;		/* Pointer to flow list, or NULL. */
    } FET;

/* System-defined transistor types:
 *
 * FET_NENH:	nMOS enhancement device.
 * FET_NENHP:	nMOS enhancement device driven by a pass transistor (no
 *		load or super-buffer pullup attached to its gate).
 * FET_NDEP:	nMOS depletion device other than FET_NLOAD or FET_NSUPER.
 * FET_NLOAD:	nMOS load (depletion device with one of source/drain
 *		connected to Vdd and the other connected to the gate).
 * FET_NSUPER:	nMOS super-buffer pullup (a depletion device with one
 *		of source/drain connected to Vdd and the other NOT
 *		connected to the gate).
 * FET_NCHAN:	CMOS n-channel device (this is essentially the same as
 *		FET_NENH, except for different resistance values).
 * FET_PCHAN:	CMOS p-channel device.
 */

#define FET_NENH 0
#define FET_NENHP 1
#define FET_NDEP 2
#define FET_NLOAD 3
#define FET_NSUPER 4
#define FET_NCHAN 5
#define FET_PCHAN 6

/* Transistor flag bits:
 *
 * FET_FLOWFROMSOURCE:	TRUE means that there is a source of 0 or 1
 *			on the source side of the transistor and a
 *			gate on the drain side of the transistor.
 * FET_FLOWFROMDRAIN:	TRUE means that there is a source of 0 or 1
 *			on the drain side of the transistor and a
 *			gate on the source side of the transistor.
 * FET_FORCEDON:	TRUE means the transistor has been forced on
 *			by logic simulation.
 * FET_FORCEDOFF:	TRUE means the transistor has been forced off
 *			by logic simulation.
 * FET_ONALWAYS:	A copy of the TYPE_ONALWAYS bit for the fet.
 * FET_ON0:		A copy of the TYPE_ON0 bit for the fet.
 * FET_ON1:		A copy of the TYPE_ON1 bit for the fet.
 * FET_NOSOURCEINFO:	TRUE means there was an "In" attribute attached
 *			to the drain or an "Out" attribute attached to
 *			the source.  This flag prevents the flag
 *			FET_FLOWFROMSOURCE from ever being set (it is
 *			used only in mark.c)
 * FET_NODRAININFO:	TRUE means there was an "Out" attribute attached
 *			to the drain or an "In" attribute attached to
 *			the source.  Similar to FET_NOSOURCEINFO.
 * FET_SPICE		Set during SPICE deck generation to indicate
 *			that this transistor will be output to the
 *			SPICE deck.
 */

/* Note: flags in the low byte must be in exactly the same position
 * as the corresponding TYPE flags!
 */

#define FET_ON0 1
#define FET_ON1 2
#define FET_ONALWAYS 4
#define FET_FLOWFROMSOURCE 0400
#define FET_FLOWFROMDRAIN 01000
#define FET_FORCEDON 02000
#define FET_FORCEDOFF 04000
#define FET_NOSOURCEINFO 010000
#define FET_NODRAININFO 020000
#define FET_SPICE 040000

/* Flow records are used to control information flow through
 * pass transistors, and are generated from attributes in the
 * .sim file.  Since each transistor may potentially have many
 * flow attributes, the flow records for each transistor are
 * linked together using objects of type FPointer.
 */

typedef struct f2 {
    char *fl_name;		/* Text name for flow attribute. */
    Node *fl_entered;		/* Non-NULL means info has flowed through
				 * a fet such that the info source was on
				 * the side of the transistor labelled with
				 * this attribute.  The value is the node
				 * from which info flowed into fet.
				 */
    Node *fl_left;		/* Has same meaning as at_entered, except info
				 * source was on the unnamed side of the fet.
				 */
    int fl_flags;		/* Flag bits, see below. */
    } Flow;

/* Flow flag bits:
 *
 *	FL_INONLY:	This is set by the "flow in" command, and means
 *			never allow information to flow into fet from
 *			the unnamed side of the transistor.
 *	FL_OUTONLY:	Set by "flow out", means never allow information
 *			to flow into fet from the named side of fet.
 *	FL_IGNORE:	Ignore this attribute altogether.
 *	FL_OFF:		Disallow any information flow through this FET at all.
 */

#define FL_INONLY 1
#define FL_OUTONLY 2
#define FL_IGNORE 4
#define FL_OFF 8

typedef struct f3 {
    struct f2 *fp_flow;		/* Pointer to flow record. */
    int fp_flags;		/* Flag bits, see below. */
    struct f3 *fp_next;		/* Next flow pointer. */
    } FPointer;

/* Flow pointer flag bits:
 *
 *	FP_SOURCE:	Flow attribute attached to FET source.
 *	FP_DRAIN:	Flow attribute attached to FET drain.
 */

#define FP_SOURCE 1
#define FP_DRAIN 2

/* The following data structure is used to keep track of a single
 * stage in a delay path.  A stage is a path from a Vdd or Ground
 * source through transistor channels to a destination node.  The
 * delay module collects information about the transistors and
 * nodes in the stage, then calls the model routines to compute
 * the actual delay.  Delay calculation for a stage is normally
 * stimulated by a change in the value of a transistor gate
 * somewhere along the stage.  This transistor is called the trigger.
 * Information about the stage is divided into two pieces: one piece
 * between the trigger and Vdd or Ground, including the trigger
 * (called piece1);  and one piece between the trigger and the
 * destination gate (called piece2).  Each piece is limited in length
 * to PIECELIMIT transistors in series.  Within each piece, the FETs
 * and nodes are in order emanating out from the trigger.  In piece1,
 * Node[i] is the one between FET[i] and FET[i+1] with the last node
 * being Vdd or GND or an input or a bus;  in piece2, FET[i] is
 * between Node[i] and Node[i+1].  The last node is the destination,
 * and the last FET in piece2 is NULL.  Some stages don't have
 * trigger transistors:  examples are stages being driven by loads
 * (the trigger is off to the side of the stage) and stages driven
 * by an input or bus through a pass transistor (the input or bus
 * triggers the change, but there's no trigger transistor).  In
 * these cases, Piece1Size is zero.
 *
 * Stages are linked together to record all the information about
 * a complete delay path through the circuit.
 */

/* This was changed to 50 from 5 so that Machester carry chains
 * could be exmained.
 * - fwo -      4/2/87
 */
#define PIECELIMIT 50
typedef struct s1 {
    int st_piece1Size;		/* Number of entries in piece1FET. */
    int st_piece2Size;		/* Number of entries in piece2FET. */
    struct s1 *st_prev;		/* Stage that caused the trigger to change. */
    int st_rise;		/* TRUE means stage is rising, FALSE means
				 * it's falling.
				 */
    float st_time;		/* Time when destination settles. */
    float st_edgeSpeed;		/* Absolute value of transition speed at
				 * destination, in ns/volt.  Used by the
				 * slope model.  0 means instantaneous
				 * rise or fall.
				 */
    FET *st_piece1FET[PIECELIMIT];
    Node *st_piece1Node[PIECELIMIT];
    FET *st_piece2FET[PIECELIMIT];
    Node *st_piece2Node[PIECELIMIT];
    } Stage;


/* Type records are used to describe the various classes of transistor.
 * The idea is to make this information technology-independent
 * enough to handle both nMOS and CMOS without any changes inside
 * Crystal.  Certain types of transistor are predefined (see the
 * types defined above).  The "strength" values indicate the
 * relative pulling power of a transistor.  Strength is used only
 * in logic simulation, to determine what wins when several fets
 * tug at the same node (e.g. loads have relatively low strength).
 */

#define MAXSLOPEPOINTS 10

typedef struct t1
{
    int t_flags;		/* Random flags, see below. */
    int t_strengthHi;		/* Transistor strength when pulling to 1. */
    int t_strengthLo;		/* Transistor strength when pulling to 0. */
    char *t_name;		/* Name of transistor type. */
    float t_cPerArea;		/* Capacitance between channel and gate. */
    float t_cPerWidth;		/* Overlap cap. between gate and s/d. */
    float t_rUp;		/* Resistance per square pulling up
				 * (used in the RC model).
				 */
    float t_rDown;		/* Resistance per square pulling down
				 * (used in the RC model). */
    int t_spiceBody;		/* Node number to use for body in SPICE
				 * decks.
				 */
    char t_spiceType;		/* One-character SPICE transistor type. */

    /* The following six arrays are used by the slope model for a better
     * approximation to transistor delays.  See slopemodel.c for details.
     */

    float t_upESRatio[MAXSLOPEPOINTS];
    float t_upREff[MAXSLOPEPOINTS];
    float t_upOutES[MAXSLOPEPOINTS];
    float t_downESRatio[MAXSLOPEPOINTS];
    float t_downREff[MAXSLOPEPOINTS];
    float t_downOutES[MAXSLOPEPOINTS];

} Type;

/* Maximum number of different types in the system. */

#define MAXTYPES 64

/* Type flags:
 *
 * TYPE_ON0:		TRUE means fet is on when the gate is zero.
 * TYPE_ON1:		TRUE means fet is on when the gate is one.
 * TYPE_ONALWAYS:	TRUE means fet is depletion device or some
 *			other kind of "always on" transistor.
 */

/* Note:  these bits must be in the same positions as the bits
 * in the FET flags!!!
 */

#define TYPE_ON0 1
#define TYPE_ON1 2
#define TYPE_ONALWAYS 4

/* The following stuff is used to indirect-call the procedure
 * to compute delays for the current model.
 */

extern int CurModel;
extern (*(ModelDelayProc[]))();

#define ModelDelay (*ModelDelayProc[CurModel])
