/*
 * version.c
 *
 * The version string generator.
 */

#include "port.h"
#include "oct.h"

#define RPC_CRYSTAL_VERSION "1-0"

#if OCT_LEVEL == 2
#define RPC_LEVEL 3
#else
#define RPC_LEVEL 1
#endif

/*
 * version
 * 
 * Output the all-important version information.
 */
char *
version()
{
    static char string[256];

    (void) sprintf( string,
	"2This is rpc-crystal version %s (made %s with RPC library %d).\n0",
		   RPC_CRYSTAL_VERSION, __DATE__, RPC_LEVEL);
    return( string );
}
