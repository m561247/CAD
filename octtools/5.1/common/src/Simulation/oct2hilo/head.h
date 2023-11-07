#define MAX_TERM 20
#define MAX_GATE 1000
#define MAX_INST 200
#define MAX_PRIMITIVES 200
#define MAX_DELAY 30
#define MAX_ARG 20
#define MAX_OUT 500
#define MAX_GMU 1000
#define BLOCK 256
#define INDENT 5
#define CIRCNAMESIZE 50
#define DELSIZE 10
#define ARGSIZE 10

#define TERM_IN  0
#define TERM_OUT 1
#define TERM_INOUT 2
#define TERM_SUPPLY0 3
#define TERM_SUPPLY1 4
#define TERM_UNK -1

#define PROG_HILO 0
#define PROG_VHDL 1
#define PROG_UNK -1

#define DELAY_UNIT 1
#define DELAY_TYP 2
#define DELAY_MAX 3

extern int inst_cnt;
extern int prog;
extern FILE *fp;
extern int j_i,j_o,j_s1,j_s0;
extern char *circType;


/* all these structures need to be reworked to use pointers */
struct gate_tbl {
	char *g_name;
	char *term[MAX_TERM];
	int next;
	};

struct gate_tbl my_gate[MAX_GATE];

struct inst_tbl {
	char *inst_numb;
	char *term_name[MAX_TERM];
	int term_dir[MAX_TERM];
	int term_cnt;
	int prim_num;
        int gate;
	};

 struct inst_tbl my_inst[MAX_INST];


/* the following structure is illconceived, but will have
   to do for now..  -- jds */
struct primitives_tbl {
  char number_name[30];
  char primitive_name[50];
  int delCnt;
  char delay[MAX_DELAY][DELSIZE];
  int argCnt;
  char arg[MAX_DELAY][ARGSIZE];
  };
struct primitives_tbl primitives[MAX_PRIMITIVES];

struct gmuInstance_tbl {
  char *instName;
  char *output[MAX_OUT];
  int  numInst;
  int  ptypesize;
  int  ntypesize;
  };

struct gmuInstance_tbl gmuInstance[MAX_GMU];

typedef struct alias {
  char *netname;
  char *termname;
  }Alias;

struct INT_term {
  char     *name;
  int      cnt;
  int      type;
  struct INT_term *next;
  struct INT_term *next_all;
  };

struct sortlist {
  char     *str;
  struct sortlist *next;
  };

struct INT_terms {
  struct INT_term *all_head;
  struct INT_term *all_cur;
  struct INT_term *in_head;
  struct INT_term *in_cur;
  struct INT_term *out_head;
  struct INT_term *out_cur;
  struct INT_term *inout_head;
  struct INT_term *inout_cur;
  struct INT_term *supply0_head;
  struct INT_term *supply0_cur;
  struct INT_term *supply1_head;
  struct INT_term *supply1_cur;
  };


struct IO_term {
  char     *termname;	  /* name of terminal */
  char     *netname; 	  /* name of terminal in net if different */
  struct INT_term *termptr;      /* pointer to terminal entry */
  struct IO_term  *next; /* pointer to next IO_term of same type */
  };
