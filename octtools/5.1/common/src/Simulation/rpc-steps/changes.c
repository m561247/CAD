
#include "port.h"
#include "rpc.h"
#include "oh.h"

extern void clearStepsAnnotation();





changesProcess( changeList )
    octObject *changeList;
{
    octObject cr, inst, facet;
    octGenerator gen;
    
    octGetFacet( changeList, &facet );

    OH_ASSERT(octInitGenContents( changeList, OCT_CHANGE_RECORD_MASK, &gen ));
    while ( octGenerate( &gen, &cr ) == OCT_OK ) {
	if ( octGetByExternalId( &facet, cr.contents.changeRecord.objectExternalId, &inst ) == OCT_OK ) {
	    /* fprintf( stderr, "Detected instance creation %s\n", ohFormatName( &inst ) ); */
	    clearStepsAnnotation( &inst );
	}
    }
    
    octDelete(changeList);


    changesStart( &facet );

}



changesStart( facet )
    octObject *facet;
{
    octObject changeList;
    changeList.type = OCT_CHANGE_LIST;
    changeList.contents.changeList.objectMask = OCT_INSTANCE_MASK;
    changeList.contents.changeList.functionMask = OCT_CREATE_MASK;

    OH_ASSERT(octCreate( facet, &changeList ));

    RPCRegisterDemon( &changeList, changesProcess );
}

