#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"

extern octObject cfacet;
extern octObject ifacet;
extern char *buffer;
extern char *cct_name;
extern char *outputFile;
extern int  debug;
extern int  have_vdd;
extern int  have_gnd;

extern struct INT_terms INT_terms;

FILE *fp, *fopen();
int j_i=0,j_o=0,j_s1=0,j_s0=0;
int a_o=0,a_i=0;
octObject bag;
char *input[500];
char *outputs[500];
char *supply1[100];
char *supply0[100];

struct IO_term *IO_in_head,*IO_in_cur;
struct IO_term *IO_out_head,*IO_out_cur;
struct IO_term *IO_inout_head,*IO_inout_cur;


#define MAXLINLEN 79


/* read the IO terminals of the circuit */
read_terminals() {
  octObject termobj, term_name, term_dir,net;
  octGenerator tgen,ngen;
  char *termptr,*netptr;
  struct IO_term *tmp;

  /* initialize internal terminal list pointers */
  INT_terms.all_head = NULL;
  INT_terms.all_cur = NULL;
  INT_terms.in_head = NULL;
  INT_terms.in_cur = NULL;
  INT_terms.out_head = NULL;
  INT_terms.out_cur = NULL;
  INT_terms.inout_head = NULL;
  INT_terms.inout_cur = NULL;
  INT_terms.supply0_head = NULL;
  INT_terms.supply0_cur = NULL;
  INT_terms.supply1_head = NULL;
  INT_terms.supply1_cur = NULL;

  /* initialize interface terminal list pointers */
  IO_in_head = IO_in_cur = NULL;
  IO_out_head = IO_out_cur = NULL;
  IO_inout_head = IO_inout_cur = NULL;

  /* initialize generator for cfacet */
  o_init_gen_contents(&cfacet, OCT_TERM_MASK, &tgen);

  /* while we have terms...*/
  while (octGenerate(&tgen, &termobj) == OCT_OK) {
    /* get terminal */
    term_name.type = OCT_TERM;
    term_name.contents.term.name = termobj.contents.term.name;
    if (octGetByName(&ifacet, &term_name)  != OCT_OK) {
      fprintf(stderr,"read_terms: octGetByName failed\n");
      } 
    /* get direction of terminal */
    term_dir.type = OCT_PROP;
    term_dir.contents.prop.name = "DIRECTION";
    if (octGetByName(&term_name, &term_dir) != OCT_OK) {
      term_dir.contents.prop.value.string = "UNSPEC";
      }

    /* If we have an output terminal */
    if (strcmp(term_dir.contents.prop.value.string, "OUTPUT") == 0) {
      /* try to get it's net name */
      o_init_gen_containers(&termobj, OCT_NET_MASK, &ngen);
      if (octGenerate(&ngen, &net) == OCT_OK) {
        /* make sure we don't blow out on the Sun */
        if (net.contents.net.name!=NULL) {

          /* allocate new IO_term struct and add it to the tree*/
          tmp = (struct IO_term *) malloc(sizeof(struct IO_term));
          if(IO_out_head==NULL) {
            IO_out_head = tmp;
            IO_out_cur = tmp;
            }
          else {
            IO_out_cur->next = tmp;
            IO_out_cur = tmp;
            }
         
          /* if the names don't match, record both names */
          netptr = net.contents.net.name;
          termptr = termobj.contents.net.name;
          if(debug>=1) {
            fprintf(stderr,"read_terms: OUTPUT netptr: %s termptr %s\n",
                netptr,termptr);
            fflush(stderr);
            }
          if(strcmp(netptr,termptr)!=0) {
            IO_out_cur->termname = (char *) fix_termName(termptr);
            IO_out_cur->termptr = (struct INT_term *) add_termName(netptr,TERM_OUT);
            IO_out_cur->netname = IO_out_cur->termptr->name;
            }
          else {
            IO_out_cur->termptr = (struct INT_term *) add_termName(termptr,TERM_OUT);
            IO_out_cur->termname = IO_out_cur->termptr->name;
            IO_out_cur->netname = NULL;
	    }
          }
        }
      } 
    else if (strcmp (term_dir.contents.prop.value.string, "INPUT") == 0) {
      /* try to get it's net name */
      o_init_gen_containers(&termobj, OCT_NET_MASK, &ngen);
      if (octGenerate(&ngen, &net) == OCT_OK) {
        /* make sure we don't blow out on the Sun */
        if (net.contents.net.name!=NULL) {

          /* if name is Vdd or GND, note it but don't add it to
           * any terminal list for now */
          if(strcmp(net.contents.net.name,"Vdd")==0) 
            have_vdd = 1;
          else if(strcmp(net.contents.net.name,"GND")==0) 
            have_gnd = 1;
          else {

            /* allocate new IO_term struct and add it to the tree*/
            tmp = (struct IO_term *) malloc(sizeof(struct IO_term));
            if(IO_in_head==NULL) {
              IO_in_head = tmp;
              IO_in_cur = tmp;
              }
            else {
              IO_in_cur->next = tmp;
              IO_in_cur = tmp;
              }
           
            /* if the names don't match, record both names */
            netptr = net.contents.net.name;
            termptr = termobj.contents.net.name;
            if(debug>=1) {
              fprintf(stderr,"read_terms: INPUT netptr: %s termptr %s\n",
                  netptr,termptr);
              fflush(stderr);
              }
            if(strcmp(netptr,termptr)!=0) {
              IO_in_cur->termname = (char *) fix_termName(termptr);
              IO_in_cur->termptr = (struct INT_term *) add_termName(netptr,TERM_IN);
              IO_in_cur->netname = IO_in_cur->termptr->name;
              }
            else {
              IO_in_cur->termptr = (struct INT_term *) add_termName(termptr,TERM_IN);
              IO_in_cur->termname = IO_in_cur->termptr->name;
              IO_in_cur->netname = NULL;
              }
            }
          }
        }
      }
    else if (strcmp (term_dir.contents.prop.value.string, "INOUT") == 0) {
      /* try to get it's net name */
      o_init_gen_containers(&termobj, OCT_NET_MASK, &ngen);
      if (octGenerate(&ngen, &net) == OCT_OK) {
        /* make sure we don't blow out on the Sun */
        if (net.contents.net.name!=NULL) {

          /* if name is Vdd or GND, note it but don't add it to
           * any terminal list for now */
          if(strcmp(net.contents.net.name,"Vdd")==0) 
            have_vdd = 1;
          else if(strcmp(net.contents.net.name,"GND")==0) 
            have_gnd = 1;
          else {

            /* allocate new IO_term struct and add it to the tree*/
            tmp = (struct IO_term *) malloc(sizeof(struct IO_term));
            if(IO_inout_head==NULL) {
              IO_inout_head = tmp;
              IO_inout_cur = tmp;
              }
            else {
              IO_inout_cur->next = tmp;
              IO_inout_cur = tmp;
              }
         
            /* if the names don't match, record both names */
            netptr = net.contents.net.name;
            termptr = termobj.contents.net.name;
            if(debug>=1) {
              fprintf(stderr,"read_terms: INOUT netptr: %s termptr %s\n",
                  netptr,termptr);
              fflush(stderr);
              }
            if(strcmp(netptr,termptr)!=0) {
              IO_inout_cur->termname = (char *) fix_termName(termptr);
              IO_inout_cur->termptr = (struct INT_term *) add_termName(netptr,TERM_INOUT);
              IO_inout_cur->netname = IO_inout_cur->termptr->name;
              }
            else {
              IO_inout_cur->termptr = (struct INT_term *) add_termName(termptr,TERM_INOUT);
              IO_inout_cur->termname = IO_inout_cur->termptr->name;
              IO_inout_cur->netname = NULL;
              }
            }
          }
        }
      }
    else if (debug>=1) {
      fprintf(stderr,"read_terms: unknown terminal direction: %s for %s\n",
         term_dir.contents.prop.value.string,termobj.contents.net.name);
      fflush(stderr);
      }
    }
  bag.type = OCT_BAG;
  bag.contents.bag.name = "INSTANCES";
  }

