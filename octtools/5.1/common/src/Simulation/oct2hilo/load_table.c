#include "copyright.h"
#include "port.h"
#include "oct.h"
#include "st.h"
#include "oh.h"
#include "head.h"

int delcnt;
int argcnt;
int inst;
FILE *fp;

extern char *techFile;

/*  
 *  load_table() loads data from the selected tech file into the
 *      primitives structure.  lines beginning with # are considered
 *      to be comments.  blank lines are allowed.
 */


load_table() {
  int i,j,k;
  int arg;
  char c;
  char d;
  char buf[1024],*ptr1,*ptr2;
  FILE *fp;
  int lineno,num_del,num_arg;

  inst=0;

  if((fp = fopen(techFile,"r"))== NULL) {
    fprintf(stderr,"%s: can't be opened\n",techFile);
    exit(-1);  
    }
  else {
    lineno = 0;
    while(fgets(buf,1024,fp)!=NULL) {
      lineno++;
      /* commented lines begin with a # */
      if((*buf=='#')||(*buf=='\n'))
        continue;
      /* blank lines are ok too */
      if((ptr1=strtok(buf," \t"))==NULL)
        continue;
      /* if this line is bad, note it and skip it */
      if((ptr2=strtok(NULL," \t"))==NULL){
        bad_line(techFile,lineno);
        continue;
        }
      /* save the names */
      strcpy(primitives[inst].number_name,ptr1);
      strcpy(primitives[inst].primitive_name,ptr2);
      num_del = num_arg = 0;
      /* now read the timing parameters*/
      while((ptr1=strtok(NULL," \t"))!=NULL) {
        if(*ptr1==';') {
          primitives[inst].delCnt = num_del;
          break;
          }
        strcpy(primitives[inst].delay[num_del++],ptr1);
        }
      /* now read the terminal names */
      while((ptr1=strtok(NULL," \t"))!=NULL) {
        if(*ptr1=='*') {
          primitives[inst].argCnt = num_arg;
          break;
          }
        strcpy(primitives[inst].arg[num_arg++],ptr1);
        }
      inst++;
      }
    }
  }


/*
 *  bad_line() lets the user know there is a bad line in a tech file.
 */

bad_line(techFile,lineno) 
  char *techFile;
  int lineno; {

  fprintf(stderr,"tech file %s has malformed line %d\n",techFile,lineno);
  fflush(stderr);
  }
	

      
/* 
 *  dump_table() dumps the contents of the primitives structure.
 *     it is for debugging only.
 */

dump_table() {
  int i,j,k;

  for(i=0;i<inst;i++) {
    printf("%s %s",primitives[i].number_name,primitives[i].primitive_name);
    for(j=0;j<primitives[i].delCnt;j++)
      printf(" %s",primitives[i].delay[j]);
    for(j=0;j<=primitives[i].argCnt;j++)
      printf(" %s",primitives[i].arg[j]);
    printf("\n");
    }
  }

