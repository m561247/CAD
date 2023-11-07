#include "musa.h"
#include "simstack.h"
#include "stdio.h"

smplace		smstack[SOURCE_DEPTH];
int		smlevel;        /* current place in smstack */
int		last_source;    /* most recent sr in stack */


int simStackPush()
{
    if ( smlevel < SOURCE_DEPTH - 1 ) {
	smlevel++;
	return 1;
    } else {
	Warning( "Too many source levels." );
	return 0;
    }
}

void simStackPushFile( fp, stopflags )
    FILE* fp;
    int stopflags;
{
    if ( simStackPush() ) {
	smstack[smlevel].is_macro = FALSE;
	smstack[smlevel].interactive = FALSE;
	smstack[smlevel].u.sr.file = fp;
	smstack[smlevel].u.sr.stop = stopflags;
	last_source = smlevel;
    }
}

void simStackPushMacro( m, argc, argv )
    macdef* m;
    int argc;
    char** argv;
{
    if ( simStackPush() ) {
	int i;
	smstack[smlevel].is_macro = TRUE;
	smstack[smlevel].interactive = FALSE;
	smstack[smlevel].u.ma.place = m->def;
	smstack[smlevel].u.ma.numparams = argc;
	smstack[smlevel].u.ma.params = ALLOC(char *, argc);
	for (i=0; i<argc; i++) {
	    smstack[smlevel].u.ma.params[i] = util_strsav(argv[i]);
	}
    }
}

void simStackPop()
{
    if ( smlevel == 0 ) return;	/* Already at the bottom. */
    if ( simStackFile() ) {
	int i;
	(void) fclose( simStackFile() );
	for (i = smlevel - 1; i >= 0; i--) {
	    if (! smstack[i].is_macro) {
		last_source = i;
		break;
	    }	
	}	
    }

    if ( smstack[smlevel].is_macro ) {
	int i;
	for (i=0; i<smstack[smlevel].u.ma.numparams; i++) {
	    FREE(smstack[smlevel].u.ma.params[i]);
	}			
	FREE(smstack[smlevel].u.ma.params);
    }
    if ( smlevel )  smlevel--;
}


void simStackReset( interactive )
    int interactive;
{
    int j;
    for (j = smlevel; j > 0; j--) {
	if (!smstack[j].is_macro) {
	    (void) fclose(smstack[j].u.sr.file);
	}
    }		
    single_step = FALSE;

    smlevel = last_source = 0;
    smstack[0].is_macro = FALSE;
    smstack[0].interactive = interactive;
    smstack[0].u.sr.file = stdin;
    smstack[0].u.sr.stop = SIM_ANYBREAK;
}



int  simStackGetMacroLine( line )
    char* line;
{
    int i = 0;
    while (*smstack[smlevel].u.ma.place != '\n') {
	line[i++] = *(smstack[smlevel].u.ma.place++);
    }		
    smstack[smlevel].u.ma.place++;
    line[i] = '\0';

    if (strncmp(line, "$end", 4) == 0) { /* End of the macro. */
	simStackPop();
	line[0] = '\0';
	return -1;
    } else {
	return i;
    }
}



FILE* simStackFile()
{
    return  simStackMacro() ? NIL(FILE) : smstack[smlevel].u.sr.file;
}


int simStackInteractive()
{
    return smstack[smlevel].interactive;
}

int simStackMacro()
{
    return smstack[smlevel].is_macro;
}



