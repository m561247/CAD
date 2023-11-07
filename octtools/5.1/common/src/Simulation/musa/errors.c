/***********************************************************************
 * Error Reporting and Message Routines
 * Filename: errors.c
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
 *	void FatalError() - Fatal error message handler.
 *	void FatalOctError() - Fatal Oct error message handler.
 *	void Warning() - Warning message handler.
 *	void Format() - Rpc command format message handler.
 *	void Usage() - Command usage message handler.
 *	void VerboseWarning() - Verbose warning message handler.
 *	void InternalError() - Internal error message handler.
 *	void Message() - Message handler.
 *	void VerboseMessage() - Verbose message handler.
 * Modifications:
 *	Rodney Lai				Commented						1989
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define ERRORS_MUSA
#include "musa.h"

/***********************************************************************
 * FATALERROR
 * Print fatal error message and exit.
 *
 * Parameters:
 *	es (char *) - error message. (Input)
 */
void FatalError(es)
	char *es;			/* error string */
{
	(void) fprintf(stderr,"\n\n%s: Fatal Error: %s\n" , progName, es);
	(void) fprintf(stderr,"\n\tI am sorry that I have to abort due to this serious error.");
	(void) fprintf(stderr,"\n\tPlease correct the problem and try again soon.");
	(void) fprintf(stderr,"\n");
#ifdef RPC_MUSA
	RPCExit(0);
#else
	VOVend(2);
#endif
} /* FatalError... */

/************************************************************************
 * FATALOCTERROR
 * Print fatal oct error and exit.
 *
 * Parameters:
 *	es (char *) - error message. (Input)
 */
void FatalOctError(es)
     char *es;			/* error string */
{
    (void) fprintf(stderr,"\n\n%s: Fatal Error: %s\n" , progName, es);
    octError("Last OCT error");
    (void) fprintf(stderr,"\n\tI am sorry that I have to abort due to this OCT error.");
    (void) fprintf(stderr,"\n\tPlease check the data base and try again soon.");
    (void) fprintf(stderr,"\n");
#ifdef RPC_MUSA
    RPCExit(0);
#else
    VOVend(-1);
#endif
}				/* FatalOctError... */

/***********************************************************************
 * WARNING
 * Print warning message.
 *
 * Parameters:
 *	es (char *) - warning message. (Input)
 */
void Warning(es)
	char *es;			/* error string */
{
#ifdef RPC_MUSA
    (void) sprintf(msgbuf, "2%s0: Warning: %s\n", progName, es);
    VEMMSG(msgbuf);
    vemPrompt();
#else
    (void) fprintf(stdout, "%s: Warning: %s\n", progName, es);
#endif
    if (output_file != NIL(FILE)) {
	(void) fprintf(output_file,"%s: Warning: %s\n",progName,es);
    }
}	

#ifdef RPC_MUSA
/***********************************************************************
 * FORMAT
 * Print RPC command format.
 *
 * Parameters:
 *	name (char *) - the command name. (Input)
 */
void Format(name,cominfo)
	char	*name;
	COMINFO	*cominfo;
{
    (void) sprintf(msgbuf, "Format: %s : %s\n",cominfo->format, name);
    VEMMSG(msgbuf);
    vemPrompt();
    if (output_file != NIL(FILE)) {
	(void) fputs(msgbuf,output_file);
    }
} 

#else

/***********************************************************************
 * USAGE
 * Print command usage message.
 *
 * Parameters:
 *	es (char *) - usage message. (Input)
 */
void Usage(es)
	char *es;			/* error string */
{
#ifdef RPC_MUSA
    (void) sprintf(msgbuf, "2%s0: Usage: %s\n", progName, es);
    VEMMSG(msgbuf);
    vemPrompt();
#else
    (void) fprintf(stdout, "%s: Usage: %s\n", progName, es);
#endif
    if (output_file != NIL(FILE)) {
	(void) fprintf(output_file,"%s: Usage: %s\n",progName,es);
    }
}	
#endif

/***********************************************************************
 * VERBOSEWARNING
 * Print a verbose warning.
 *
 * Parameters:
 *	es (char *) - the warning message. (Input)
 */
void VerboseWarning(es)
	char *es;			/* error string */
{
    if (verbose) {
	Warning(es);
    }	
}	

/***********************************************************************
 * INTERNALERROR
 * Print internal error message and exit.
 *
 * Parameters:
 *	msg (char *) - the error message. (Input)
 */
void InternalError(msg)
	char *msg;
{
    (void) fprintf(stderr, "Internal Error: %s\n", msg);
#ifdef RPC_MUSA
    RPCExit(0);
#else
    VOVend(-2);
#endif
}				/* InternalError... */

/***********************************************************************
 * MESSAGE
 * Print message.
 *
 * Parameters:
 *	msg (char *) - message string. (Input)
 */
void Message(msg)
	char *msg;
{
#ifdef RPC_MUSA
    (void) sprintf(msgbuf,"%s: Message: %s\n", progName, msg);
    VEMMSG(msgbuf);
    vemPrompt();
#else
    (void) fprintf(stdout, "%s: Message: %s\n", progName, msg);
#endif
    if (output_file != NIL(FILE)) {
	(void) fprintf(output_file,"%s: Message: %s\n",progName,msg);
    }				
}				

/**********************************************************************
 * VERBOSEMESSAGE
 * Print verbose message.
 *
 * Parameters:
 *	msg (char *) - the message. (Input)
 */
void VerboseMessage(msg)
	char *msg;
{
	if (verbose) {
		Message(msg);
	} /* if... */
} /* VerboseMessage... */

