#include <sys/types.h>
#include <sys/stat.h>

#include "copyright.h"
#include "port.h"
#include "options.h"
#include "oct.h"
#include "head.h"

/* defaults -  should be moved to head.h */
#define DEFTECHLIB "~octtools/lib/oct2hilo/"
#define UNIT_DELAY_TECH "UNIT.tech"
#define TYP_DELAY_TECH  "TYP.tech"
#define MAX_DELAY_TECH  "MAX.tech"
#define DEF_TECH TYP_DELAY_TECH
#define SCHEMTECHFILE "schematic.tech"
#define DEFAULT_CELL_LIB "work"

int ARGFLAG;
int errorcount = 0;
int topdown = 0;
int singlenets = 0;
int showconnect = 0;
int warning = 1;
int yo=0;
int have_vdd = 0;
int have_gnd = 0;
char *cct_name;
char *techFile;
char *cell_n_view;
char *delayType;
char *cell_lib;
char *outputFile;
int  prog;
char *delay_tech;
int debug;
char *cell, *view;
char *spec_view;
char *spec_prog;

octObject cfacet;
octObject ifacet;
extern octObject bag;
extern char *buffer;


main(argc, argv)
  int argc;
  char **argv; {

  int invert = 0, nout = 0;
  char *ptr,*strchr();
  int option;
 
  /* initialize a few globals with defaults */
  cell_lib = DEFAULT_CELL_LIB;
  techFile = NULL;
  spec_view = NULL;
  spec_prog = NULL;
  delay_tech = DEF_TECH;
  debug = 0;  /* don't print debug info */
  cell_n_view = NULL;

  /* read the command line */
  if(check_flag(argc,argv)==0)
      exit(-1);

  /*  allocate space for global buffer.  (sort of a remnant) */
  buffer = (char*) malloc(BLOCK*sizeof(char)); 

    
  /* get program name and set global variable.  right now we think
     we're oct2hilo and oct2vhdl */
  if(spec_prog!=NULL)
    ptr = spec_prog;
  else if((ptr=(char*)strrchr(argv[0],'/'))==NULL)
    ptr = argv[0];
  else
    ptr++;
  if(strcmp(ptr,"oct2hilo")==0)
    prog = PROG_HILO;
  else if(strcmp(ptr,"oct2vhdl")==0)
    prog = PROG_VHDL;
  else {
    fprintf(stderr,"%s: program name unknown.\n",ptr);
    exit(-1);
    }


  /* initialize OCT database */
  octBegin(); 
  cfacet.objectId = oct_null_id;
  ifacet.objectId = oct_null_id;

  /* break cell_n_view into cell and view */
  parse_facet_spec(cell_n_view, &cell, &view);

  /* open contents facet */
  open_old_facet(&cfacet, cell, view, "contents", "r");

 
  /* if view specified on command line: */  
  if(spec_view!=NULL) {
    if (strcmp(spec_view,"schematic")==0) 
      /* generate HiLo/VHDL model of schematic view of cell */
      do_schematic();
    else if (strcmp(spec_view,"gem")==0) 
      /* generate HiLo/VHDL model of gem view of cell */
      do_gem();
    else if (strcmp(spec_view,"symbolic")==0 ) 
      /* generate HiLo/VHDL model of schematic view of cell */
      do_symbolic();
    else {
      /* doesn't seem to be anything we understand */
      fprintf(stderr,"%s:  unrecognized view: %s\n",argv[0],spec_view);
      exit(-1);
      }
    }
  /* else try to use the view from cell:view */
  else {
    if (strcmp(view,"schematic")==0) 
      /* generate HiLo/VHDL model of schematic view of cell */
      do_schematic();
    else if (strcmp(view,"gem")==0) 
      /* generate HiLo/VHDL model of gem view of cell */
      do_gem();
    else if ((strcmp(view,"logic")==0) || (strcmp(view,"symbolic")==0) || 
        (strcmp(view,"optlogic")==0) || (strcmp(view,"wolfe")==0) ||
        (strcmp(view,"flat")==0)) 
      /* generate HiLo/VHDL model of schematic view of cell */
      do_symbolic();
    else {
      /* doesn't seem to be anything we understand */
      fprintf(stderr,"%s:  unrecognized view: %s\n",argv[0],view);
      exit(-1);
      }
    }
  }


/* 
 *  parse_facet_spec()  parses the cell and view from the name.  
 */

parse_facet_spec(arg, cell, view)
  char *arg, **cell, **view; {
  char *colon, *name;

  if(debug>0) {
    fprintf(stderr,"parse_facet_spec: arg %s\n",arg);
    fflush(stderr);
    }

  /* split arg into cell and view */
  if ((colon=strchr(arg,':'))==NULL) {
    /* no view specified! */
    *cell = arg;
    /* default to symbolic view */
    *view = "symbolic";
    }
  else {
    *colon = '\0';
    *cell = arg;
    *view = colon+1;
    }

  if((name=strrchr(arg,'/'))==NULL) {
    cct_name = (char *) copy_str(arg,1);
    }
  else { 
    cct_name = (char *) copy_str(name,1);
    }
  }

/* 
 *  open_old_facet() fills in an object and opens a facet 
 */

open_old_facet(theFacet, cell, view, facet, mode)
  octObject *theFacet;
  char *cell, *view, *facet, *mode; {

  if(debug>0) {
    fprintf(stderr,"open_old_facet: cell %s view %s facet %s mode %s\n",
      cell,view,facet,mode);
    fflush(stderr);
    }
  theFacet->type = OCT_FACET;
  theFacet->contents.facet.cell = cell;
  theFacet->contents.facet.view = view;
  theFacet->contents.facet.facet = facet;
  theFacet->contents.facet.mode = mode;
  theFacet->contents.facet.version = OCT_CURRENT_VERSION;
  if (octOpenFacet(theFacet) < OCT_OK) {
    printf("Error: File %s:%s does not exist\n",cell,view);
    (void) exit(-1);
    }
  }




/* 
 *  check_flag() parses the commande line.  prints a usage message
 *      given bogus flags 
 */

check_flag(itemsNo, itemsArray) 
  int itemsNo;
  char *itemsArray[]; {

  char *ptr;
  int c;
  int i;
  int retcode;

  i=1;
  retcode=1;
  ARGFLAG=0;
  while (i < itemsNo) {
    if(*(itemsArray[i]) == '-') {
      itemsArray[i]++;
      switch(c = *(itemsArray[i])) {
        case 't' : ptr = itemsArray[++i];
                   if(strcmp(ptr,"unit")==0)
                     delay_tech = UNIT_DELAY_TECH;
                   else if(strcmp(ptr,"typ")==0)
                     delay_tech = TYP_DELAY_TECH;
                   else if(strcmp(ptr,"max")==0)
                     delay_tech = MAX_DELAY_TECH;
                   else {
                     fprintf(stderr,"\nUnknown delay type: %s\n",ptr);
                     show_usage();
                     retcode=0;
                     }
                   break;
        case 'l' : techFile = (char *) malloc(strlen(itemsArray[++i])+1);
                   strcpy(techFile,itemsArray[i]);
                   break;
        case 'o' : outputFile = (char *)malloc(strlen(itemsArray[++i])+5);
                   strcpy(outputFile,itemsArray[i]);
                   break;
        case 'V' : cell_lib = (char *)malloc(strlen(itemsArray[++i])+5);
                   strcpy(cell_lib,itemsArray[i]);
                   break;
        case 'v' : spec_view = (char *) copy_str(itemsArray[++i],1);
                   break;
        case 'P' : spec_prog = (char *) copy_str(itemsArray[++i],1);
                   break;
        case 'a'  :ARGFLAG=1;
   		   break;
        case 'd'  :debug=1;
   		   break;
        case '/'  :show_usage();
                   retcode=0;
   		   break;
        case 'u'  :show_usage();
                   retcode=0;
   		   break;
        default   :fprintf(stderr,"\nIllegal option: %c\n",c);
                   show_usage();
                   retcode=0;
                   break;
        }
      }
    else {
      cell_n_view = (char *) malloc(strlen(itemsArray[i])+1);
      strcpy(cell_n_view,itemsArray[i]);
      }
    i++;
    }
  if((cell_n_view == NULL) && (retcode == 1)){
    fprintf(stderr,"\nNo cellname given\n");
    show_usage();
    retcode=0;
    }
  return(retcode);
  }



/* 
 *  show_usage() prints a usage message
 */

show_usage() {

  char *ptr;

  if(prog==PROG_HILO)
    ptr = "oct2hilo";
  else
    ptr = "oct2vhdl";

  fprintf(stderr,"\nUsage: %s [options] cell:view [options]\n\n",ptr);
  fprintf(stderr,"  -t {unit,typ,max}  -- use {unit,typ,max} delays\n");
  fprintf(stderr,"                        typ is default\n");
  fprintf(stderr,"  -l path            -- dir or file for tech files\n");
  if(prog==PROG_HILO)
    fprintf(stderr,"                        ~octtools/lib/oct2hilo is default\n");
  else
    fprintf(stderr,"                        ~octtools/lib/oct2vhdl is default\n");
  fprintf(stderr,"  -o outfile         -- name of output file\n");
  if(prog==PROG_HILO)
    fprintf(stderr,"                        cell.cct is default\n");
  else
    fprintf(stderr,"                        cell.vhd is default\n");
  if(prog==PROG_VHDL) {
    fprintf(stderr,"  -V library_name    -- name of vhdl cell library\n");
    fprintf(stderr,"                        %s is default\n",DEFAULT_CELL_LIB);
    }
  fprintf(stderr,"  -u                 -- show usage information\n");
  fprintf(stderr,"  -v viewtype        -- specify view type: schematic, gem, or\n");
  fprintf(stderr,"                        symbolic.  default is specified view \n");
  fprintf(stderr,"                        given in cell:view\n");
  fprintf(stderr,"  -d                 -- print debugging information\n\n");
  fflush(stderr);
  }


/*
 * resolve_techFile() takes the 'view' passed to it and looks for a tech
 *     file.  If no tech file or lib has been specified with the -l flag
 *     it looks in DEFTECHLIB for the appropriate tech file.  If techFile
 *     is a directory it will look in there.  If techFile is a file, it
 *     will use it as the tech file.
 */

resolve_techFile(view) 
  char *view;  {

  char *ptr;
  struct stat statbuf;

  /* if the user hasn't specified a tech file: */
  if(techFile==NULL) { 
    /* use default schematic tech file in default lib using delay_tech */
    ptr = (char *)util_file_expand(DEFTECHLIB);
    techFile = (char *) malloc(strlen(ptr)+strlen(view)+strlen(delay_tech)+1);
    strcpy(techFile,ptr);
    strcat(techFile,view);
    strcat(techFile,delay_tech);
    }
  else {
   
    /* if techFile is a file, use it.  If not append SCHEM & delay_tech */
    if(stat(techFile,&statbuf)==0) {
      if((statbuf.st_mode&S_IFREG)==S_IFREG) {
        /* don't need to do anything here as techFile has already been given */
        }
      else if((statbuf.st_mode&S_IFDIR)==S_IFDIR) {
        ptr = (char *) malloc(strlen(techFile)+strlen(view)+strlen(delay_tech)+2);
        strcpy(ptr,techFile);
        strcat(ptr,"/");
        strcat(ptr,view);
        strcat(ptr,delay_tech);
        free(techFile);
        techFile = ptr;
        if(debug>0) {
          fprintf(stderr,"Using %s as tech file\n",ptr);
          fflush(stderr);
          }
        }
      else {
        fprintf(stderr,"%s is not a file or directory\n",techFile);
        fflush(stderr);
        exit(-1);
        }
      }
    else {
      fprintf(stderr,"Unable to stat %s \n",techFile);
      fflush(stderr);
      exit(-1);
      }
    }
  }



/*
 * do_schematic() processes a schematic view and produces a hilo
 *     .cct file
 */
do_schematic() {

  resolve_techFile("SCHEM_");
  
  open_old_facet(&ifacet, cell, view, "interface", "r");
  load_table();
  read_terminals();
  if (prog==PROG_VHDL)
    vhdl_header();
  else if(prog==PROG_HILO)
    hilo_header();
  if (octGetByName(&cfacet,&bag) == OCT_OK) {
    make_inst(&bag);
    if (prog==PROG_VHDL)
      write_vhdl();
    else if(prog==PROG_HILO)
      write_hilo();
    }
  else {
    fprintf(stderr,"ERROR GETTING BAG FOR CONTENT FACET\n");
    }
  } 


/*
 * do_gem() processes a gem view and produces a hilo .cct file
 */
do_gem() {

  resolve_techFile("GEM_");
  read_terminals();/*try this */
  makeGmuInst();
  outGmuInst();
  }

/*
 * do_symbolic() processes a symbolic view and produces a hilo
 *     .cct file
 */
do_symbolic() {

  resolve_techFile("SYM_");

  /* open interface facet of cell:view */
  open_old_facet(&ifacet, cell, view, "interface", "r");

  /* read tech file */
  load_table();
  read_terminals();

  if (prog==PROG_VHDL)
    vhdl_header();
  else if(prog==PROG_HILO)
    hilo_header();

  if (octGetByName(&cfacet,&bag) == OCT_OK) {
    make_inst(&bag);
    if (prog==PROG_VHDL)
      write_vhdl();
    else if(prog==PROG_HILO)
      write_hilo();
    }
  }

