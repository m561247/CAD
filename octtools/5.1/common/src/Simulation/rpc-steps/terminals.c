/* Notice: this file is common in rpc-steps and rpc-symhelp */
/* Make sure that if you change one, you change the other too */


#include "port.h"
#include "errtrap.h"
#include "rpc.h"    
#include "oh.h"
  
labelFormal( spotPtr, dir, tt )
    RPCSpot *spotPtr;
    char *dir;			/* Direction */
    char *tt;			/* Term type */
{
    octObject term, prop, net, contact;
    octGenerator genTerm;
    char buf[256];
    int count = 0;

    if ( vuFindSpot(spotPtr, &contact, OCT_INSTANCE_MASK) == VEM_OK ) {
	OH_ASSERT( octInitGenContents( &contact, OCT_TERM_MASK, &genTerm ));
	while ( octGenerate(&genTerm,&term) == OCT_OK ) {
	    if ( octGenFirstContainer( &term, OCT_TERM_MASK, &term ) != OCT_OK ) continue;
	    if ( octIdIsNull( term.contents.term.instanceId ) ) {
		count++;
		ohCreateOrModifyPropStr(&term,&prop,"DIRECTION", dir );
		ohCreateOrModifyPropStr(&term,&prop,"TERMTYPE", tt );
		sprintf( buf, "Term %s: %s %s\n",
			term.contents.term.name, dir, tt );
		VEMMSG( buf );

		/*
		 * Give a good name to the attached net, unless it already has a name.
		 * Also attach or update the important NETTYPE prop.
		 */

	    }
	    if ( octGenFirstContainer( &term, OCT_NET_MASK, &net ) == OCT_OK ) {
		if ( net.contents.net.name == 0 || strlen( net.contents.net.name ) == 0 ) {
		    net.contents.net.name = term.contents.term.name;
		    octModify( &net );
		}
	    }
	    sprintf( buf, "Net %s type %s\n", net.contents.net.name, tt );
	    VEMMSG( buf );
	    ohCreateOrModifyPropStr( &net, &prop, "NETTYPE", tt );

	}

	octFreeGenerator( &genTerm );

	if ( count == 0 ) {
	    VEMMSG( "No formal terminals in the selected instance.\n" );
	}
    } else {
	VEMMSG( "No instance found under the cursor.\n" );
    }
}

labelFormalInput( spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    labelFormal( spotPtr, "INPUT", "SIGNAL" );
    return RPC_OK;
}

labelFormalOutput( spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    labelFormal( spotPtr, "OUTPUT", "SIGNAL" );
    return RPC_OK;
}

labelFormalSupply( spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    labelFormal( spotPtr, "INPUT", "SUPPLY" );
    return RPC_OK;
}

labelFormalGround( spotPtr, cmdList, userOptionWord)
    RPCSpot *spotPtr;
    lsList cmdList;
    long userOptionWord;
{
    labelFormal( spotPtr, "INPUT", "GROUND" );
    return RPC_OK;
}

