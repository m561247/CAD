#include "port.h"
#include "utility.h"
#include "list.h"
#include "oct.h"
#include "oh.h"
#include "st.h"
#include "table.h"
#include "defs.h"

extern char* genmod();


noise2spice( fp, inst )
    FILE* fp;
    octObject *inst;
{
    octObject prop;

    prop.type = OCT_PROP;
    prop.contents.prop.name = "TF";
    
    if (ohGetByPropName(inst, &prop, "TF") == OCT_OK) {
	fprintf(fp, ".tf %s %s\n", 
		prop.contents.prop.value.string,
		deviceName( inst ));
    }

    if (ohGetByPropName (inst, &prop, "NOISE") == OCT_OK) {
	octObject value;
	OH_ASSERT(ohGetByPropName(&prop, &value, "VALUE" ));

	fprintf(fp, ".noise %s %s %s\n", 
		getPropValue( &prop ),
		deviceName( inst ),
		getPropValue( &value ) );
    }
}


depSource2spice( fp, inst )
    FILE* fp;
    octObject *inst;
{
    octObject net, master, celltype;

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));
    OH_ASSERT( ohGetByPropName( &master, &celltype, "CELLTYPE" ));
    
    if ( PROPEQ( &celltype, "CCCS" )) {

	char* src_name = genmod("i");
	getLegalDeviceName( inst, "f" );

        fprintf(fp, "%s %s %s 1\n",
		src_name,
		processInstTerm( inst, "CTOP", &net, "ctop" ),
		processInstTerm( inst, "CBOTTOM", &net, "cbottom" ));

	fprintf(fp, "%s %s %s %s %g\n",
		inst->contents.instance.name,
		processInstTerm( inst, "TOP", &net, "top" ),
		processInstTerm( inst, "BOTTOM", &net, "bottom" ),
		src_name );

	
    } else if (PROPEQ( &celltype, "CCVS")) {
	char *vs_name = genmod("v");

	getLegalDeviceName( inst, "h" );
        fprintf(fp, "%s %s %s 1\n",
		vs_name, 
		processInstTerm( inst, "CTOP", &net, "ctop" ),
		processInstTerm( inst, "CBOTTOM", &net, "cbottom" ));

	fprintf(fp, "%s %s %s %s",
		inst->contents.instance.name,
		processInstTerm( inst, "TOP", &net, "top" ),
		processInstTerm( inst, "BOTTOM", &net, "bottom" ),
		vs_name );
	table2spice( fp,  inst, depSourceTable );

	
    } else if (PROPEQ(&celltype, "VCCS") ||
	       PROPEQ(&celltype, "VCVS") ) {
	fprintf(fp, "%s %s %s %s %s",
		deviceName( inst ),
		processInstTerm( inst, "TOP", &net, "top" ),
		processInstTerm( inst, "BOTTOM", &net, "bottom" ),
		processInstTerm( inst, "CTOP", &net, "ctop" ),
		processInstTerm( inst, "CBOTTOM", &net, "cbottom" ));
	table2spice( fp,  inst, depSourceTable );
    }
}


indepSource2spice( fp, inst )
    FILE* fp;
    octObject *inst;
{
    octObject srctype, vdd, net, master;
    int vdd_ok;
    char* termname;
    octObject term;
    

    OH_ASSERT( ohOpenMaster( &master, inst, "interface", "r" ));
    OH_ASSERT( ohGetByPropName( &master, &srctype, "SOURCETYPE" ));

    
    if ( PROPEQ( &srctype, "CURRENT" ) ) {
	getLegalDeviceName( inst, "i" );
	fprintf(fp, "%s %s %s\n",
		deviceName( inst ),
		processInstTerm( inst, "TOP", &net, "top" ), 
		processInstTerm( inst, "BOTTOM", &net, "bottom" ));

	table2spice( fp, inst, sourceTable );
    } else if ( PROPEQ(&srctype, "VOLTAGEGRD") ) {
	OH_ASSERT( octGenFirstContent( inst, OCT_TERM_MASK, &term ));
	termname = term.contents.term.name;
	getLegalDeviceName( inst, "v" );
	fprintf(fp, "%s %s %s\n",
		deviceName( inst ),
		processInstTerm( inst, termname, &net, "top" ), getGround());
	table2spice( fp, inst, sourceTable );
    } else if ( PROPEQ(&srctype, "CURRENTGND" )) {
	OH_ASSERT( octGenFirstContent( inst, OCT_TERM_MASK, &term ));
	termname = term.contents.term.name;
	getLegalDeviceName( inst, "i" );
	fprintf(fp, "%s %s %s\n",
		deviceName( inst ),
		processInstTerm( inst, termname, &net, "top" ), getGround());
	table2spice( fp, inst, sourceTable );
    } else if (PROPEQ( &srctype, "VSUPPLY" ) ) {
	getLegalDeviceName( inst, "v" );
	vdd_ok = (ohGetByPropName(&master, &vdd, "VDDMODEL") == OCT_OK);
	if ( vdd_ok ) {
	    OH_ASSERT( octGenFirstContent( inst, OCT_TERM_MASK, &term ));
	    termname = term.contents.term.name;
	    fprintf(fp, "%s %s %s\n",
		    deviceName( inst ),
		    processInstTerm( inst, termname, &net, "top" ), getGround() );
	    table2spice( fp, inst, dcSourceTable );
	} else {
	    printf( "VSUPPLY is not VDDmodel\n" );
	}

    } else if ( PROPEQ( &srctype, "VOLTAGE" ) ) {
	getLegalDeviceName( inst, "v" );
	fprintf(fp, "%s %s %s\n",
		deviceName( inst ),
		processInstTerm( inst, "TOP", &net, "top" ), 
		processInstTerm( inst, "BOTTOM", &net, "bottom" ));
	table2spice( fp, inst, sourceTable );
    }

    noise2spice( fp, inst );
}
