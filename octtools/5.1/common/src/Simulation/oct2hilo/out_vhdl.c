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
extern char *cct_name;
extern char *cell_lib;
extern char *outputFile;

char *buffer;
char *entity_name;

#define MAXLINLEN 79


write_vhdl() {

  strcpy(buffer,"ARCHITECTURE structural OF ");
  strcat(buffer,entity_name);
  strcat(buffer," IS\n");
  fputs(buffer,fp);

  vhdl_components();
  vhdl_usage();
  vhdl_signals();
  vhdl_alias_signals();

  strcpy(buffer,"BEGIN\n");
  fputs(buffer,fp);

  vhdl_alias();
  vhdl_gates();

  strcpy(buffer,"END structural;\n");
  fputs(buffer,fp);

/*  vhdl_alias();
  vhdl_supply0();
  vhdl_supply1();
  vhdl_input();
  vhdl_end();*/
  }


/* write the VHDL style header for the circuit */
vhdl_header() {
  char *ptr1,*ptr2;
  int i,j,k;
  int linlen,strlength; 

#ifdef notdef
  if (outputFile==NULL) {
    outputFile=(char *)malloc(strlen(cct_name)+5);
    strcpy(outputFile,cct_name);
    strcat(outputFile,".vhd");
    }

  fprintf(stderr,"Your VHDL Circuit is being stored on %s \n",outputFile);
  
  entity_name = cct_name;
  /* check for leading digit in circuit name */
  if(isdigit(*cct_name)) {
    entity_name = (char *) malloc(strlen(cct_name)+2);
    strcpy(entity_name,"E");
    strcat(entity_name,cct_name);
    fprintf(stderr,"Your VHDL Entity has been renamed %s \n\n",entity_name);
    }

  if((fp=fopen(outputFile,"w"))==NULL) {
    fprintf(stderr,"oct2vhdl: unable to open %s for output\n",outputFile);
    exit(-1);
    }
  /* write header & circuti name */
  strcpy(buffer,"ENTITY ");
  strcat(buffer,entity_name);
  strcat(buffer," IS\n");
  fputs(buffer,fp);

  if((j_i!=0)&&(a_o!=0)) {
    strcpy(buffer,"  PORT (");
    linlen = strlen(buffer);
  } 
 
  if(j_i!=0) {
    /* write input signal names */
    for (j=0;j<j_i;j++) {
      strlength = strlen(input[j]);
      if((linlen+strlength)>MAXLINLEN){ 
        strcat(buffer,"\n     ");
        fputs(buffer,fp);
        *buffer = '\0';
        linlen = 5;
        }
      strcat(buffer,input[j]);
      linlen = strlength + linlen + 1;
      if(j!=(j_i-1))
        strcat(buffer,",");
      }
    strlength = strlen(": IN BIT; ");
    if((linlen+strlength)>MAXLINLEN){ 
      strcat(buffer,"\n     ");
      fputs(buffer,fp);
      *buffer = '\0';
      linlen = 5;
      }   
    strcat(buffer,": IN BIT; ");
    linlen = strlength + linlen + 1;
    fputs(buffer,fp);
    *buffer = '\0';
    }

  if(a_o!=0) {
    /* write output signal names */
    for (j=0;j<a_o;j++) {
      strlength = strlen(aliasOutput[j].termname);
      if((linlen+strlength)>MAXLINLEN){ 
        strcat(buffer,"\n     ");
        fputs(buffer,fp);
        *buffer = '\0';
        linlen = 5;
        }
      strcat(buffer,aliasOutput[j].termname);
      if(j!=(a_o-1))
        strcat(buffer,",");
      linlen = strlength + linlen +1;
      }
    strlength = strlen(": OUT BIT);\n");
    if((linlen+strlength)>MAXLINLEN){ 
      strcat(buffer,"\n     ");
      fputs(buffer,fp);
      *buffer = '\0';
      linlen = 5;
      }   
    strcat(buffer,": OUT BIT);\n");
    linlen = strlength + linlen + 1;
    fputs(buffer,fp);
    }
  else {
    strcat(buffer,");\n");
    fputs(buffer,fp);
    }

  strcpy(buffer,"END ");
  strcat(buffer,entity_name);
  strcat(buffer,";\n");
  fputs(buffer,fp);

  /* wrap it up */
  strcpy(buffer,"\n");
  fputs(buffer,fp);

#endif notdef
  }


vhdl_components() {
  int i,k;
  int dir;

  for (i=0;i<inst_cnt;i++) {
    strcpy(buffer,"  COMPONENT ");
    strcat(buffer,primitives[my_inst[i].prim_num].primitive_name);
    strcat(buffer," PORT (");
    dir = my_inst[i].term_dir[0];
    for(k=0;k<my_inst[i].term_cnt;k++) {
      if(dir!=my_inst[i].term_dir[k]) {
        buffer[strlen(buffer)-1] = '\0';
        if(dir==TERM_OUT)
          strcat(buffer,": OUT BIT;");
        else
          strcat(buffer,": IN BIT;");
        }
      strcat(buffer,my_inst[i].term_name[k]);
      strcat(buffer,",");
      dir = my_inst[i].term_dir[k];
      }
    buffer[strlen(buffer)-1] = '\0';
    if(dir==TERM_OUT)
      strcat(buffer,": OUT BIT);\n");
    else
      strcat(buffer,": IN BIT);\n");
    fputs(buffer,fp);
    strcpy(buffer,"    END COMPONENT;\n");
    fputs(buffer,fp);
    }
  }

vhdl_usage() {
  int i,k;

  for (i=0;i<inst_cnt;i++) {
    if((k = get_prim_num(my_inst[i].inst_numb))== -1) {
      fprintf(stderr,"Can't map cell %s, aborting\n",my_inst[i].inst_numb);
      exit(-1);
      }
    strcpy(buffer,"  FOR ALL: ");
    strcat(buffer,primitives[k].primitive_name);
    strcat(buffer," USE ENTITY ");
    strcat(buffer,cell_lib);
    strcat(buffer,".");
    strcat(buffer,primitives[k].primitive_name);
    strcat(buffer,"(functional);\n");
    fputs(buffer,fp);
    }
  }



vhdl_gates() {

  int i,k;
  int prim_num;

  for (i=0;i<inst_cnt;i++) {
    prim_num = get_prim_num(my_inst[i].inst_numb);
    for(k=my_inst[i].gate;(k != -1);k=my_gate[k].next) {
      strcpy(buffer,"  "); 
      strcat(buffer,my_gate[k].g_name);
      strcat(buffer,": ");
      strcat(buffer,primitives[prim_num].primitive_name);
      strcat(buffer," PORT MAP (");
      vhdl_term(k);
      buffer[strlen(buffer)-1] = '\0';
      strcat(buffer,");\n");
      fputs(buffer,fp);
      }
    }
  }


vhdl_term(g_num)
  int  g_num; {
  int k,a=0;
  char c;

  for (k=0;my_gate[g_num].term[k]!=NULL;k++) {
    strcat(buffer,my_gate[g_num].term[k]);
    strcat(buffer,",");
    }
  }


vhdl_wires() {
  vhdl_signals();
  }


vhdl_signals() {
  int i,j,isInput,isOutput;
  int linlen, strlength;
  extern int UNKN;

#ifdef notdef
  isInput = 0;
  strcpy(buffer,"  SIGNAL ");
  linlen = strlen(buffer);
  if(UNKN) {
    for (i=0;i<j_o;i++) {
      for(j=0;j<j_i;j++) {
        if(strcmp(outputs[i],input[j])==0) {
          isInput=1; 
          break;
          }
        }
      if(isInput==0) {
        /* check for aliased output too */
        isOutput=0; 
        for(j=0;j<a_o;j++) {
          if(strcmp(outputs[i],aliasOutput[j].netname)==0) {
            isOutput=1; 
            break;
            }
          }
         }
      if((isInput==0) && (isOutput==0)) {
        strlength = strlen(outputs[i]);
        if((linlen+strlength)>MAXLINLEN) {
          strcat(buffer,"\n     ");
          fputs(buffer,fp);
          *buffer = '\0';
          linlen = 5;
          }
	strcat(buffer,outputs[i]);
	linlen = linlen + strlength + 1;
        if(i!=(j_o-1))
  	  strcat(buffer,",");
        }
      }
    }
  else {
    for (i=0;i<j_o;i++) {
      if(*outputs[i]!='\0') {
        strlength = strlen(outputs[i]);
	if((linlen+strlength)>MAXLINLEN) {
          strcat(buffer,"\n     ");
          fputs(buffer,fp);
          *buffer = '\0';
          linlen = 5;
          }
	strcat(buffer,outputs[i]);
	linlen = linlen + strlength + 1;
        if(i!=(j_o-1))
	  strcat(buffer,",");
        }
      }
    }
  if(buffer[strlen(buffer)-1]==',')
    buffer[strlen(buffer)-1]='\0';
  strcat(buffer,": BIT;\n");
  fputs(buffer,fp);
#endif notdef
  }


/* write signal line for aliased inputs */
vhdl_alias_signals() {
  int i;

#ifdef notdef
  if(a_i>0) {
    strcpy(buffer,"  SIGNAL ");
    for(i=0;i<a_i;i++) {
      strcat(buffer,aliasInput[i].netname);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,": BIT;\n");
    fputs(buffer,fp);
    }
  if(a_o>0) {
    strcpy(buffer,"  SIGNAL ");
    for(i=0;i<a_o;i++) {
      if(strcmp(aliasOutput[i].termname,aliasOutput[i].netname) ){
        strcat(buffer,aliasOutput[i].netname);
        strcat(buffer,",");
        }
      }
    if(buffer[strlen(buffer)-1]==',') {
      buffer[strlen(buffer)-1] = '\0';
      strcat(buffer,": BIT;\n");
      fputs(buffer,fp);
      }
    }
#endif notdef
  }


vhdl_alias() {
  int i;

#ifdef notdef
  if(*(aliasOutput[0].termname)!=NULL) {
    for(i=0;i<a_o;i++) {
      if(strcmp(aliasOutput[i].termname,aliasOutput[i].netname) ){
        strcpy(buffer,"  ");
        strcat(buffer,aliasOutput[i].termname);
        strcat(buffer," <= ");
        strcat(buffer,aliasOutput[i].netname);
        strcat(buffer," AFTER 0 NS;\n");
        fputs(buffer,fp);
        }
      }
    }
  if(a_i>0) {
    for(i=0;i<a_i;i++) {
      strcpy(buffer,"  ");
      strcat(buffer,aliasInput[i].netname);
      strcat(buffer," <= ");
      strcat(buffer,aliasInput[i].termname);
      strcat(buffer," AFTER 0 NS;\n");
      fputs(buffer,fp);
      }
    }
#endif notdef
  }

vhdl_input() {
  int i,a;
  int linlen, strlength;

  strcpy(buffer,"INPUT ");
  linlen = strlen(buffer);
  for (i=0;i<j_i;i++) {
    strlength = strlen(input[i]);
    if((linlen+strlength)>MAXLINLEN) {
      strcat(buffer,"\n     ");
      fputs(buffer,fp);
      *buffer = '\0';
      linlen = 5;
      }
    strcat(buffer,input[i]);
    linlen = linlen + strlength + 1;
    if(i!=(j_i-1))
      strcat(buffer,",");
    }

  strcat(buffer,";\n");
  fputs(buffer,fp);
}


vhdl_supply1() {
  int i,a;

  if(j_s1>0) {
    strcpy(buffer,"SUPPLY1 ");
    for (i=0;i<j_s1;i++) {
      strcpy(buffer,supply1[i]);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fputs(buffer,fp);
    }
  }

vhdl_supply0() {
  int i,a;

  if(j_s0>0) {
    strcpy(buffer,"SUPPLY0 ");
    for (i=0;i<j_s0;i++) {
      strcpy(buffer,supply0[i]);
      strcat(buffer,",");
      }
    buffer[strlen(buffer)-1] = '\0';
    strcat(buffer,";\n");
    fputs(buffer,fp);
    }
  }


vhdl_end() {

  fprintf(fp,".\n");
  }

