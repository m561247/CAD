/*
 * octStore.c
 *
 * Store crystal's critical paths in an Oct facet.
 */

#include "crystal.h"
#include "cOct.h"

/* the top level facet							*/
extern octObject oFacet;
extern st_table *OctIdTable;

/* the Crystal storage of the critical paths				*/
extern Stage **OctCritPaths;
extern int DPNumAnyPaths;
extern int DPMaxPaths;

/*
 * cOctStoreCrit
 *
 * Get or Create a crystal-critical-paths bag in the facet
 * and store the latest critical paths.
 */
cOctStoreCrit( flush)
int flush; /* boolean tells if flush should be done			*/
{
    int p;
    octObject oCritPathBag, oPathBag, oStageBag, oNet;
    octGenerator gBag;
    char bagname[ 20 ]; /* only allowed 5 crit paths now		*/
    Stage *stage;

    /* initialize bags							*/
    oPathBag.type = OCT_BAG;
    oCritPathBag.type = OCT_BAG;

    /* Get the critical path bag					*/
    oCritPathBag.contents.bag.name = "CRYSTAL_CRITICAL_PATHS";
    OH_ASSERT( octGetOrCreate( &oFacet, &oCritPathBag));

    for( p = DPNumAnyPaths - 1; p >= 0; --p){
	/* get the bag for this path					*/
	sprintf( bagname, "PATH%d", DPNumAnyPaths - p);
	oPathBag.contents.bag.name = bagname;
	OH_ASSERT( octGetOrCreate( &oCritPathBag, &oPathBag));

	/* get rid of any previous contents of this path bag		*/
	OH_ASSERT( octInitGenContents( &oPathBag, OCT_ALL_MASK, &gBag));
	while( octGenerate( &gBag, &oStageBag) == OCT_OK){
	    OH_ASSERT( octDelete( &oStageBag));
	}

	oStageBag.type = OCT_BAG;
	for( stage = OctCritPaths[ p ]; stage != NIL(Stage);
		stage = stage->st_prev){
	    sprintf( bagname, "%.2fns", stage->st_time);
	    oStageBag.contents.bag.name = bagname;
	    OH_ASSERT( octCreate( &oPathBag, &oStageBag));

	    if( st_lookup( OctIdTable,
			(char *)(stage->st_piece2Node[stage->st_piece2Size-1]),
			(char **)&(oNet.objectId)) == 0){
		continue;
	    }
	    OH_ASSERT( octGetById( &oNet));
	    if( oNet.type != OCT_NET){
		fprintf(stderr,"Crystal Oct reader: %s isn't a net\n",
			ohFormatName( &oNet));
		continue;
	    }
	    OH_ASSERT( octAttach( &oStageBag, &oNet));

	}
    }

    if( flush){
	OH_ASSERT( octFlushFacet( &oFacet));
    }
}
