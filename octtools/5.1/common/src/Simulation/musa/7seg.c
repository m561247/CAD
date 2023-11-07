/***********************************************************************
 * Seven-Segment Display Module
 * Filename: 7seg.c
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
 *	static void ComputePixel() - Compute color intensity.
 *	static SEG *Init7Seg() - Initialize seven segment structure.
 *	int16 seg_read() - Read seven segment from oct database.
 *	int seg_info() - Display information about segment.
 *	int sim_seg() - Simulate segment.
 * Modifications:
 *	Andrea Casotto				Created						10/22/88
 *	Rodney Lai					Modified for X11			7/89
 *
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define SEG_MUSA
#include "musa.h"

static Font			segFont;
#define SEG_FONT	"courier8"

/*** Seven segment display terminals ***/
#define COMMON  0
#define DP      1
#define A       2
#define B       3
#define C       4
#define D       5
#define E       6
#define F       7
#define G       8

/*** Seven segment intensity level ***/
#define SEG_BRIGHT	0
#define SEG_DIM		1
#define SEG_DARK	2

/*** Seven segment structure ***/
typedef struct {
	char    *name;
	int     x,y;
	PIXEL   colorPixels[3];
} SEG;

static int16		segmentLength = 60;
static int16		segmentWidth = 10;

/***************************************************************************
 * COMPUTEPIXEL
 * Determine the color intensity.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 *	ss (SEG *) - the segment structure. (Input)
 *	term (int) - the terminal index. (Input)
 *	pixelP (PIXEL) - the color intensity. (Output)
 */
static void ComputePixel(inst, ss , term, pixelP)
	musaInstance	*inst;
	SEG		*ss;
	int		term;
	PIXEL		*pixelP;
{
    int		termVoltage,commonVoltage;
   
    termVoltage = inst->connArray[term].node->voltage & L_VOLTAGEMASK;
    commonVoltage = inst->connArray[COMMON].node->voltage & L_VOLTAGEMASK;

    if ((termVoltage == L_UNKNOWN) || (commonVoltage == L_UNKNOWN)) {
	*pixelP = ss->colorPixels[SEG_DIM];
    } else if ((termVoltage == L_HIGH) && (commonVoltage == L_LOW)) {
	*pixelP = ss->colorPixels[SEG_BRIGHT];
    } else {
	*pixelP = ss->colorPixels[SEG_DARK];
    }				/* if... */
}				/* ComputePixel... */

/***************************************************************************
 * BOX
 * Draw a box.
 *
 * Parameters:
 *	x, y (int) - the coordinates to draw box at. (Input)
 *	pixel (PIXEL) - the color to draw box. (Input)
 */
static box ( x, y, pixel)
	int		x,y;
	PIXEL	pixel;
{
	XSetForeground(theDisp,gc,pixel);
	XFillRectangle(theDisp,panelWindow,gc,x,y - 10,10,10);
} /* box... */

/**************************************************************************
 * HBOX
 * Draw a horizonal rectangle.
 *
 * Parameters:
 *	x, y (int) - the coordinates to draw box at. (Input)
 *	pixel (PIXEL) - the color to draw box. (Input)
 */
static hbox (x , y, pixel )
    int		x,y;			/*  lower Left Corner */
    PIXEL	pixel;
{
    int w = segmentWidth;
    int h = segmentLength;

    XSetForeground(theDisp,gc,pixel);
    XFillRectangle(theDisp,panelWindow,gc,x,y - w,h,w);
} /* hbox... */

/***************************************************************************
 * VBOX
 * Draw a vertical rectangle box.
 *
 * Parameters:
 *	x, y (int) - the coordinates to draw box at. (Input)
 *	pixel (PIXEL) - the color to draw box. (Input)
 */
static vbox ( x , y, pixel )
    int		x,y;			/*  lower Left Corner */
    PIXEL	pixel;
{
    int w = segmentWidth;
    int h = segmentLength;

    XSetForeground(theDisp,gc,pixel);
    XFillRectangle(theDisp,panelWindow,gc,x,y - h,w,h);
} 



/**********************************************************************
 * INIT7SEG
 * Initialize seven segment structure.
 *
 * Parameters:
 *	name (char *) - the name of the seven segment display. (Input)
 *	x,y (int) - the coordinates of the segment display in panel window. (Input)
 *	color (char *) - the color of the seven segment display. (Input)
 *
 * Return Value:
 *	The seven segment display structure. (SEG *)
 */
static SEG *Init7Seg(name, x, y, color)
	char	*name;
	int		x,y;
    char	*color;
{
    SEG		*newSeg;
    char	buffer[128];

    if (!StartPanel()) {
	return(NIL(SEG));
    }				/* if... */
  
    segFont = XLoadFont(theDisp,SEG_FONT);
    newSeg = ALLOC(SEG,1);
    newSeg->name = util_strsav(name);
    newSeg->x = x;
    newSeg->y = y;

    if (DISPLAYPLANES(theDisp) < 4) {
	newSeg->colorPixels[SEG_BRIGHT] = fgPanelPixel;
	newSeg->colorPixels[SEG_DIM] = fgPanelPixel;
	newSeg->colorPixels[SEG_DARK] = bgPanelPixel;
    } else {
	if (strcmp(color,"RED")==0) {
	    newSeg->colorPixels[SEG_BRIGHT] = GetColor("Red");
	    newSeg->colorPixels[SEG_DIM] = GetColor("#A88");
	    newSeg->colorPixels[SEG_DARK] = GetColor("#888");
	} else if (strcmp(color,"GREEN")==0) {
	    newSeg->colorPixels[SEG_BRIGHT] = GetColor("Green");
	    newSeg->colorPixels[SEG_DIM] = GetColor("#8A8");
	    newSeg->colorPixels[SEG_DARK] = GetColor("#888");
	} else if (strcmp(color,"YELLOW")==0) {
	    newSeg->colorPixels[SEG_BRIGHT] = GetColor("Yellow");
	    newSeg->colorPixels[SEG_DIM] = GetColor("#AA8");
	    newSeg->colorPixels[SEG_DARK] = GetColor("#888");
	} else {
	    (void) sprintf(buffer,"Unknown LED color (%s). Legal colors are RED GREEN YELLOW", color);
	    FatalError(color);
	}			/* if... */
	if (newSeg->colorPixels[SEG_BRIGHT] == -1) {
	    newSeg->colorPixels[SEG_BRIGHT] = fgPanelPixel;
	}			/* if... */
	if (newSeg->colorPixels[SEG_DIM] == -1) {
	    newSeg->colorPixels[SEG_DIM] = fgPanelPixel; 
	}			/* if... */
	if (newSeg->colorPixels[SEG_DARK] == -1) {
	    newSeg->colorPixels[SEG_DARK] = bgPanelPixel;
	}			/* if... */
    }				/* if... */
    return(newSeg);
}				/* initSeg... */


/**********************************************************************
 * SEG_READ
 * Read the seven segment display.
 *
 * Parameters:
 *	ifacet (octObject *) - the interface facet. (Input)
 *	inst (octObject *) - the contents facet. (Input)
 *	celltype (char *) - cell type. (Input)
 *
 * Return Value:
 *	TRUE, if instance was successfully read as a seven segment display. (Input)
 */
int16 seg_read(ifacet, inst, celltype)
	octObject	*ifacet, *inst;
	char		*celltype;
{
    musaInstance		*newmusaInstance;
    octObject	commonTerm, 
    dpTerm, aTerm, bTerm, cTerm, dTerm, eTerm, fTerm, gTerm;
    octObject	prop;
    int			inst_data = L_x; /* mask of instance data */
    octObject	color;
    octObject	label;

    if (strcmp(celltype, "I/O") != 0) {
	return(FALSE);
    }				/* if... */

    if (ohGetByPropName(ifacet, &prop, "I/O-MODEL") != OCT_OK) {
	(void) sprintf(errmsg,"%s:%s is of type I/O but it lacks the I/O-MODEL prop",ifacet->contents.facet.cell,ifacet->contents.facet.view);
	Warning(errmsg);
	return(FALSE);
    }				/* if... */

    if (strcmp(prop.contents.prop.value.string, "SEG") != 0) {
	return(FALSE);
    }				/* if... */

    if (ohGetByPropName(ifacet, &color, "SEG_COLOR") != OCT_OK) {
	color.contents.prop.value.string = "RED";
    }				/* if... */

    OH_ASSERT(ohGetByTermName(inst, &commonTerm, "COMMON"));    
    OH_ASSERT(ohGetByTermName(inst, &dpTerm, "DP"));    
    OH_ASSERT(ohGetByTermName(inst, &aTerm, "A"));    
    OH_ASSERT(ohGetByTermName(inst, &bTerm, "B"));    
    OH_ASSERT(ohGetByTermName(inst, &cTerm, "C"));    
    OH_ASSERT(ohGetByTermName(inst, &dTerm, "D"));    
    OH_ASSERT(ohGetByTermName(inst, &eTerm, "E"));    
    OH_ASSERT(ohGetByTermName(inst, &fTerm, "F"));    
    OH_ASSERT(ohGetByTermName(inst, &gTerm, "G"));    

    if (ohGetByPropName(inst, &label,"SEG_LABEL")!=OCT_OK) {
	label.contents.prop.value.string = inst->contents.instance.name;
    }				/* if... */
    
    make_leaf_inst(&newmusaInstance, DEVTYPE_7SEGMENT, inst->contents.instance.name, 9);

    newmusaInstance->userData = (char *) Init7Seg(label.contents.prop.value.string,
						  inst->contents.instance.transform.translation.x,
						  inst->contents.instance.transform.translation.y,
						  color.contents.prop.value.string);

    connect_term(newmusaInstance, &commonTerm, COMMON);
    connect_term(newmusaInstance, &dpTerm, DP);
    connect_term(newmusaInstance, &aTerm, A);
    connect_term(newmusaInstance, &bTerm, B);
    connect_term(newmusaInstance, &cTerm, C);
    connect_term(newmusaInstance, &dTerm, D);
    connect_term(newmusaInstance, &eTerm, E);
    connect_term(newmusaInstance, &fTerm, F);
    connect_term(newmusaInstance, &gTerm, G);

    newmusaInstance->data |= inst_data;
    lsNewEnd(segList,(lsGeneric) newmusaInstance,LS_NH);
    sim_seg(newmusaInstance);
    return(TRUE);
}				/* seg_read... */

/************************************************************************
 * SEG_INFO
 * Display information about sevent segment.
 *
 * Parameter:
 *	inst (musaInstance *) - the instance to display information for. (Input)
 */
int seg_info(inst)
	musaInstance	*inst;
{
    if (inst->type == DEVTYPE_7SEGMENT) { 
	MUSAPRINT("\n");
	ConnectInfo(inst, 9);
    }
}	

/***********************************************************************
 * SIM_SEG
 * Simulate seven segment display.
 * 
 * Parameters:
 *	inst (musaInstance *) - the instance to simulate. (Input)
 */
int sim_seg(inst)
	musaInstance	*inst;
{
    SEG		*ss = (SEG *) inst->userData;
    int		hw = segmentWidth / 2;
    int		l = segmentLength;
    PIXEL	pixel;

    if (!MusaOpenXDisplay()) {
	return;
    }				/* if... */

    if (inst->type == DEVTYPE_7SEGMENT) { 
	ComputePixel(inst,ss,A,&pixel);
	hbox(ss->x + 3 * hw , ss->y + 8 * hw , pixel);
	ComputePixel(inst,ss,B,&pixel);
	vbox(ss->x + 2 * hw + l , ss->y + 8 * hw + l , pixel);
	ComputePixel(inst,ss,C,&pixel);
	vbox(ss->x + 2 * hw + l , ss->y + 10 * hw + 2 * l , pixel);
	ComputePixel(inst,ss,D,&pixel);
	hbox(ss->x + 3 * hw , ss->y + 12 * hw + 2 * l , pixel);
	ComputePixel(inst,ss,E,&pixel);
	vbox(ss->x + 2 * hw , ss->y + 10 * hw + 2 * l , pixel);
	ComputePixel(inst,ss,F,&pixel);
	vbox(ss->x + 2 * hw , ss->y + 8 * hw +  l , pixel);
	ComputePixel(inst,ss,G,&pixel);
	hbox(ss->x + 3 * hw ,ss->y + 10 * hw +  l , pixel);
	ComputePixel(inst,ss,DP,&pixel);
	box(ss->x + 4 * hw + l ,ss->y + 12 * hw + 2 * l, pixel);
	XSetForeground(theDisp,gc,fgPanelPixel);
	XSetBackground(theDisp,gc,bgPanelPixel);
	XDrawImageString (theDisp,panelWindow,gc,
			  ss->x + l/2, ss->y + 14 * hw + 2 * l, ss->name, strlen(ss->name));
	XFLUSH(theDisp);
    }				/* if... */
}				/* sim_seg... */

