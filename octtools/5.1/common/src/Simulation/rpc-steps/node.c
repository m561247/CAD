/*   STEPS - Schematic Text Entry Package utilizing Schematics
 * 
 *   Denis S. Yip
 *   U.C. Berkeley
 *   1990
 */

#include "port.h"
#include "list.h"
#include "utility.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "table.h"
#include "defs.h"

int processPulse();
int processSin();
int processExp();
int processPwl();
int processSffm();

extern char* strrmone();
extern char* getPropValue();

int stepsUseNodeNumbers = 0;

extern int modelEdit();
extern double atofe();
extern int highlight_flag;
extern int dc_ok;
extern int ac_ok;
extern int tran_ok;
extern int noise_ok;
extern int disto_ok;
extern char cur_deck_wo[100];  
extern char display_ptr[40];
extern char cur_deck[100];
extern char *index();
char *gensym();
char *genmod();
FILE *fp;




st_table *modeltable, *subckttable;
lsList	 modList, subList;

static	octObject	topFacet, groundNet;

#define	STREQ(a,b)	(strcmp((a), (b)) == 0)



int renumberNodes( facet )
    octObject *facet;
{
    octGenerator gen;
    octObject net, prop;
    int count = 2;
    int n;
    char buf[100];

    octInitGenContents( facet, OCT_NET_MASK, &gen );
    while ( octGenerate( &gen, &net ) == OCT_OK ) {
	n = count++;
	if ( ohGetByPropName( &net, &prop, "NETTYPE" ) == OCT_OK ) {
	    if ( PROPEQ( &prop, "GROUND" ) ) {
		n = 0;
	    }
	}
	sprintf( buf, "%d", n );
	ohCreateOrModifyPropStr( &net, &prop, "NODENUMBER", buf );
    }
    return count;		/* Largest node number. */
}

char* nodeNumber( net )
    octObject* net;
{
    char* n = "?";
    octObject prop;

    if ( stepsUseNodeNumbers ) {
	if ( ohGetByPropName( net, &prop, "NODENUMBER" ) == OCT_OK ) {
	    n = prop.contents.prop.value.string;
	}
    } else {
	if ( ohGetByPropName( net, &prop, "NETTYPE" ) == OCT_OK && PROPEQ(&prop,"GROUND") ) {
	    n = "0"; 
	} else {
	    n = net->contents.net.name;
	}
    }
    if ( n[0] == '\0' ) n = "????"; /* Not a name */
    return n;
}

deckInit(facet )
    octObject	*facet;
{
    octObject	optBag, icBag, prop, net, term, property, bag,
    prop0, prop1, prop2, prop3, prop4, prop5, prop6;
    octGenerator gen;
    char file_str[30], temp_str[30], c;
    int opt_count, opt_i;
    int i ;
    char *filename;


    topFacet = *facet;


    filename = getStepsOption( facet, "Spice deck in" );

    stepsEcho( "Opening file " ); stepsEcho( filename );

    if ((fp = stepsFopen( util_file_expand(filename), "w")) == (FILE *) 0) {
	errRaise( "STEPS", 1, "Can't open text file %s", filename );
    }


    /*** title line ***/
    fprintf(fp,  "STEPS deck from %s\n", ohFormatName( facet ) );


    /** Spice Options **/
    ohGetOrCreateBag( facet, &optBag, "SPICE_OPTIONS" );
    /* fprintf( fp, "\n***** Set all SPICE defaults explicitly *****\n" ); */
    table2spice( fp, &optBag, spiceDefaultsTable );

    /* user-options */
    insertUserFiles( fp, facet );
	    
    return 1;
}

insertUserFiles( fp, facet )
    FILE *fp;
    octObject *facet;

{
    char *userFiles = getStepsOption( facet, "User file" ); /* Comma separated list */
    char *file, *end_file;
    FILE *fpuser;
    char c;

    if ( userFiles == 0 || strlen( userFiles ) == 0  )  return 0;
	
    userFiles = util_strsav( userFiles ); /* Make a copy to work on */
    file = userFiles;

    while ( file && strlen( file ) ) {
	char *end_file = strchr( file, ',' ); /* Is there a comma? */
	if ( end_file != 0 ) *end_file++ = '\0'; /* Yes, terminate name and move one over. */

	if ((fpuser = stepsFopen( util_file_expand(file), "r")) == (FILE *) 0) {
	    char buf[1024];
	    sprintf( buf, "ERROR: Can't open user file %s", file );
	    fprintf( fp, "** %s\n", buf );
	    stepsEcho( buf );
	}  else {
	    fprintf( fp, "** Start User file: %s\n", file );
	    while ((c=getc(fpuser)) != EOF) putc(c, fp); /* Append user file. */
	    (void) fclose(fpuser);
	    fprintf( fp, "** End   User File: %s\n\n", file );
	}
	file = end_file;	/*  */
    }
    FREE(userFiles );
    return  0;
}



deckFinish( facet )
    octObject *facet;
{
    int i, j;
    int inits_flag;
    char c;
    octObject bag;


    
    ohGetOrCreateBag( facet, &bag, "STEPS" );
    table2spice( fp, &bag, analysisTable );
    table2spice( fp, &bag, printTable );


    fprintf(fp, "\n.end\n");

    fclose(fp);

    if (highlight_flag) {
	stepsEcho( "Instances with missing connections are highlighted\n");
	stepsEcho( "Please use 'highlight-off' when editing is done\n");
    }
    
    
    
    {
	char buf[256];
	sprintf( buf, "Deck %s ready.", getStepsOption( facet, "Spice deck in")); 
	stepsEcho( buf );
    }

    return;
}


double  atofe(str, i)
    int i;
    char *str;
{
    float value;
    int len;
  
    len = strlen(str);
    if (strcspn(str, "k") != len || strcspn(str, "K") != len) {
	value = 1000 * atof(str);
	return(value);
    }
    if (strcspn(str, "m") != len) {
	value = 0.001 * atof(str);
	return(value);
    }
    if (strcspn(str, "M") != len) {
	value = 1000000 * atof(str);
	return(value);
    }
    if (strcspn(str, "g") != len || strcspn(str, "G") != len) {
	value = 1000000000 * atof(str);
	return(value);
    }
    if (strcspn(str, "u") != len || strcspn(str, "U") != len) {
	if (i) {
	    value = 0.000001 * atof(str);
	} else {
	    value = 0.000001 * atof(str);
	}      
	return(value);
    }
    if (strcspn(str, "n") != len || strcspn(str, "N") != len) {
	if (i) {
	    value = 0.000000001 * atof(str);
	} else {
	    value = 0.000000001 * atof(str);
	}
	return(value);
    }
    if (strcspn(str, "p") != len || strcspn(str, "P") != len) {
	if (i) {
	    value = 0.000000000001 * atof(str);
	} else {
	    value = 0.000000000001 * atof(str);
	}
	return(value);
    }  
    if (strcspn(str, "f") != len || strcspn(str, "F") != len) {
	if (i) {
	    value = 0.000000000000001 * atof(str);
	} else {
	    value = 0.000000000000001 * atof(str);
	}
	return(value);
    }  
    if (strcspn(str, "a") != len || strcspn(str, "A") != len) {
	if (i) {
	    value = 0.000000000000000001 * atof(str);
	} else {
	    value = 0.000000000000000001 * atof(str);
	}
	return(value);
    }  
    /*
      value = 1.000001 * atof(str);
      */
    value = atof(str);

    return(value);
}





stepsDefaults( facet )
    octObject *facet;
{
    char buf[256];
    char root[256];

    sprintf( root, "%s-%s", facet->contents.facet.cell,
	    facet->contents.facet.view );

    if ( getStepsOption( facet, "Spice deck in" ) == 0 ) {
	sprintf( buf, "%s.spi", root );
	setStepsOption( facet, "Spice deck in", buf, S );
    }

    if ( getStepsOption( facet, "Spice deck out" ) == 0 ) {
	sprintf( buf, "%s.out", root );
	setStepsOption( facet, "Spice deck out", buf, S );
    }

}
