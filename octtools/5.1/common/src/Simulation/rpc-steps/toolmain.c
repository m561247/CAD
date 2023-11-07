#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "oh.h"
#include "options.h"
#include "vov.h"
#include "tap.h"
#include "th.h"
#include "utility.h"
/*
 *   >>>  GLOBAL VARIABLES DECLARATION
 */


int dc_ok = 0;
int disto_ok = 0;
int xgraph_flag = 0;
int highlight_flag = 0;

int isRPC = 0;

int verbose = 0;

optionStruct optionList[] = {
    {"C", "real",              "Threshold cap for parasitics" },
    {"f", "filename",          "Technology rule file" },
    {"o", "out file",          "Output file name" },
    {"v", "",                  "Verbose mode"},
    { OPT_RARG, "cell:view",      "Input facet" },
    { 0, 0 , 0}
};



main(argc,argv)
    int argc;
    char *argv[];
{
    char  *outputName = 0;
    char  *inputName = 0;
    char  *techFileName = 0;
    char *outFileName = 0;
    char *minCapArg = 0;
    int   c;
    double minCap = 0.020e-12;	/* 0.020 pF */

    /* parse command line options*/
    while ((c = optGetOption(argc,argv)) != EOF) {
	switch(c) {
	case 'C': minCapArg = optarg; break;
	case 'f': techFileName = optarg; break;
	case 'o': outFileName = optarg; break;
	case 'v':
	    verbose = 1;
	    VOVredirect( 0 );
	    break;
	default:
	    optUsage();
	    break;
	}
    }

    /* the remaining arguments are argv[optind ... argc-1] */
    if (optind >= argc) {
	optUsage();
    } else {
	inputName  = util_strsav(argv[optind]);
    }

    octBegin();
    VOVbegin( argc, argv );

    if ( techFileName ) {
	thReadFile( techFileName );
    }

    {
	octObject Chip, net;

	ohInputFacet( &Chip, inputName );

	
	/* if ( userFileName ) setStepsOption( &Chip, "User file", userFileName,S ); */
	if ( minCapArg  ) setStepsOption( &Chip, "CapThreshMin", minCapArg, OCT_REAL );

	stepsDefaults( &Chip );
	deckInit( &Chip );
	processCell( &Chip, 0 );
	deckFinish( &Chip );
    }
    octEnd();
    VOVend(0);			/* Successful. */
}

stepsEcho( s )
    char* s;
{
    printf( "STEPS:%s\n", s );
}

FILE* stepsFopen( name, mode )
    char* name;
    char* mode;
{
    
    return VOVfopen( name, mode );
}

highlight( obj )
    octObject* obj;
{
    printf( "HIGHLIGHT %s\n", ohFormatName( obj ) );
}
