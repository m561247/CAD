
/***********************************************************************
 * LED Module
 * Filename: led.c
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
 *	Led *InitLed() - initialize led.
 *	void DisplayLed() - display led to panel window.
 *	int16 led_read() - read led from oct database.
 *	int led_info() - display information about led.
 *	int sim_led() - simulate led.
 * Modifications:
 *	Andrea Casotto				Created					10/22/88
 *	Rodney Lai					Ported to X11			1989
 *								RPC interface routines
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define LED_MUSA
#include "musa.h"

static Font		ledFont;
#define LED_FONT "courier8"

/*** Led Terminals ***/
#define ANODE	0
#define CATHODE	1

/*** LED brightness levels ***/
#define LED_BRIGHT 0
#define LED_DIM    1
#define LED_DARK   2

/*** Led structure ***/
typedef struct {
    char	*name;
    int		x,y;
    PIXEL	colorPixels[3];
} Led;

/**********************************************************************
 * INITLED
 * Allocate and initialize the led structure.
 *
 * Parameters:
 *	name (char *) - the name of the led. (Input)
 *	x,y (int) - the position of the led in the panel window. (Input)
 *	color (char *) - the name of the led color. (Input)
 *
 * Return Value:
 *	The led structure. (Led *)
 */
Led *InitLed(name, x, y, color)
	char	*name;
	int		x,y;
    char	*color;
{
	Led		*newLed;

	if (!StartPanel()) {
		return(NIL(Led));
	} /* if... */

	ledFont = XLoadFont(theDisp,LED_FONT);

	newLed = ALLOC(Led,1);
	newLed->name = util_strsav(name);
	newLed->x = x;
	newLed->y = y;

	if (DISPLAYPLANES(theDisp) < 4) {
		newLed->colorPixels[LED_BRIGHT] = fgPanelPixel;
		newLed->colorPixels[LED_DIM] = fgPanelPixel;
		newLed->colorPixels[LED_DARK] = bgPanelPixel;
	} else {		
		if (strcmp(color,"RED")==0) {
			newLed->colorPixels[LED_BRIGHT] = GetColor("Red");
			newLed->colorPixels[LED_DIM] = GetColor("#A88");
			newLed->colorPixels[LED_DARK] = GetColor("#888");
		} else if (strcmp(color,"GREEN")==0) {
			newLed->colorPixels[LED_BRIGHT] = GetColor("Green");
			newLed->colorPixels[LED_DIM] = GetColor("#8A8");
			newLed->colorPixels[LED_DARK] = GetColor("#888");
		} else if (strcmp(color,"YELLOW")==0) {
			newLed->colorPixels[LED_BRIGHT] = GetColor("Yellow");
			newLed->colorPixels[LED_DIM] = GetColor("#AA8");
			newLed->colorPixels[LED_DARK] = GetColor("#888");
		} else {
			(void) sprintf(msgbuf,"Unknown LED color (%s). Legal colors are RED GREEN YELLOW", color);
			FatalError(msgbuf);
		} /* if... */
		if (newLed->colorPixels[LED_BRIGHT] == -1) {
			newLed->colorPixels[LED_BRIGHT] = fgPanelPixel;
		} /* if... */
		if (newLed->colorPixels[LED_DIM] == -1) {
			newLed->colorPixels[LED_DIM] = fgPanelPixel;
		} /* if... */
		if (newLed->colorPixels[LED_DARK] == -1) {
			newLed->colorPixels[LED_DARK] = bgPanelPixel;
		} /* if... */
	} /* if... */
	return(newLed);
} /* InitLed... */

/*************************************************************************
 * DISPLAYLED
 * Display LED in panel window.
 *
 * Parameters:
 *	led (Led *) - the LED structure. (Input)
 *	colorIndex (int) - the intensity of the color 
 * 		[LED_DIM,LED_BRIGHT,LED_DARK]. (Input)
 */
void DisplayLed(led,colorIndex)
    Led *led;
    int colorIndex;
{
	int x = led->x;
	int y = led->y;
	int r = 5;			/* radius of the LED */

	XSetForeground(theDisp,gc,fgPanelPixel);
	XSetBackground(theDisp,gc,bgPanelPixel);
	XDrawImageString(theDisp,panelWindow,gc,x + r + r, y, 
		led->name,strlen(led->name));
	XSetForeground(theDisp,gc,led->colorPixels[colorIndex]);
	XFillArc(theDisp,panelWindow,gc,x,y,r+r,r+r,0,360*64);
	XFLUSH(theDisp);
} /* DisplayLED... */

/**********************************************************************
 * LED_READ
 * Read LED from oct database.
 *
 * Parameters:
 *	ifacet (octObject *) - interface facet. (Input)
 *	inst (octObject *) - contents facet. (Input)
 *	celltype (char *) - cell type. (Input)
 *
 * Return Value:
 *	TRUE, facet is an led, read successful.
 *	FALSE, facet is not an led, read unsuccessful.
 */
int16 led_read(ifacet, inst, celltype)
	octObject	*ifacet, *inst;
	char		*celltype;
{
	musaInstance		*newmusaInstance;
	octObject	anodeTerm, cathodeTerm;
	octObject	prop;
	int			inst_data = L_x;	/* mask of instance data */
	octObject	color;
	octObject	label;

	if (strcmp(celltype, "I/O") != 0) {
		return(FALSE);
	} /* if... */

	if (ohGetByPropName(ifacet, &prop, "I/O-MODEL") != OCT_OK) {
		(void) sprintf(errmsg,"%s:%s is of type I/O but it lacks the I/O-MODEL prop",ifacet->contents.facet.cell,ifacet->contents.facet.view);
		Warning(errmsg);
		return(FALSE);
	} /* if... */

	if (strcmp(prop.contents.prop.value.string, "LED") != 0) {
		return(FALSE);
	} /* if... */

	if (ohGetByPropName(ifacet, &color, "LED_COLOR") != OCT_OK) {
		color.contents.prop.value.string = "RED";
	} /* if... */

	OH_ASSERT(ohGetByTermName(inst, &anodeTerm, "ANODE"));    
	OH_ASSERT(ohGetByTermName(inst, &cathodeTerm, "CATHODE"));    

	if (ohGetByPropName(inst, &label,"LED_LABEL")!=OCT_OK) {
		label.contents.prop.value.string = inst->contents.instance.name;
	} /* if... */
    
	make_leaf_inst(&newmusaInstance, DEVTYPE_LED, inst->contents.instance.name, 2);

	newmusaInstance->userData = (char *) InitLed(label.contents.prop.value.string,
		inst->contents.instance.transform.translation.x,
		inst->contents.instance.transform.translation.y,
		color.contents.prop.value.string);

	connect_term(newmusaInstance, &anodeTerm, ANODE);
	connect_term(newmusaInstance, &cathodeTerm, CATHODE);

	newmusaInstance->data |= inst_data;
	lsNewEnd(ledList,(lsGeneric) newmusaInstance, LS_NH);
	sim_led(newmusaInstance);
	return(TRUE);
} /* led_read... */

/************************************************************************
 * LED_INFO
 * Display information if instance is an led.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
int led_info(inst)
	musaInstance	*inst;
{
	int anodeVoltage;
	int cathodeVoltage;

	if (inst->type == DEVTYPE_LED) { 
		anodeVoltage = inst->connArray[ANODE].node->voltage & L_VOLTAGEMASK;
		cathodeVoltage = inst->connArray[CATHODE].node->voltage & L_VOLTAGEMASK;

		if (anodeVoltage == L_HIGH && cathodeVoltage == L_LOW) {
			MUSAPRINT("BRIGHT\n");
		} else {
			MUSAPRINT("DARK\n");
		} /* if... */
		ConnectInfo(inst, 2);
	} /* if... */
} /* led_info... */

/***********************************************************************
 * SIM_LED
 * Simulate led if instance is an led.
 *
 * Parameters:
 *	inst (musaInstance *) - the instance. (Input)
 */
int sim_led(inst)
	musaInstance	*inst;
{
	Led		*led = (Led *) inst->userData;
	int		anodeVoltage;
	int		cathodeVoltage;

	if (!MusaOpenXDisplay()) {
		return;
	} /* if... */
	if (inst->type == DEVTYPE_LED) { 
		anodeVoltage = inst->connArray[ANODE].node->voltage & L_VOLTAGEMASK;
		cathodeVoltage = inst->connArray[CATHODE].node->voltage & L_VOLTAGEMASK;

		if ((anodeVoltage == L_UNKNOWN) || (cathodeVoltage == L_UNKNOWN)) {
			DisplayLed(led,LED_DIM);
		} else if ((anodeVoltage == L_HIGH) && (cathodeVoltage == L_LOW)) {
			DisplayLed(led,LED_BRIGHT);
		} else {
			DisplayLed(led,LED_DARK);
		} /* if... */
	} /* if... */
} /* sim_led... */


