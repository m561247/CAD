#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"

extern char *buffer;
extern FILE *fp;
extern int GmuInstCnt;
extern char *circuitType;
extern char *cct_name;
extern int GmuInstCnt;
extern char *outputs[];;
extern char *input[];;

outGmuInst () {

  int i,j,k,a;
  int b;
  char func[6],*gate,*outInst,*input;
  int max, haveInst;

  getOutTerm();
  getInTerm();
  if(prog==PROG_VHDL)
    vhdl_header();
  else if(prog==PROG_VHDL)
    hilo_header();

  /*gmuHeader();*/

  if(strcmp(circuitType,"p")==0) 
    strcpy(func,"nand ");
  else 
    strcpy(func,"nor ");

  max=get_max();

  for (i=1;i<=max;i++) {
    haveInst = 0;
    strcpy(buffer,func);
    strcat(buffer,"\n");
    for(j=0;j<GmuInstCnt;j++) {
      if(gmuInstance[j].numInst==i) {
        fwrite(buffer,1,strlen(buffer),fp);
        haveInst = 1;
        strcpy(buffer,"     G");
        strcat(buffer, gmuInstance[j].instName+1);
        strcat(buffer,"(");
        strcat(buffer, gmuInstance[j].instName);
        strcat(buffer,",");
	for(k=0;k<gmuInstance[j].numInst;k++)  {
          strcat(buffer, gmuInstance[j].output[k]);
          strcat(buffer,",");
          }
        buffer[strlen(buffer)-1] = '\0';
        strcat(buffer,")\n");
        }
      }
    if(haveInst==1) {
      buffer[strlen(buffer)-1] = '\0';
      strcat(buffer,";\n");
      fwrite(buffer,1,strlen(buffer),fp);
      }
    }

  if (prog==PROG_VHDL) {
    vhdl_wires();
    vhdl_input();
    vhdl_end();
    }
  else if(prog==PROG_HILO) {
    hilo_wires();
    hilo_input();
    hilo_end();
    }
  }

int get_max() {

  int i,max;
  
  max = 0;
  for(i=0;i<GmuInstCnt;i++) 
    if(gmuInstance[i].numInst>max) 
      max=gmuInstance[i].numInst;
  return max;
  }

gmuHeader() {

  int a,i;
  char *filename;

  filename = (char *)malloc(strlen(cct_name)+6);
  strcpy(filename,cct_name);
  strcat(filename,".hilo");
  fp=fopen(filename,"w");
  strcpy(buffer,"CCT ");
  strcat(buffer,cct_name);
  strcat(buffer,"(");

  for(i=0;i<j_o;i++) {
    strcat(buffer,outputs[i]);
    strcat(buffer,",");
    }
  for(i=0;i<j_i;i++) {
    strcat(buffer,input[i]);
    strcat(buffer,",");
    }

  buffer[strlen(buffer)-1] = '\0';
  strcat(buffer,")\n");
  fwrite(buffer,1,strlen(buffer),fp);
  }

getOutTerm() {
  int i,a;

  for(i=0;i<GmuInstCnt;i++) 
    outputs[j_o++]=gmuInstance[i].instName;
  }


getInTerm() {

  int i,a,j,k,l,found,isThere;

  found = 1;
  isThere = 1;
  for(i=0;i<GmuInstCnt;i++) {
    for(k=0;k<gmuInstance[i].numInst;k++) {
      found=1;
      for(j=0;j<j_o;j++) {
        if(strcmp(gmuInstance[i].output[k],outputs[j])==0) {
	  found=0;
	  break;
          }
        }
      if(found) {
	isThere=1;
	for(l=0;l<j_i;l++) {
	  if(strcmp(gmuInstance[i].output[k],input[l])==0) {
    	    isThere=0;
	    break;
            }
	  }
	if(isThere) {
	  input[j_i++]=gmuInstance[i].output[k];
          }
        }
      }
    }
  }

