/*   STEPS - Schematic Text Entry Package utilizing Schematics
 * 
 */

#include "port.h"
#include "oct.h"
#include "oh.h"
#include "table.h"
#include "defs.h"


tableEntry spiceOptionsTable2[] = {
    {"acct", I , "0", 	"acct",   0, ".options acct ", 0},
    {"list", I , "0", 	"list",   0, "list " , 0 },
    {"nopage", I , "0",	"nopage", 0, "nopage ", 0 },
    {"nomod", I , "0", 	"nomod",  0, "nomod ", 0 },
    {"node", I , "0", 	"node",	  0, "node ", 0 },
    {"opts", I , "0", 	"opts",   0, "opts ", 0	},
    {0,0,0,0,0,0,0}
};


int spiceOptions(facet, optBag)
    octObject	*facet, *optBag;
/*
 */
{
    octObject stepsBag;
    
    OH_ASSERT( ohGetOrCreateBag( facet, &stepsBag, "STEPS" ) );
    OH_ASSERT( ohGetOrCreateBag( &stepsBag, optBag, "SPICE_OPTIONS" ) );
    return stepsMultiText( optBag, "Spice Options", spiceDefaultsTable );
}


int spiceOptions2(facet, bag)
octObject	*facet, *bag;
/*
 */
{
    octObject stepsBag;
    OH_ASSERT( ohGetOrCreateBag( facet, &stepsBag, "STEPS" ) );
    OH_ASSERT( ohGetOrCreateBag( &stepsBag, bag, "SPICE_OPTIONS" ) );
    return stepsMultiWhich( bag, "Select SPICE options", spiceOptionsTable2, 0);
}







