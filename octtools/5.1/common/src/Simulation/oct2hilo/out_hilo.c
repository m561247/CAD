#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"


extern int j_i;
extern char *input[];
extern char *outputs[];
extern char *supply0[];
extern char *supply1[];
extern int j_o;
extern int j_s1;
extern int j_s0;
extern int have_vdd;
extern int have_gnd;
extern char *cct_name;
extern char *outputFile;
extern struct IO_term *IO_in_head,*IO_in_cur;
extern struct IO_term *IO_out_head,*IO_out_cur;
extern struct IO_term *IO_inout_head,*IO_inout_cur;
extern struct INT_terms INT_terms;

int have_pullup;
int have_pulldown;


char *buffer;

#define MAXLINLEN 79


/*
 *  write_hilo() writes the GHDL description of the circut. note 
 *      that the header has already been written
 */

write_hilo() {
  hilo_gates();
  hilo_wires();
  hilo_alias();
  hilo_supply0();
  hilo_supply1();
  hilo_input();
  hilo_inout();
  hilo_end();
  }


/* 
 * hilo_header() writes the HILO style header for the circuit 
 */

hilo_header() {
  char *ptr1,*ptr2;
  int i,j,k;
  int linlen,strlength; 
  /*struct IO_term *tmp,*tmp2,*tmp3 ;*/
  struct sortlist *slist,*tmp;

  /* has an output file been specified? if not, make one out of the cct name */
  if (outputFile==NULL) {
    outputFile=(char *)malloc(strlen(cct_name)+5);
    strcpy(outputFile,cct_name);
    strcat(outputFile,".cct");
    }

  fprintf(stderr,"Your Hilo Circuit is being stored on %s \n",outputFile);

  /* open the output file */
  if((fp=fopen(outputFile,"w"))==NULL) {
    fprintf(stderr,"oct2hilo: unable to open %s for output\n",outputFile);
    exit(-1);
    }

  /* write header & circuit name */
  strcpy(buffer,"CCT ");
  strcat(buffer,cct_name);
  strcat(buffer,"(");
  linlen = strlen(buffer); 
 
  /* create a sortlist of the terminals */
  slist = (struct sortlist*) IO_2_sortlist(IO_out_head);
  /* sort the list */
  sort_list(&slist);
  /* vectorize the list */
  slist = (struct sortlist*) vectorize(slist);

  /* write output signal names */
  for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
    strlength = strlen(tmp->str);

    if((linlen+strlength)>MAXLINLEN){ 
      strcat(buffer,"\n     ");
      fwrite(buffer,1,strlen(buffer),fp);
      *buffer = '\0';
      linlen = 5;
      }
    strcat(buffer,tmp->str);
    strcat(buffer,",");
    linlen = strlength + linlen +1;
    }
  free_sortlist(slist);


  /* create a sortlist of the terminals */
  slist = (struct sortlist*) IO_2_sortlist(IO_in_head);
  /* sort the list */
  sort_list(&slist);
  /* vectorize the list */
  slist = (struct sortlist*) vectorize(slist);

  /* write input signal names */
  for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
    strlength = strlen(tmp->str);
    if((linlen+strlength)>MAXLINLEN){ 
      strcat(buffer,"\n     ");
      fwrite(buffer,1,strlen(buffer),fp);
      *buffer = '\0';
      linlen = 5;
      }
    strcat(buffer,tmp->str);
    linlen = strlength + linlen + 1;
    strcat(buffer,",");
    }
  free_sortlist(slist);

  /* create a sortlist of the terminals */
  slist = (struct sortlist*) IO_2_sortlist(IO_inout_head);
  /* sort the list */
  sort_list(&slist);
  /* vectorize the list */
  slist = (struct sortlist*) vectorize(slist);

  /* write inout signal names */
  for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
    strlength = strlen(tmp->str);
    if((linlen+strlength)>MAXLINLEN){ 
      strcat(buffer,"\n     ");
      fwrite(buffer,1,strlen(buffer),fp);
      *buffer = '\0';
      linlen = 5;
      }
    strcat(buffer,tmp->str);
    linlen = strlength + linlen + 1;
    strcat(buffer,",");
    }
  free_sortlist(slist);

  /* clean up any trailing ','s */
  if(buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1] = '\0';

  /* wrap it all up */
  strcat(buffer,")\n");
  fwrite(buffer,1,strlen(buffer),fp);
  }



/*
 * hilo_gates() prints the cells, instantiations and their signals 
 */

hilo_gates() {

  int i,k;

  /* for each type of cell used */
  for (i=0;i<inst_cnt;i++) {
    prim_name_n_delays(my_inst[i].inst_numb,i);
    /* for each instance of that cell */
    for(k=my_inst[i].gate;my_gate[k].next!=-1;k=my_gate[k].next) {
      strcpy(buffer,"     "); /*indent string*/
      strcat(buffer,my_gate[k].g_name);
      strcat(buffer,"(");
      hilo_term(k);
      buffer[strlen(buffer)-1] = '\0';
      strcat(buffer,")\n");
      fwrite(buffer,1,strlen(buffer),fp);
      }
    strcpy(buffer,"     "); /*indent string*/
    strcat(buffer,my_gate[k].g_name);
    strcat(buffer,"(");
    hilo_term(k);
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,");\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  }


/*
 * hilo_term() prints the terminals associated with each instantiation
 */

hilo_term(g_num)
  int  g_num; {
  int k,a=0;
  char c;

  for (k=0;my_gate[g_num].term[k]!=NULL;k++) {
    strcat(buffer,my_gate[g_num].term[k]);
    strcat(buffer,",");
    }
  if(have_pullup == 1)
    strcat(buffer,"Tvdd,");
  else if(have_pulldown == 1)
    strcat(buffer,"Tgnd,");
  }


/* 
 * hilo_wires() prints the WIRE list of the circuit.  
 */

hilo_wires() {
  int i,j,isInput;
  int linlen, strlength;
  extern int UNKN;
  struct sortlist *tmp,*slist;

  if (INT_terms.out_head!=NULL) {
    strcpy(buffer,"WIRE ");
    linlen = strlen(buffer);

    slist = (struct sortlist*) INT_2_sortlist(INT_terms.out_head);
    sort_list(&slist);
    slist = (struct sortlist*) vectorize(slist);
    for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
      if(strcmp(tmp->str,"Tvdd")==0) 
        have_vdd = 1;
      else if(strcmp(tmp->str,"Tgnd")==0) 
        have_gnd = 1;
      else {
        strlength = strlen(tmp->str);
        if((linlen+strlength)>MAXLINLEN) {
          strcat(buffer,"\n     ");
          fwrite(buffer,1,strlen(buffer),fp);
          *buffer = '\0';
          linlen = 5;
          }
        strcat(buffer,tmp->str);
        linlen = linlen + strlength + 1;
        strcat(buffer,",");
        }
      }
    buffer[strlen(buffer)-1]='\0';
    strcat(buffer,";\n");
    fwrite(buffer,1,strlen(buffer),fp);
    free_sortlist(slist);
    }
  }



/* 
 * hilo_alias() prints the aliases from the circuit.  
 */

hilo_alias() {
  int i;
  struct IO_term *tmp;

  /* print aliases outputs */
  for(tmp=IO_out_head;tmp!=NULL;tmp=tmp->next) {
    if(tmp->netname!=NULL) {
      strcpy(buffer,"WIRE ");
      strcat(buffer,tmp->termname);
      strcat(buffer," = ");
      strcat(buffer,tmp->netname);
      strcat(buffer,";\n");
      fwrite(buffer,1,strlen(buffer),fp);
      }
    }

  /* print aliases inputs */
  for(tmp=IO_in_head;tmp!=NULL;tmp=tmp->next) {
    if(tmp->netname!=NULL) {
      strcpy(buffer,"WIRE ");
      strcat(buffer,tmp->netname);
      strcat(buffer," = ");
      strcat(buffer,tmp->termname);
      strcat(buffer,";\n");
      fwrite(buffer,1,strlen(buffer),fp);
      }
    }

  /* not so sure about this one */
  for(tmp=IO_inout_head;tmp!=NULL;tmp=tmp->next) {
    if(tmp->netname!=NULL) {
      strcpy(buffer,"WIRE ");
      strcat(buffer,tmp->netname);
      strcat(buffer," = ");
      strcat(buffer,tmp->termname);
      strcat(buffer,";\n");
      fwrite(buffer,1,strlen(buffer),fp);
      }
    }
  }



/* 
 * hilo_input() prints the inputs to the circuit.  
 */

hilo_input() {
  int i,a;
  int linlen, strlength;
  struct sortlist *tmp,*slist;

  if(IO_in_head != NULL) {
    strcpy(buffer,"INPUT ");
    linlen = strlen(buffer);
    slist = (struct sortlist*) IO_2_sortlist(IO_in_head);
    sort_list(&slist);
    slist = (struct sortlist*) vectorize(slist);
    for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
      strlength = strlen(tmp->str);
      if((linlen+strlength)>MAXLINLEN) {
        strcat(buffer,"\n     ");
        fwrite(buffer,1,strlen(buffer),fp);
        *buffer = '\0';
        linlen = 5;
        }
      strcat(buffer,tmp->str);
      linlen = linlen + strlength + 1;
      strcat(buffer,",");
      }

    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  }



/* 
 * hilo_inout() prints the inout signals of the circuit.  
 */

hilo_inout() {
  int i,a;
  int linlen, strlength;
  struct sortlist *tmp,*slist;

  if(IO_inout_head != NULL) {
    strcpy(buffer,"TRI ");
    linlen = strlen(buffer);
    slist = (struct sortlist*) IO_2_sortlist(IO_inout_head);
    sort_list(&slist);
    slist = (struct sortlist*) vectorize(slist);
    for (tmp=slist;tmp!=NULL;tmp=tmp->next) {
      strlength = strlen(tmp->str);
      if((linlen+strlength)>MAXLINLEN) {
        strcat(buffer,"\n     ");
        fwrite(buffer,1,strlen(buffer),fp);
        *buffer = '\0';
        linlen = 5;
        }
      strcat(buffer,tmp->str);
      linlen = linlen + strlength + 1;
      strcat(buffer,",");
      }

    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  }



/* 
 * hilo_supply1() prints the Vdd signals of the circuit.  
 */

hilo_supply1() {
  int i,a;
  struct INT_term *tmp;

  if(INT_terms.supply1_head != NULL) {
    strcpy(buffer,"SUPPLY1 ");
    for(tmp=INT_terms.supply1_head;tmp!=NULL;tmp=tmp->next) {
      strcpy(buffer,tmp->name);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  else if (have_vdd == 1) {
    strcpy(buffer,"SUPPLY1 Tvdd;\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  }


/* 
 * hilo_supply0() prints the GND signals of the circuit.  
 */

hilo_supply0() {
  int i,a;
  struct INT_term *tmp;

  if(INT_terms.supply0_head != NULL) {
    strcpy(buffer,"SUPPLY0 ");
    for(tmp=INT_terms.supply0_head;tmp!=NULL;tmp=tmp->next) {
      strcpy(buffer,tmp->name);
      strcpy(buffer,supply0[i]);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  else if (have_gnd == 1) {
    strcpy(buffer,"SUPPLY0 Tgnd;\n");
    fwrite(buffer,1,strlen(buffer),fp);
    }
  }



/* 
 * hilo_end() prints the end of the circuit description
 */

hilo_end() {

  fprintf(fp,".\n");
  }



/* 
 *  prim_name_n_delays() looks up gate in table from tech file 
 *      and writes out delays if there are any 
 */

prim_name_n_delays(thename,pos)
  char *thename;
  int pos; {
  char *realName;
  int i,j,k;

  have_pullup = 0;
  have_pulldown = 0;
  if((i=get_prim_num(thename))!= -1) 
    realName= (char *)primitives[i].primitive_name;
  else {
    realName=thename;
    fprintf(stderr,"WARNING: %s was not mapped to MSU cell\n",realName);
    }

  /* handle pullups and pulldowns by forcing substitutions */
  if((strcmp(realName,"pullup")==0) || (strcmp(realName,"pulldown")==0))
    strcpy(buffer,"load");
  else
    strcpy(buffer,realName);
  if((i != -1)&&(primitives[i].delCnt>0)) {
    strcat(buffer," (");
    for(k=0;k<primitives[i].delCnt;k++) {
      strcat(buffer,primitives[i].delay[k]);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1]=')';
    }
  
  if(strcmp(realName,"pullup")==0) {
    strcat(buffer," ** pullup");
    have_pullup = 1;
    have_vdd = 1;
    }
  else if(strcmp(realName,"pulldown")==0){
    strcat(buffer," ** pulldown");
    have_pulldown = 1;
    have_gnd = 1;
    }
  strcat(buffer,"\n");
  fputs(buffer,fp);
  }

