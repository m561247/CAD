#include "port.h"
#include "oct.h"
#include "oh.h"
#include "table.h"
#include "defs.h"
#include "utility.h"
#include "pep.h"

doSubtable( prop )
    octObject *prop;
{
    char *value = util_strsav(getPropValue( prop ));
    int rtn = 0;
    
    /* fprintf(stderr, "doSubtable: value = %s\n" , value ); */
    if ( !strncmp( value, "yes", 3 ) ) {
	rtn = 1;
    }
    return rtn;
}

char* getPropValue( prop )
    octObject* prop;
{
    static char buf[8][1024];
    static int count = 7;

    if ( --count == 0  ) count = 7;
    switch ( prop->contents.prop.type ) {
    case OCT_INTEGER:
	sprintf( buf[count], "%d", prop->contents.prop.value.integer );
	break;
    case OCT_REAL:
	sprintf( buf[count], "%s", PPP(prop->contents.prop.value.real) );
	break;
    case OCT_STRING:
	strcpy( buf[count],  prop->contents.prop.value.string );
	break;
    default:
	errRaise( "steps", 1, "Illegal prop type %s", ohFormatName( prop ) );
    }
    return buf[count];
}


void setPropValue( prop, value )
    octObject* prop;
    char* value;
{
    static char buf[1024];
    extern double atofe();

    switch ( prop->contents.prop.type ) {
    case OCT_INTEGER:
	prop->contents.prop.value.integer = atoi( value );
	break;
    case OCT_REAL:
	prop->contents.prop.value.real = atofe( value );
	break;
    case OCT_STRING:
	prop->contents.prop.value.string  = value;
	break;
    default:
	errRaise( "STEPS", 1, "Illegal prop value" );
    }
}



int tableSize( table )
    tableEntry *table;
{
    int count = 0;
    while ( table[count].propName != 0 ) count++ ;
    return count;
}

void fillTableEntry( tblPtr, i, propName, type, defaultValue, desc, width, spiceFormat, table )
    tableEntry *tblPtr;
    int i;
    char* propName;
    octPropType type;
    char* defaultValue;
    char* desc;
    int   width;		
    char* spiceFormat;		
    struct tableEntry *table;
{
    tblPtr[i].propName = propName;
    tblPtr[i].type = type;
    tblPtr[i].defaultValue = defaultValue;
    tblPtr[i].desc = desc;
    tblPtr[i].width  = width;		
    tblPtr[i].spiceFormat  = spiceFormat;		
    tblPtr[i].table = table;
}


void delTable(obj, tblPtr)
    octObject *obj;
    tableEntry *tblPtr;
{
    int i = 0;
    octObject prop;
    while ( tblPtr[i].propName ) {
	if (ohGetByPropName( obj, &prop, tblPtr[i].propName ) == OCT_OK ) {
	    octDelete( &prop );
	}
	i++;
    }
}


/* Additional properties of mosfets */
tableEntry mosfetICTable[] = {
    { "VDS", R, "0.0", "VDS", 10, "IC=(%s,", 0 },
    { "VGS", R, "0.0", "VGS", 10, "%s,", 0 },
    { "VBS", R, "0.0", "VBS", 10, "%s)", 0 },
    { NIL(char),0,NIL(char),NIL(char),0,NIL(char), 0}
};

tableEntry mosfetTable[] = {
    { "MOSFETWIDTH", R, "2e-6", "mosfet width", 10, "W=%s ", 0},
    { "MOSFETLENGTH", R, "1e-6", "mosfet length", 10, "L=%s ",  0 },
    { "MOSFETDRAINAREA", R , "0.0", "area drain", 10 , "\n+ AD=%s ", 0},
    { "MOSFETSOURCEAREA", R , "0.0", "area source", 10 , "AS=%s ", 0},
    { "MOSFETDRAINPERIM", R , "0.0", "perimeter drain", 10, "PD=%s ", 0 },
    { "MOSFETSOURCEPERIM", R , "0.0", "perimeter source", 10, "PS=%s ", 0},
    { "NRD", R , "0.0", "NRD", 10 , "NRD=%s ", 0},
    { "NRS", R , "0.0", "NRS", 10, "NRS=%s ", 0},
    { "OFF", S , "", "(''/OFF)", 10, "OFF ", 0},
    { "IC", S , "", "Initial conditions", 20, "", mosfetICTable },
    { "model", S, "", "Model name", 20, 0 , 0},
    { "device", S, "m", "Device name", 20, 0 , 0},
    { NIL(char),0,NIL(char),NIL(char),0,NIL(char), 0}
};
	

tableEntry jfetICTable[] = {
    { "VGS", R , "0.0", "", 10, "IC=(%s," , 0 },
    { "VDS", R , "0.0", "", 10, "%s)", 0 },
    { NIL(char),0,NIL(char),NIL(char),0,NIL(char), 0}
};

tableEntry jfetTable[] = {
    { "AREA", R, "10e-6", "jfet area", 10, "AREA=%s ", 0  },
    { "IC", S , "", "Initial conditions", 20, " IC=", jfetICTable },
    { "OFF", S , "", "Leave empty or say OFF", 10, "OFF ", 0 },
    { "model", S, "", "Model name", 20, 0, 0 },
    { "device", S, "j", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};
	



tableEntry bjtTable[] = {
    { "NS",  S,  "0",  "Substrate node" , 10, 0, 0 }, /* Does not show */
    { "X",   R,  "1", "BJT area X", 10, "%s ", 0 },
    { "VBE",  R,  "",  "VBE" , 10, "IC=%s", 0 },
    { "VCE",  R,  "",  "VCE" , 10, "%s", 0 },
    { "OFF", S , "", "Leave empty or say OFF", 10, "OFF ", 0 },
    { "model", S, "", "Model name", 20, 0, 0 },
    { "device", S, "q", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};


tableEntry diodeTable[] = {
    { "DIODEAREA",   R,  "100e-12", "Diode area X", 10, "%s", 0 },
    { "OFF", S , "", "Leave empty or say OFF", 10, "OFF", 0 },
    { "VD",  R,  "",  "VD" , 10, "IC=%s", 0 },
    { "model", S, "", "Model name", 20 , 0, 0},
    { "device", S, "d", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry resistorTable[] = {
    { "r",   R,  "1.0", "Resistance", 10, "%s ", 0 },
    { "device", S, "r", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};


tableEntry capacitorTable[] = {
    { "c",   R,  "1.0", "Capacity", 10, "%s" , 0 },
    { "device", S, "c", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry inductorTable[] = {
    { "l",   R,  "1.0", "Inductance area X", 10, "%s", 0 },
    { "device", S, "", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry transformerTable[] = {
    { "l1",   R,  "1.0", "Inductance 1", 10, 0, 0 },
    { "l2",   R,  "1.0", "Inductance 2 ", 10, 0, 0 },
    { "k",   R,  "1.0", "Mutual  2 ", 10, 0, 0 },
    { "model", S, "", "Model name", 20, 0, 0 },
    { "device", S, "", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry depSourceTable[] = {
    { "k",     R, "1.0", "Transfer coefficient", 10, "%s ", 0 },
    { "model", S,  ""  , "Model name", 20, 0, 0 },
    { "device", S, "", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry acSourceTable[] = {
    { "acmagnitude",  R,  "1.0", "AC magnitude", 10, "+ AC %s ", 0 },
    { "acphase",      R,  "0.0", "AC phase", 10, " %s ", 0 },
    {0,0,0,0,0,0,0}
};

tableEntry pulseTable[] = {
    { "pulseV1",   R,  "0.0", "Voltage 1", 10, "+ PULSE(%s ", 0 },
    { "pulseV2",   R,  "5.0", "Voltage 2", 10, "%s ", 0 },
    { "pulseTD",   R,  "0.0", "Delay time", 10, "%s ", 0 },
    { "pulseTR",   R,  "0.0", "Rise time", 10, "%s ", 0 },
    { "pulseTF",   R,  "0.0", "Fall time", 10, "%s ", 0 },
    { "pulsePW",   R,  "50n", "Pulse width", 10, "%s ", 0 },
    { "pulsePER",  R, "100n", "Period", 10, " %s) ", 0 },
    {0,0,0,0,0,0,0}
};

tableEntry pwlTable[] = {
    { "pwlT1",   R,  "0.0", "Time 1", 10, "+ PWL(%s ", 0 },
    { "pwlV1",   R,  "5.0", "Voltage 1", 10, "%s ", 0 },
    { "pwlT2",   R,  "1u", "Time 2 ", 10, " %s ", 0 },
    { "pwlV2",   R,  "0", "Voltage 2 ", 10, "%s ", 0 },
    { "pwl-other", S, "", "Other T V pairs", 20, "%s ", 0 }, /* Optional */
    { "---",  S,      "", "--keep this empty--", 10, " )", 0 }, /* So the parenthesis is always there */
    {0,0,0,0,0,0,0}
};



tableEntry dcSourceTable[] = {
    { "dc",   R,  "5.0", "DC value", 10, "+ DC %s ", 0 },
    /* { "model", S,  "", "Model name", 20, 0, 0 }, */
    { "device", S, "", "Device name", 20, 0 , 0},
    {0,0,0,0,0,0,0}
};

tableEntry outputTable[] = {
    { "C_LOAD",   R,  "10f",  "Load Capacitance To Ground", 10, 0, 0 },
    { "R_LOAD",   R,  "1000k", "Load Resistance  To Ground", 10, 0, 0 },
    { "L_LOAD",   R,  "-1", "Load Inductance  To Ground", 10, 0, 0 },
    {0,0,0,0,0,0,0}
};


tableEntry sinSourceTable[] = {
    { "sinV0",   R,  "0.0", "Offset (volts or amps)", 10, "+ SIN(%s ", 0 },
    { "sinVA",   R,  "1.0", "Amplitude (volts or amps)", 10, "%s ", 0 },
    { "sinFREQ", R,  "100MEG", "Frequency (Hz)", 10, " %s ", 0 },
    { "sinTD",   R,  "0", "Delay (seconds)", 10, "%s ", 0 },
    { "sinTHETA",R,  "1e10", "Damping factor (1/seconds)", 10, " %s)", 0 },
    /* { "model",   S,  "", "Model name", 20, 0, 0 }, */
    {0,0,0,0,0,0,0}
};

tableEntry expSourceTable[] = {
    { "expV1",   R,  "0.0", "Initial value", 10, "+ EXP(%s ", 0 },
    { "expV2",   R,  "5.0", "Pulsed value", 10, "%s ", 0 },
    { "expTD1",  R,  "0.0", "Rise delay time ", 10, " %s ", 0 },
    { "expTAU1", R,  "5.0", "Rise time constant ", 10, "%s ", 0 },
    { "expTD2",  R,  "0.0", "Fall delay time ", 10, " %s ", 0 },
    { "expTAU2", R,  "5.0", "Fall time constant ", 10, "%s)", 0 },
    /* { "model", S,  "", "Model name", 20, 0, 0 }, */
    {0,0,0,0,0,0,0}
};

tableEntry sffmSourceTable[] = {
    { "sffmV0",   R,  "0.0", "Offset (volts or amps)", 10, "+ SFFM(%s ", 0 },
    { "sffmVA",   R,  "1.0", "Amplitude (volts or amps)", 10, "%s ", 0 },
    { "sffmFC",   R,  "1MEG", "Carrier frequency (Hz)", 10, " %s ", 0 },
    { "sffmMDI",  R,  "5", "Modulation index", 10, "%s ", 0 },
    { "sffmFS",   R,  "1k", "Signal frequency (Hz)", 10, " %s) ", 0 },
    /* { "model",    S,  "", "Model name", 20, 0, 0 }, */
    {0,0,0,0,0,0,0}
};

tableEntry sourceTable[] = {
    { "dc_flag",   S,  "yes", "DC", 10, "", dcSourceTable },
    { "ac_flag",   S,  "",    "AC", 10, "", acSourceTable },
    { "pul_flag",  S,  "", "Pulse", 10, "", pulseTable },
    { "pwl_flag",  S,  "", "Piece-wise linear", 10, "", pwlTable },
    { "sin_flag",  S,  "", "Sinusoidal", 10, "", sinSourceTable },
    { "exp_flag",  S,  "", "Exponential", 10, "", expSourceTable },
    { "sffm_flag", S,  "", "Single frequency FM", 10, "", sffmSourceTable },
    { "device", S, "", "Device name (e.g.: `va' `ib')", 20, 0, 0 },
    {0,0,0,0,0,0,0}
};



/* These are the defaults models */
tableEntry modelTable[] = {
    { "npn", S,  "npn IS=1e-16 BF=100 VA=33", "npn", 60, "", 0 },
    { "pnp", S,  "pnp IS=1e-16 BF=100 VA=34", "pnp", 60, "", 0 },
    { "nmos", S, "nmos VTO=0.8 KP=7e-05",     "nmos", 60, "", 0 },
    { "pmos", S, "pmos VTO=-0.8 KP=3e-05",    "pmos", 60, "", 0 },
    { "diode", S,"d IS=1e-16",            "diode", 60, "", 0 },
    { "njfet", S,"njf VTO=2", 	      "njfet", 60, "", 0 },
    { "pjfet", S,"pjf VTO=-2", 	      "pjfet", 60, "", 0 },
    {0,0,0,0,0,0,0}
};




tableEntry optionsTable[] = {
    { "INITCONDS", S, "", "Initial conditions", 40, "\n.ic %s", 0 },
    { "NODESET", S, "", "nodeset", 40, "\n.nodeset %s", 0 },
    { "TEMP", R, "300.0", "temperature", 40, "\n.temp %s", 0 },
    { "WIDTH IN", I, "1000", "width input", 40, "\n.width in %s", 0 },
    { "WIDTH OUT", I, "80", "width output", 40, "\n.width in %s", 0 },
    {0,0,0,0,0,0,0}
};


tableEntry opAnalysisTable[] = {
    { "OP", S, "", "Just confirm OP analysis", 20, ".op \n", 0 },
    {0,0,0,0,0,0,0}
};


tableEntry dcAnalysisTable[] = {
    { "SRCNAM", S, "", "Source name", 20, ".dc %s ", 0 },
    { "VSTART", R, "0", "VSTART", 20, "%s ", 0 },
    { "VSTOP",  R, "5", "VSTOP",  20, "%s ", 0 },
    { "VINCR",  R, "0.1", "VINCR", 20, "%s ", 0 },
    {0,0,0,0,0,0,0}
};

tableEntry acAnalysisTable[] = {
    { "acVAR", S, "", "Variation (DEC,OCT,LIN)", 20, "\n.ac %s ", 0 },
    { "acN",   I, "10", "Points", 20, "%s ", 0 },
    { "acFSTART",  R, "1k", "Start Freqency",  20, "%s ", 0 },
    { "acFSTOP",   R, "1MEG", "Stop Frequency", 20, "%s", 0 },
    {0,0,0,0,0,0,0}
};

tableEntry tranAnalysisTable[] = {
    { "tranTSTEP",   R, "1n", "Step (seconds)", 10, ".tran %s ", 0 },
    { "tranTSTOP",   R, "100n", "Stop (seconds)", 10, "%s ", 0 },
    { "tranTSTART",  R, "0", "Start (seconds)",  10, "%s ", 0 },
    { "tranTMAX",    R, "", "Max step size (seconds)", 10, "%s ", 0 },
    { "tranUIC",     S, "", "Use initial conditions? (yes,'')", 10, "UIC", 0 },
    {0,0,0,0,0,0,0}
};



tableEntry analysisTable[] = {
    { "OP", I, "1", "Operating Point (yes/no)", 40, "", opAnalysisTable },
    { "DC", S, "", "DC Analysis (yes/no)", 10, "", dcAnalysisTable },
    { "AC", S, "", "AC Analysis (yes/no)", 10, "", acAnalysisTable },
    /* { "TF", S, "",  "Transfer function", 10, ".tf %s\n", 0 }, */
    /* { "SENS", S, "", "Sensitivity", 40, ".sens %s\n", 0 }, */
    /* { "DISTO",S, "", "Distorsion", 40, ".disto %s\n", 0 }, */
    { "TRAN", S, "", "Transient Analysis (yes/no)", 40, "", tranAnalysisTable },
    /* { "FOUR", S, "", "Fourier Analysis", 40, ".four %s\n", 0 },   */
    {0,0,0,0,0,0,0}
};

tableEntry printTable[] = {
    { "PRINTDC", S, "", "print dc (e.g. `v(1)' or `nodea')", 40, ".print dc %s\n", 0 }, 
    { "PRINTAC", S, "", "print ac (e.g. `v(1)' or `nodea')", 40, ".print ac  %s\n", 0 },
    { "PRINTTRAN", S, "", "print trans", 40, "\n.print tran %s", 0 },
    /* { "PRINTDISTO", S, "", "print disto", 40, "\n.print disto %s", 0 }, */
    /* { "PRINTNOISE",  S, "", "print noise", 40, "\n.print noise %s", 0 }, */
    { "PLOTDC", S, "", "plot dc", 40, "\n.plot dc %s", 0 }, 
    { "PLOTAC", S, "", "plot ac", 40, "\n.plot ac %s", 0 },
    { "PLOTTRAN", S, "", "plot tran", 40, "\n.plot tran %s", 0 },
    /* { "PLOTDISTO", S, "", "plot disto", 40, "\n.plot disto %s", 0 }, */
    /* { "PLOTNOISE",  S, "", "plot noise", 40, "\n.plot noise %s", 0 }, */
    { "USEROPTIONS", S, "", "Other (enter entire line)", 40, "\n%s", 0 },
    {0,0,0,0,0,0,0}
};


tableEntry connectivityTable[] = {
    {"Ground node",   S, "0",       "Ground node (name of node labelled 0)", 20, 0, 0},
    {"Psubstrate node", S , "",     "Name of P Substrate", 20, 0 , 0 },
    {"Nsubstrate node", S , "",     "Name of N Substrate", 20, 0 , 0 },
    {0,0,0,0,0,0,0}
};



tableEntry stepsTable[] = {
    {"Spice deck in", S, "%s.spi", "Spice file", 20, 0, 0},
    /* {"Spice deck out", S, "%s.out", "Spice file out", 20, 0, 0}, */
    {"User file",     S, "",        "Files to be included (comma separated)", 40, 0, 0 },
    {"CapThreshMin", R , "20f", "Smallest Cap", 20, 0, 0 },
    {"ResThreshMin", R , "2", "Smallest Res", 20, 0, 0 },
    {0,0,0,0,0,0,0}
};


tableEntry spiceDefaultsTable[] = {
    { "gmin", R, "1.0e-12", "gmin", 10, ".options gmin=%s ", 0 },
    { "reltol", R, "0.001", "reltol", 10, "reltol=%s ", 0 },
    { "abstol", R, "1.0e-12", "abstol", 10, "abstol=%s ", 0 },
    { "vntol", R, "1.0e-6", "vntol", 10, "vntol=%s ", 0 },
    { "trtol", R, "7.0", "trtol", 10, "\n.options trtol=%s ", 0 },
    { "chgtol", R, "1.0e-14", "chgtol", 10, "chgtol=%s ", 0 },
    { "pivtol", R, "1.0e-13", "pivtol", 10, "pivtol=%s ", 0 },
    { "pivrel", R, "1.0e-3", "pivrel", 10, "pivrel=%s ", 0 },
    { "numdgt", I, "4", "numdgt", 10, "\n.options numdgt=%s ", 0 },
    { "tnom", R, "27", "tnom", 10, "tnom=%s ", 0 },
    { "itl1", I, "100", "itl1", 10, "\n.options itl1=%s ", 0 },
    { "itl2", I, "50", "itl2", 10, "itl2=%s ", 0 },
/*    { "itl3", I, "4", "itl3", 10, "itl3=%s ", 0 },     */
    { "itl4", I, "10", "itl4", 10, "itl4=%s ", 0 },
/*    { "itl5", I, "5000", "itl5", 10, "itl5=%s ", 0 },  */
    { "cptime", I, "", "cptime", 10, "cptime=%s ", 0 },
/*    { "limtim", I, "2", "limtim", 10, "limtim=%s ", 0 },   */
/*    { "limpts", I, "201", "limpts", 10, "limpts=%s ", 0 }, */
/*    { "lvlcod", I, "2", "lvlcod", 10, "lvlcod=%s ", 0 },   */
/*    { "lvltim", I, "2", "lvltim", 10, "lvltim=%s ", 0 },   */
    { "method", S, "trapezoidal", "method", 10, "\n.options method=%s ", 0 },
    { "maxord", I, "2", "maxord", 10, "maxord=%s ", 0 },
    { "defl", R, "100.0u", "defl", 10, "\n.options defl=%s ", 0 },
    { "defw", R, "100.0u", "defw", 10, "defw=%s ", 0 },
    { "defad", R, "0.0", "defad", 10, "defad=%s ", 0 },
    { "defas", R, "0.0", "defas", 10, "defas=%s ", 0 },
    {0,0,0,0,0,0,0}
};



