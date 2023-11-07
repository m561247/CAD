/***********************************************************************
 * Main Module 
 * Filename: musa.c
 * Progagsram: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define MAIN_MUSA
#include "musa.h"
#include "oh.h"
#include "options.h"
#include "simstack.h"
#include "vov.h"



optionStruct optionList[] = {
    { "i", "comfile", "command file to be executed on startup" },
    { "o", "file", "output file name" },
    { "w", "thresh", "set the value W/L for weak threshold" },
    { "s", 0, "input is in sim format" },
    { "P", 0, "do pullup optimization" },
    { "M", 0, "merge parallel and series transistors" },
    { "V", 0, "print version number" },
    { "v", 0, "verbose mode" },
    { OPT_RARG, "cell:view (for Oct) or file (for sim)", "" },
    { 0, 0, 0 },
};

/**************************************************************************
 * MAIN
 */
main(argc, argv)
int	argc;
char	**argv;
{
    octObject	rootfacet;
    int		option;
    int16	iOption = FALSE;
    char	*inputFileName = 0;
    char	*outFileName = 0;
    int         status;		/* Simulation status. */

    progName = "musa"; 
    /* get option stuff */

    while ((option = optGetOption(argc, argv)) != EOF) {
	switch (option) {
	case 'V':
	    Message(version());
	    break;
	case 'v':
	    verbose = TRUE;
	    break;
	case 'P':
	    no_pulls = FALSE;
	    break;
	case 'M':
	    no_merge = FALSE;
	    break;
	case 'i':
	    iOption = TRUE;
	    inputFileName = optarg;
	    break;
	case 'o':
	    outFileName = optarg;
	    break;
	case 's':
	    sim_format = TRUE;
	    break;
	case 'w':
	    weak_thresh = atof(optarg);
	    break;
	default:
	    optUsage();
	}
    } 
    

    displayName = (char *) getenv("DISPLAY");
    InitLists();
    InitAllocs();
    topmusaInstance = NIL(musaInstance);
    if (sim_format) {
	VOVbegin( argc, argv );

	if ( outFileName != NIL(char ) ) {
	    VOVoutput( VOV_UNIX_FILE, outFileName  );
	    if ((output_file = fopen( util_file_expand( outFileName ), "w")) == NIL(FILE)) {
		(void) fprintf(stderr, "Error: Can't open \"%s\"\n", optarg);
	    }
	}
	if (optind > argc-1) {
	    optUsage();
	}
	while (optind < argc) {
	    if (!yyopenfile(argv[optind++], 0)) {
		VOVend(-1);
	    } 
	    read_sim();
	} 
    } else {
	if (optind != argc-1) {
	    optUsage();
	} 

	VOVbegin( argc, argv );
	
	if ( outFileName != NIL(char ) ) {
	    VOVoutput( VOV_UNIX_FILE, outFileName  );
	    if ((output_file = fopen( util_file_expand( outFileName ), "w")) == NIL(FILE)) {
		(void) fprintf(stderr, "Error: Can't open \"%s\"\n", optarg);
	    }
	}

	ohUnpackDefaults(&rootfacet, "r", "::contents");
	if (ohUnpackFacetName(&rootfacet, argv[optind]) != OCT_OK) {
	    optUsage();
	}

	octBegin();
	VOVinputFacet( &rootfacet );
	OH_ASSERT( octOpenFacet(&rootfacet) );

	read_oct(&rootfacet);
	OH_ASSERT( octCloseFacet(&rootfacet) );
	octEnd();
    } 
    if (!no_merge) {
	merge_trans();
    }
    init_sched();
    InterInit();
    if (iOption) {
	VOVinput( VOV_UNIX_FILE, inputFileName );
	if ((input_file = fopen(inputFileName, "r")) == NIL(FILE)) {
	    (void) fprintf(stderr, "Can't open \"%s\"\n",inputFileName);
	} else {
	    simStackReset( FALSE );
	    simStackPushFile( input_file, SIM_ANYBREAK );
	    DisplaySourceStatus ();
	}			
    }				
    status = musaInteractiveLoop( 0 );
    if (theDisp) {
	XCloseDisplay(theDisp);
    }
    if ( status == SIM_OK ) {
	VOVend( 0 );
    } else {
	Message( "MUSA exiting with error status 1" );
	VOVend( 1 );
    }
}				/* main... */

