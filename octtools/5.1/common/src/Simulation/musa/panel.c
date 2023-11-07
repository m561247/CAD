/***********************************************************************
 * Create the Musa panel window.
 * Filename: panel.c
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
 *	PIXEL GetColor() - allocate color.
 *	int16 MusaOpenXDisplay() - open x display.
 *	void HandleXEvents() - handle x events for panel and plot windows.
 *	void RPCHandleXEvents() - rpc version of x event handler.
 *	int16 StartPanel() - open panel window.
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define PANEL_MUSA
#include "musa.h"

/************************************************************************
 * GETCOLOR
 * Given a standard color name,  this routine allocated the color for
 * the display.  If it can't find or allocate the color,  it returns -1.
 *
 * Parameters:
 *	name (char *) - the name of the color. (Input)
 *
 * Return Value:
 *	The identifier for the specified color, if unable to allocate
 *	color then -1. (PIXEL)
 */
PIXEL GetColor(name)
	char	*name;
{
    XColor	def;
    
    if (!XParseColor(theDisp,
		     DefaultColormap(theDisp,DefaultScreen(theDisp)),name, &def)) {
	return(-1);
    }				/* if... */
    if (XAllocColor(theDisp,
		    DefaultColormap(theDisp,DefaultScreen(theDisp)), &def)) {
	return(def.pixel);
    } else {
	return(-1);
    }				/* if... */
}				/* GetColor... */

/*********************************************************************
 * MUSAOPENXDISPLAY
 * This routine tryies to open the X display.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	TRUE, if success.
 *	FALSE, unsuccessful.
 */

int16 MusaOpenXDisplay()
{
	static int16 tries = 0;

	if (theDisp) {
		return (TRUE);
	} /* if... */
    
	if (tries > 5) {
		return (FALSE);
	} /* if... */
	if ((theDisp = XOpenDisplay(displayName)) == NIL(Display)) {
		Warning("Cannot open the display.");
		tries++;
		return(FALSE);
	} /* if... */
	return(TRUE);
} /* MusaOpenXDisplay... */

/**********************************************************************
 * HANDLEXEVENTS
 * This routine handles any X events.
 *
 * Parameters:
 *	None.
 */
void HandleXEvents()
{
	XEvent	theEvent;
	lsGen	gen;
	musaInstance	*inst;

	if (!theDisp) {
		/*** if display is not open then don't handle events ***/
		return;
	} /* if... */
	while (XPENDING(theDisp) > 0) {
		XNEXTEVENT (theDisp,&theEvent);
		if (((XAnyEvent *) &theEvent)->window == panelWindow) {
			/*** Handle events for panel window ***/
			if ((((XAnyEvent *) &theEvent)->type == Expose) &&
				(((XExposeEvent *) &theEvent)->count == 0)) {
				gen = lsStart(ledList);
				while (lsNext(gen,(lsGeneric *) &inst,LS_NH) == LS_OK) {
					sim_led(inst);
				} /* while... */
				lsFinish(gen);
				gen = lsStart(segList);
				while (lsNext(gen,(lsGeneric *) &inst,LS_NH) == LS_OK) {
					sim_seg(inst);
				} /* while... */
				lsFinish(gen);
			} /* if... */
		} else {
			/*** Handle events for plots ***/
			(void) xgHandleEvent(&theEvent);
		} /* if... */
	} /* while... */
} /* HandleXEvents... */

#ifdef RPC_MUSA
/**********************************************************************
 * RPCHANDLEXEVENTS
 * Handle events in 'rpc-musa'
 *
 * Parameters:
 *	number (int) -
 *	read,write,except (int *) -
 */
void RPCHandleXEvents(number,read,write,except)
	int		number;
	int		*read,*write,*except;
{
	XEvent	theEvent;
	lsGen	gen;
	musaInstance	*inst;

	(void) ignore ((char *) &number);
	(void) ignore ((char *) read);
	(void) ignore ((char *) write);
	(void) ignore ((char *) except);

	if (!theDisp) {
		return;
	} /* if... */
	while (XPENDING(theDisp) > 0) {
		XNEXTEVENT (theDisp,&theEvent);
		if (((XAnyEvent *) &theEvent)->window == panelWindow) {
			if ((((XAnyEvent *) &theEvent)->type == Expose) &&
				(((XExposeEvent *) &theEvent)->count == 0)) {
				gen = lsStart(ledList);
				while (lsNext(gen,(lsGeneric *) &inst,LS_NH) == LS_OK) {
					sim_led(inst);
				} /* while... */
				lsFinish(gen);
				gen = lsStart(segList);
				while (lsNext(gen,(lsGeneric *) &inst,LS_NH) == LS_OK) {
					sim_seg(inst);
				} /* while... */
				lsFinish(gen);
			} /* if... */
		} else {
			(void) xgRPCHandleEvent(&theEvent);
		} /* if... */
	} /* while... */
} /* RPCHandleXEvents... */
#endif

/***********************************************************************
 * STARTPANEL
 * Open panel window.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	TRUE, success.
 *	FALSE, unsuccessful.
 */
int16 StartPanel()
{
    static int16			panelStarted = FALSE;
    XSetWindowAttributes	attr;
#ifdef RPC_MUSA
    int						zero = 0;
    int						readmask;
#endif

    if (panelStarted) {
	/*** Panel window already opened ***/
	return(TRUE);
    }				/* if...  */

    if (!MusaOpenXDisplay()) {
	/*** X display not opened ***/
	return(FALSE);
    }				/* if... */

    if (DISPLAYPLANES(theDisp) < 4) {
	/*** X display is black and white ***/
	bgPanelPixel = BLACKPIXEL(theDisp);
	fgPanelPixel = WHITEPIXEL(theDisp);
	bdPanelPixel = WHITEPIXEL(theDisp);
    } else {
	/*** X display is color ***/
	if ((bgPanelPixel = GetColor("DarkSlateGray")) == -1) {
	    bgPanelPixel = BLACKPIXEL(theDisp);
	}			/* if... */
	if ((fgPanelPixel =  GetColor("white")) == -1) {
	    fgPanelPixel = WHITEPIXEL(theDisp);
	}			/* if... */
	if ((bdPanelPixel =  GetColor("white")) == -1) {
	    bdPanelPixel = WHITEPIXEL(theDisp);
	}			/* if... */
    }				/* if... */
    attr.override_redirect = FALSE;
    attr.background_pixel = bgPanelPixel;
    attr.border_pixel = bdPanelPixel;
    panelWindow = XCreateWindow(theDisp,DefaultRootWindow(theDisp),
				3,3,700,300,3,CopyFromParent,CopyFromParent,CopyFromParent,
				CWBackPixel|CWBorderPixel|CWOverrideRedirect,&attr);
    XSelectInput (theDisp,panelWindow,ExposureMask);
    tile = XCreatePixmap(theDisp,panelWindow,10,10,DefaultDepth(theDisp,
								DefaultScreen(theDisp)));
    gc = DefaultGC(theDisp,DefaultScreen(theDisp));
    XSTORENAME(theDisp,panelWindow,"MUSA PANEL");
    XMAPWINDOW(theDisp,panelWindow);
    XFLUSH(theDisp);

#ifdef RPC_MUSA
    readmask = (1<<ConnectionNumber(theDisp));
    RPCUserIO(ConnectionNumber(theDisp)+1,&readmask,&zero,&zero,
	      RPCHandleXEvents);
#endif

    panelStarted = TRUE;
    return(TRUE);
}				/* StartPanel... */

