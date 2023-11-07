
/***********************************************************************
 * Interactive command line for Musa.
 * Filename: inter.c
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
 *	void InterruptRoutine() - interrupt routine.
 *	void InterInit() - initialize iteractive command line.
 *	void my_print() - print to screen and log file if opened.
 *	void Help() - 'help' command.
 *	static void parse() - parse command line.
 *	int my_getcharX() - get char from 'stdin' and check X event queue.
 *	int my_getchar() - get char from 'stdin' and if X display open then
 *		check X event queue.
 *	int my_getc() - get char from 'stdin' or from file.
 *	int16 LineCOntinues - check if line end in '\'.
 *	int getline() - get a line.
 *	static int NextSpace() - skip to white space.
 *	static int NextQuoteSpace() - skip to next quote followed by white space.
 *	static int NextChar() - skip to next character.
 *	void LOpen() - open log file.
 *	int rpcMusaLOpen() - rpc interface to 'lopen' command.
 *	void LClose() - close log file.
 *	int rpcMuasLClose() - rpc interface to 'lclose' command.
 *	void DisplaySourceStatus() - display source flags.
 *	void PContinue() - continue evaluation.
 *	void Source() - source a command file.
 *	int rpcMusaSource() - rpc interface to 'source' command.
 *	void PStep() - single step command file.
 *	void Macro() - 'macro' command.
 *	static int SubstParams() - substitue parameters into macros.
 *	int execute() - execute command.
 *	int musaInteractiveLoop() - interactive command line.
 * Modifications:
 *	Albert Wang				Stolen from 'mis'			??/??/??
 ***********************************************************************/

#define INTER_MUSA
#include <sys/time.h>
#include "musa.h"
#include "vov.h"
#include "simstack.h"

#define MAXLINE 5000
#define MAXARGU 260


macdef *macros = NIL(macdef);

/************************************************************************
 * INTERRUPTROUTINE
 * Interrupt routine, if ctrl-C is entered, program is not terminated
 * but a flag in 'sim_status' is set.
 *
 * Parameters:
 *	None.
 */
void InterruptRoutine()
{
    sim_status |= SIM_INTERRUPT;
} /* InterruptRoutine... */

/************************************************************************
 * INTERINIT
 * Initialize interactive command line.
 *
 * Parameters:
 *	None.
 */
void InterInit()
{
    char	buf[2];
    int16	junk;

    buf[0] = separator;
    buf[1] = '\0';
    (void) gnInstPath(buf, &cwd, TRUE, &junk);/* initialize to root instance */

    /* set up interrupts */
    (void) signal(SIGINT, InterruptRoutine);

    /* set up defaults for source */
    simStackReset( TRUE );
    single_step = FALSE;
} 

/************************************************************************
 * NEXTSPACE
 * Skip to white space.
 *
 * Parameters:
 *	line (char *) - the line. (Input)
 *	i (int) - the index into line. (Input)
 *
 * Return Value:
 *	if >= 0, then new index.
 *	if < 0, the EOLN. (int)
 */
static int NextSpace(line, i)
    char	*line;
    int		i;
{
    while (line[++i] != ' ' && line[i] != '\t' && line[i] != '\0');
    return  (line[i] == '\0') ? -1 : i ;
}	

/**************************************************************************
 * NEXTQUOTESPACE
 * Skip to next double quote followed by white space.
 *
 * Parameters:
 *	line (char *) - the line. (Input)
 *	i (int) - the index into line. (Input)
 *
 * Return Value:
 *	The new index. (int)
 */
static int NextQuoteSpace(line, i)
	char	*line;
	int	i;
{
    while (1) {
	i++;
	if ( line[i] == '\0' ) {
	    return -1;		/* Not found. */
	}
	if ( line[i] == '\"' ) {
	    if ( line[i+1] == ' ' || line[i+1] == '\0' || line[i+1] == '\t' ) {
		line[i] = '\0';
		return i;
	    }
	}
    }
}

/***************************************************************************
 * NEXTCHAR
 * Skip white space to next character.
 *
 * Parameters:
 *	line (char *) - the line. (Input)
 *	i (int) - the index into line. (Input)
 *
 * Return Value:
 *	if >= 0, then new index.
 *	if < 0, the EOLN. (int)
 */
static int NextChar(line, i)
	char	*line;
	int		i;
{
	while ((line[++i] == ' ' || line[i] == '\t') && line[i] != '\0') ;

	if (line[i] == '\0') {
		return(-1);
	} else {
		return(i);
	} /* if... */
} /* NextChar... */



/************************************************************************
 * MY_PRINT
 * Set output to 'stdout' and to the log file if it's open.
 *
 * Parameters:
 *	string (char *) - the output. (Input)
 */
void my_print(string)
	char *string;
{
    if (output_file != NIL(FILE)) {
	(void) fputs(string, output_file);
    }
    (void) fprintf( stdout, "%s", string );
    (void) fflush( stdout );
}	

/*****************************************************************************
 * HELP
 * 'help' command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void Help(argc, argv)
	int		argc;
	char	**argv;
{
    int16	com, i;
 
    if (argc == 0) {
	for(com = 0; com_table[com].name != 0; com++) {
	    (void) sprintf(msgbuf, "%11s  :  %4s : %s\n", com_table[com].name,
			   com_table[com].shortname,
			   com_table[com].description);
	    my_print(msgbuf);
	}
    } else {
	for (i=0; i<argc; i++) {
	    for(com = 0; com_table[com].name != 0; com++) {
		if (strcmp(argv[i], com_table[com].name) == 0 ||
		    strcmp(argv[i], com_table[com].shortname) == 0 ) {
		    (void) sprintf(msgbuf, "%s: %s\n", argv[i],com_table[com].description);
		    my_print(msgbuf);
		    break;
		}
	    }
	    if (com_table[com].name == 0) {
		(void) sprintf(errmsg, "%s: not a command", argv[i]);
		Warning(errmsg);
	    }		
	}		
    }			
}				/* Help... */


/* napms in libcursesX might not be portable. */
/* Use select to timeout. */
void musa_napms( ms )
    int ms;
{
    struct timeval timeout;
    int zero = 0;
    if ( ms >= 1000 ) ms = 999;
    timeout.tv_sec = 0;
    timeout.tv_usec = ms * 1000;
    select( 0, &zero, &zero, &zero, &timeout );
}
/*****************************************************************************
 * HELP
 * 'help' command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void Sleep(argc, argv)
	int	argc;
	char	**argv;
{
    int sec;			/* Seconds. */
    int millisec;		/* Milliseconds. */
 
    if (argc != 1 || !getNumericArg( argv[0], &millisec ) ) {
	Usage( "sleep <milliseconds>" );
	return;
    }

    if ( millisec > 0 ) {
	sec = millisec / 1000;
	millisec %= 1000;
	if ( sec ) sleep( sec );
	if ( millisec ) musa_napms( millisec );
    }
}				/* Sleep ... */

/***************************************************************************
 * PARSE
 * Parse command line and strip into components, command and arguments.
 *
 * Parameters:
 *	line (char *) - the command line to parse. (Input)
 *	command (char **) - the command. (Output)
 *	argc (int *) - the argument count. (Output)
 *	argv (char **) - the arguments. (Output)
 */
static void parse(line, command, argc, argv)
	char	*line;
	char	**command;
	int	*argc;
	char	**argv;
{
    int		i = 0, ind = 0;
    int16	quoted;		/* we're in the middle of a "..." */

    *command = line;
    if (**command == '!') {
	*command = NIL(char);
	*argc = 0;
	return;
    }

    quoted = FALSE;
    for (;;) {
	if (quoted) {
	    if ((ind = NextQuoteSpace(line, ind)) < 0) {
		Warning( "Parse error: Possibly a missing space after a closing \"" );
		break;
	    }
	    quoted = FALSE;
	} else {
	    if ((ind = NextSpace(line, ind)) < 0) {
		break;
	    }
	}	
	line[ind] = '\0';
	if ((ind = NextChar(line, ind)) < 0) {
	    break;
	}	
	if (line[ind] == '\"') {
	    quoted = TRUE;
	}	
	argv[i] = &line[ind];
	if (*argv[i] == '!') {
	    break;
	} else {
	    i++;
	}
    }

    *argc = i;
}				/* parse... */

/**************************************************************************** 
 * MY_GETCHARX
 * Check both stdin and the Xevent queue.
 * If we have an Xevent, process it, if stdin is ready, return the 
 * next char
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	The next character form 'stdin'. (int)
 */
int my_getcharX()
{
	int		nfds; 
	int		Xfd = ConnectionNumber(theDisp);/* X event file descriptor */ 
	char	buf[5];		/* slightly larger than needed */ 
	int		readfds; 

	nfds = Xfd + 1; 
	while (TRUE) {
		readfds = (1<<Xfd) | 1;
		(void) select( nfds, &readfds, NIL(int), NIL(int), NIL(struct timeval));
		if (readfds & (1<<Xfd)) {
			HandleXEvents();
		}  /* if... */
		if (readfds & 0x1) {
			if (read(fileno(stdin),buf,1)<=0) {
				return(EOF);
			} else {
				return((int) buf[0]);
			} /* if... */
		} /* if... */
	} /* while... */
} /* my_getcharX... */

/*************************************************************************
 * MY_GETCHAR
 * Get the next character from 'stdin'.  If the X display is open
 * then check for X events.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	The next character from 'stdin'. (int)
 */
int my_getchar()
{
    if (theDisp) {
	return(my_getcharX());
    } else {
	return(getchar());
    }
}	

/**************************************************************************
 * MY_GETC
 * Get a character from 'stdin' if interactive or from file if not
 * interactive.
 *
 * Parameters:
 *	None.
 *
 * Return Value:
 *	The character. (int)
 */
int my_getc()
{
    int		result;
    static int	eof_errors = 0;	/* To count errors due to early EOF */

    HandleXEvents();
    do {
	if ( smlevel == 0 && ! simStackInteractive() ) {
	    /* The first level is not interactive */
	    result = '\0';
	} else if ( simStackInteractive() && !single_step) {
	    if ((result = my_getchar()) == EOF) {
		Warning("End Of File before 'quit'");
		if (eof_errors++ > 4) {
		    errRaise( "MUSA", 1, "Too many EOF before quit at level %d.\n\tThis is a protection against infinite loops", smlevel);
		}	
		result = '\0';
	    }		
	} else if ( simStackMacro() ) {
	    InternalError("macro in my_getc");
	} else {                	    /* non-interactive source or single_step */
	    result = getc( simStackFile() );
	    if ( result == EOF) {
		simStackPop();
		if ( simStackMacro() ) {
		    result = '\n';		    /* null line to break out to macro */
		}
#ifdef RPC_MUSA
		if (smlevel == 0) {
		    return -1;
		}	
#endif
	    } else {
#ifndef RPC_MUSA
		putc(result, stdout);
#endif
	    }		
	}		
    } while (result == EOF);

    /* take care of logging commands */
    if (output_file != NIL(FILE)) {
	putc(result, output_file);
    }	
    return(result);
}				/* my_getc... */

/***************************************************************************
 * LINECONTINUES
 * If line ends in \<cr> back up pointer and return true
 *
 * Parameters:
 *	line (char *) - the line. (Input)
 *	i (int16 *) - index into line. (Input/Output)
 *
 * Return Value:
 *	TRUE, line end in '\'.
 *	FALSe, line doesn't end in '\'.
 */
int16 LineContinues(line, i)
	char	*line;
	int16	*i;
{
	int16	result = FALSE;

	if (*i <= 1) {
		result = (*line == '\\');
	} else {
		result = (strcmp(&line[*i-2], " \\") == 0);
	} /* if... */
	if (result) {
		(*i)--;
	} /* if... */
	return(result);
} /* LineContinues... */

/**************************************************************************
 * GETLINE
 * Get a line.
 *
 * Parameters:
 *	line (char *) - the line. (Output)
 *
 * Return Value:
 *	if >= 0, then length of line.
 *	if -1, then EOF.
 */
int getline(line)
	char	*line;
{
    int16	i = 0;		/* where are we in the line */
    char	c;

    if ( smlevel == 0 && ! simStackInteractive() ) {
	/* The top level is not interactive. Quit quickly. */
	/* my_print( "Quitting for lack of more input.\n" ); */
	return -1;
    }



    if ( simStackMacro() ) {
	return simStackGetMacroLine( line );
    }

    do {			/* Not a line in a macro */
#ifndef RPC_MUSA
	if (i == 0) {
	    (void)sprintf( errmsg, "MUSA(%d)> ",smlevel );
	    my_print( errmsg );
	}			
#endif
	/* Skip over leading spaces */
	while ((c = my_getc()) == ' ' || c == '\t' );
#ifdef RPC_MUSA
	if (smlevel == 0) {
	    return -1;
	}			
#endif
	/* Grab everything until the end of the line */
	while ((c != '\n') && (c != '\0')) {
	    line[i++] = c;
	    c = my_getc();
	}			
	line[i] = '\0';
    } while ( LineContinues(line, &i) );

    if (strncmp(line,"exit",4)==0||strncmp(line,"quit",4)==0||strcmp(line,"q")==0){
	if ( simStackInteractive() ) {
	    return -1;		/* Accept quit from interactive frame */
	} else {
	    Warning( "Ignoring a non interactive quit\n" );
	    return 0;
	}
    }  else {
	return i;
    }
}				/* getline... */


/***************************************************************************
 * LOPEN 
 * Open log file.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void LOpen(argc, argv)
	int		argc;
	char	**argv;
{
#ifdef RPC_MUSA
    (void) ignore((char *) argc);
#else
    if (argc != 1) {
	Usage("lopen log_file\n");
	sim_status |= SIM_USAGE;
	return;
    }
#endif
    if ( output_file != NIL(FILE) ) {
	Warning( "A log file is already open." );
    } else {
	VOVoutput( VOV_UNIX_FILE, argv[0] );
	if ((output_file = fopen( util_file_expand( argv[0] ),"w")) == NIL(FILE)) {
	    (void) sprintf(errmsg, "Can't open \"%s\"", argv[0]);
	    Warning(errmsg);
	    sim_status |= SIM_UNKNOWN;
	} 
    }
} 

#ifdef RPC_MUSA
/***************************************************************************
 * RPCMUSALOPEN
 * Rpc interface to 'lopen' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	 RPC_OK, success.
 */
int rpcMusaLOpen (spot,cmdList,cominfo)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
    int32       cominfo;
{
	struct RPCArg	*arg;
	int				len = lsLength(cmdList);
	static dmTextItem		item = {"File Name",1,50,"",NULL};

	(void) ignore((char *) spot);

	if ((len > 1) || 
		((len == 1) &&
		((lsDelBegin(cmdList,(lsGeneric *) &arg) != LS_OK) ||
		(arg->argType != VEM_TEXT_ARG)))) {
		Format("musa-lopen",(COMINFO *) cominfo);
		return(RPC_OK);
	} /* if... */
	if (len == 0) {
		if ((dmMultiText("musa-lopen",1,&item) == VEM_OK) &&
			(strlen(item.value) > 0)) {
			LOpen(1,&(item.value));
		} else {
			return (RPC_OK);
		} /* if... */
	} else {
		LOpen (1,&(arg->argData.string));
	} /* if... */
	return(RPC_OK);
} /* rpcMusaLOpen... */
#endif

/**************************************************************************
 * LCLOSE
 * Close log file.
 *
 * Parameters:
 *	None.
 */
void LClose()
{
    if (output_file == NIL(FILE)) {
	return;
    }
    if (fclose(output_file) == EOF) {
	Warning("Error trying to closing log file");
	sim_status |= SIM_UNKNOWN;
    }
    output_file = NIL(FILE);
}	

#ifdef RPC_MUSA
/***************************************************************************
 * RPCMUSALCLOSE
 * Rpc interface to 'lclose' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	 RPC_OK, success.
 */
int rpcMusaLClose (spot,cmdList,uow)
	RPCSpot     *spot;      /* window, facet, and mouse location */
	lsList      cmdList;    /* VEM argument list */
    int32       uow;        /* for future use */
{
	(void) ignore((char *) spot);
	(void) ignore((char *) &uow);
	(void) ignore((char *) &cmdList);

	LClose();
	return(RPC_OK);
} /* rpcMusaLClose... */
#endif

#ifndef RPC_MUSA
/**************************************************************************
 * DISPLAYSOURCESTATUS
 * Display source flags.
 *
 * Parameters:
 *	None.
 */
void DisplaySourceStatus ()
{
    char flags[10];

    /* print status of most recent source */
    *flags = '\0';
    if (smstack[last_source].u.sr.stop & SIM_NOTFOUND) {
	(void) strcat(flags, "F");
    }
    if (smstack[last_source].u.sr.stop & SIM_VERIFY) {
	(void) strcat(flags, "V");
    }
    (void) sprintf(msgbuf, "Source Flags: %s   Depth %d\n", flags, smlevel);
    my_print(msgbuf);
}				/* DisplaySourceStatus... */
#endif

/************************************************************************
 * PCONTINUE
 * Continue processing a source file, macro or evaluation
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void PContinue(argc, argv)
    int		argc;
    char	**argv;
{
    Evaluate(argc, argv, FALSE); /* in case eval was interrupted */
    if (smlevel > 0) {
	smstack[smlevel].interactive = FALSE;
    }
}	

/**************************************************************************
 * SOURCE
 * 'source' command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 *	step (int16) - if TRUE, single step through source file.
 */
void Source(argc, argv, step)
	int	argc;
	char	**argv;
	int16	step;		/* source in single step mode if true */
{
    int		onmask = 0, offmask = 0; /* masks for changing flags */
    int		i;
    char	*p;
    FILE	*fp;

    for (i = 0; i < argc && ((*argv[i] == '-') || (*argv[i] == '+')); i++) {
	p = &(argv[i][1]);
	while (*p != '\0') {
	    switch (*p) {
	    case 'r': case 'R':
		simStackReset( TRUE );
		single_step = FALSE;
		break;
	    case 'f': case 'F':
		if (*argv[i] == '-') {
		    offmask |= SIM_NOTFOUND;
		} else {
		    onmask |= SIM_NOTFOUND;
		}
		break;
	    case 'v': case 'V':
		if (*argv[i] == '-') {
		    offmask |= SIM_VERIFY;
		} else {
		    onmask |= SIM_VERIFY;
		}
		break;
	    default:
#ifndef RPC_MUSA
		Usage("source [+-][FRV] musa_command_file\n");
		sim_status |= SIM_USAGE;
#endif
		return;
	    }		
	    p++;
	}		
    }			

    /* Open a new source if specified */
    if (i < argc) {
	VOVinput( VOV_UNIX_FILE, argv[i] );
	if ((fp = fopen( util_file_expand( argv[i] ), "r")) == NIL(FILE)) {
	    (void) sprintf(errmsg, "Can't open \"%s\"", argv[i]);
	    Warning(errmsg);
	    sim_status |= SIM_UNKNOWN;
	} else {
	    simStackPushFile( fp, onmask );
	}
    }	

    if (step) {
	if (last_source > 0) {
	    smstack[last_source].interactive = TRUE;
	    single_step = TRUE;
	}
    }		

#ifndef RPC_MUSA
    DisplaySourceStatus ();
#endif
}				/* source... */

#ifdef RPC_MUSA
/**************************************************************************
 * RPCMUSASOURCE
 * Rpc interface to 'source' command.
 *
 * Parameters:
 *	spot (RPCSpot *) - spot structure. (Input)
 *	cmdList (lsList) - command list. (Input)
 *	cominfo (int32) - command information. (Input)
 *
 * Return Value:
 *	 RPC_OK, success.
 */
int rpcMusaSource (spot,cmdList,cominfo)
    RPCSpot     *spot;      /* window, facet, and mouse location */
    lsList      cmdList;    /* VEM argument list */
    int32       cominfo;
{
    struct RPCArg	*arg;
    int16			len;
    char			line[MAXLINE], *command, *margv[MAXARGU];
    int				margc;
    static dmTextItem		item = {"File Name",1,50,"",NULL};
    int				length = lsLength(cmdList);

    (void) ignore((char *) spot);

    if ((length > 1) || 
	((length == 1) &&
	 ((lsDelBegin(cmdList,(lsGeneric *) &arg) != LS_OK) ||
	  (arg->argType != VEM_TEXT_ARG)))) {
	Format("musa-source",(COMINFO *) cominfo);
	return(RPC_OK);
    }				/* if... */
    (void) XFindContext(theDisp,spot->theWin,dirTable,(caddr_t) &cwd);
    if (length == 0) {
	if ((dmMultiText("musa-source",1,&item) == VEM_OK) &&
	    (strlen(item.value) > 0)) {
	    Source(1,&(item.value),FALSE);
	} else {
	    return (RPC_OK);
	}			/* if... */
    } else {
	Source(1,&(arg->argData.string),FALSE);
    }				/* if... */	
    if (!(sim_status & SIM_UNKNOWN)) {
	while ((len = getline(line)) >= 0) {
	    sim_status = SIM_OK;
	    if (len > 0) {
		parse(line,&command,&margc,margv);
		if ( simStackMacro() ) {
		    if (SubstParams(margc, margv)) {
			execute(command,margc,margv);
		    }		
		} else {
		    single_step = FALSE;
		    execute(command,margc,margv);
		}		
	    }		
	}			
	VEMMSG("musa-source: DONE\n");
	vemPrompt();
    } else {
	sim_status &= !SIM_UNKNOWN;
    }				/* if... */
    return (RPC_OK);
}				/* rpcMusaSource... */
#endif

/*************************************************************************
 * PSTEP
 * Go one step in source file
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void PStep(argc, argv)
    int		argc;
    char	**argv;
{
    if (argc != 0) {
	/* run a source file in single step mode */
	Source(argc, argv, TRUE);
	return;
    }

    Evaluate(argc, argv, FALSE); /* in case eval was interrupted */
    if (smstack[smlevel].is_macro) {
	smstack[smlevel].interactive = FALSE;
    }
    if (smlevel > 0) {
	single_step = TRUE;
	smstack[last_source].interactive = TRUE;
    }
}				/* PStep... */

/**************************************************************************
 * MACRO
 * 'macro' command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
void Macro(argc, argv)
	int		argc;
	char	**argv;
{
	macdef	*temp;
	char	buffer[MAXLINE];
	char	c, *linestart;
	int		i = 0;

#ifdef RPC_MUSA
	(void) ignore((char *) &argc);
#else
	if (argc != 1) {
		Usage("macro macro_name\n");
		sim_status |= SIM_USAGE;
		return;
	} /* if... */
#endif
	while (1) {
		linestart = &(buffer[i]);
#ifndef RPC_MUSA
		my_print("> ");
#endif
		while ((c = my_getc()) == ' ' || c == '\t') {}
		if (c == '\n') {
			continue;
		} /* if... */
		buffer[i++] = c;

		while ((c = my_getc()) != '\n') {
			buffer[i++] = c;
		} /* while... */
		buffer[i++] = '\n';

		if (strncmp(linestart, "$end", 4) == 0) {
			break;
		} /* if... */
	} /* while... */
	buffer[i] = '\0';

	temp = macros;
	macros = ALLOC(macdef, 1);
	macros->next = temp;
	macros->name = util_strsav(argv[0]);
	macros->def = util_strsav(buffer);
} /* Macro... */

/****************************************************************************
 * SUBSTPARAMS
 * Substitute parameters into macros.
 * return false if unsuccessful
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 *
 * Return Value:
 *	if TRUE, success.
 *	if FALSE, unsuccessful.
 */
static int SubstParams(argc, argv)
	int argc;
	char **argv;
{
    int i, param;

    for (i=0; i<argc; i++) {
	if (*argv[i] == '$') {
	    /* substitute param */
	    param = atoi(&(argv[i][1]));
	    if ((param < 1) || (param > smstack[smlevel].u.ma.numparams)) {
		/* This is a serious error that will send us in interactive mode */
		(void) sprintf(errmsg, "No parameter %s given in macro", argv[i]);
		Warning(errmsg);
		sim_status |= SIM_NOTFOUND;
		return FALSE;
	    }			
	    argv[i] = smstack[smlevel].u.ma.params[param-1];
	}			
    }				
    return TRUE;
}				/* SubstParams... */



int SetSeparator( argc, argv )
    int argc;
    char* argv[];
{
    if ( argc ) {
	separator = *argv[0];
    }
    (void)sprintf( errmsg, "Separator is now: %c\n", separator );
    my_print( errmsg );
    return 0;
}



/***************************************************************************

*/
#define MAX_REPEAT_LEVELS 64
Repeat( argc, argv )
    int argc;
    char* argv[];
{
    static int* repeatCountArray = 0;
    static level = 0;
    int i ;
    if ( repeatCountArray == 0 ) {
	repeatCountArray = ALLOC( int, MAX_REPEAT_LEVELS );
	for ( i = 0 ; i < MAX_REPEAT_LEVELS ; i++ ) {
	    repeatCountArray[i] = 0;
	}
    }

    if ( argc < 2 || ! getNumericArg( argv[0] ,  &repeatCountArray[level] ) ) {
	sim_status |= SIM_USAGE;
	Usage("repeat <decimal number|#constant> <command>" );
	return;
    }
    while ( repeatCountArray[level]-- ) {
	level++;
	execute( argv[1], argc-2, &argv[2] );
	level--;
	if (sim_status & SIM_ANYBREAK) {
	    break;
	}
    }
}
/****************************************************************************
 * EXECUTE
 * Execute command.
 *
 * Parameters:
 *	argc (int) - argument count. (Input)
 *	argv (char **) -  the arguments. (Input)
 */
int execute(command, argc, argv)
	char	*command;
	int	argc;
	char	**argv;
{
    int16	com;
    musaInstance	*temp;
    macdef	*m;

    if (command == NIL(char)) return;
    for(com = 0; com_table[com].name != 0; com++) {
	if (strcmp(command, com_table[com].name) == 0 || strcmp(command, com_table[com].shortname) == 0 ) {
	    break;
	}
    }

    switch (com_table[com].command) {
    case COM_source:
	Source(argc, argv, FALSE);
	break;
    case COM_separator:
	SetSeparator( argc, argv );
	break;
    case COM_repeat:
	Repeat( argc, argv );
	break;
    case COM_sleep:
	Sleep( argc, argv );
	break;
    case COM_help :
	Help( argc, argv);
	break;
    case COM_lopen :
	LOpen(argc, argv);
	break;
    case COM_lclose :
	LClose();
	break;
    case COM_macro :
	Macro(argc, argv);
	break;
    case COM_watch:
	Watch(argc, argv, cwd);
	break;
    case COM_listinsts:
	gnListInsts(argc, argv, cwd);
	break;
    case COM_listelems:
	gnListNets(argc, argv, cwd);
	break;
    case COM_listlogix:
	ListLogiX(argc,argv,cwd);
	break;
    case COM_equivelems:
	gnEquivElems(argc, argv, cwd);
	break;
    case COM_cinst:
	if (gnCInst(argc, argv, &temp, cwd)) {
	    cwd = temp;
	}			/* if... */
	break;
    case COM_pwd:
	gnSetDir(cwd);
	gnPwd();
	break;
    case COM_makevect:
	MakeVect(argc, argv, cwd);
	break;
    case COM_setvect:
	SetVect(argc, argv, cwd);
	break;
    case COM_verivect:
	Verify(argc, argv, cwd);
	break;
    case COM_nodeinfo:
	NodeInfo(argc, argv, cwd);
	break;
    case COM_iinfo:
	InstInfo(argc, argv, cwd);
	break;
    case COM_show:
	ShowValue(argc, argv, cwd);
	break;
    case COM_backtrace:
	Backtrace(argc, argv, cwd);
	break;
    case COM_evaluate:
	Evaluate(argc, argv, TRUE);
#ifdef RPC_MUSA
	rpcMusaUpdateSelSet();
#endif
	break;
    case COM_continue:
	PContinue(argc, argv);
	break;
    case COM_step:
	PStep(argc, argv);
	break;
    case COM_setbreak:
	SetBreak(argc, argv, cwd);
	break;
    case COM_savestate:
	SaveState(argc, argv);
	break;
    case COM_loadstate:
	LoadState(argc, argv);
	break;
    case COM_initstate:
	InitState(argc, argv);
	break;
    case COM_setplot:
	SetPlot(argc,argv);
	break;
    case COM_plot:
	Plot(argc, argv, cwd);
	break;
    case COM_saveplot:
	SavePlot(argc,argv);
	break;
    case COM_destroyplot:
	DestroyPlot(argc,argv);
	break;
    case COM_unknown :
	/* check to see if this is a macro */
	for (m = macros; m != NIL(macdef); m = m->next) {
	    if (strcmp(m->name, command) == 0) break;
	}			
	if (m != NIL(macdef)) {
	    simStackPushMacro( m, argc, argv );
	    musaInteractiveLoop(1);
	    break;
	}
	(void) sprintf(errmsg, "Unknown command: '%s'", command);
	Warning(errmsg);
	sim_status |= SIM_USAGE;
	break;
    }				/* switch... */
}				/* execute... */

#ifndef RPC_MUSA
/****************************************************************************
 * INTER
 * runs commands from one of three sources
 * run commands from stdin or macro or source file.
 *
 * Interactive mode implies input from the tty.  This can be temporarily
 * overridden by setting single_step = 1
 *
 * Parameters:
 *	None.
 */
int musaInteractiveLoop( ephimeral )
    int ephimeral;
{
    char	line[MAXLINE], *command, *margv[MAXARGU]; 
    int		margc;
    int16	len;

    while ((len = getline(line)) >= 0) {
	sim_status = SIM_OK;
	if (len > 0) {
	    parse(line, &command, &margc, margv);	
	    if ( simStackInteractive() && !(single_step && (last_source > 0))) {
  		single_step = FALSE;			/* Interactive -- input from tty */
		execute(command, margc, margv);
	    } else if ( simStackMacro() ) { 		/* Inside a macro */
		if (SubstParams(margc, margv)) {	/* Substitute parameters as necessary. */
		    execute(command, margc, margv);
		}	
	    } else {
		single_step = FALSE;			/* non-interactive source or single_step */
		execute(command, margc, margv);
	    }		
	    if (sim_status & SIM_ANYBREAK) {
		while ( simStackMacro() ) {		/* break out of macros */
		    simStackPop();
		}
		if ( ephimeral ) {
		    return sim_status;
		}
	    } 
	    if ( !simStackMacro() ) {
		if (sim_status & smstack[smlevel].u.sr.stop) {
		    simStackPop();
		}
	    }
	}
    }		
    return sim_status;
}		
#endif

