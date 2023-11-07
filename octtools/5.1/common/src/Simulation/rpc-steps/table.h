

typedef struct tableEntry {
    char* propName;
    octPropType type;
    char* defaultValue;
    char* desc;
    int   width;		/* Width of dialog field. */
    char* spiceFormat;		/* How this should appear in Spice file. */
    struct tableEntry *table;	/* Secondary table. */
} tableEntry;

extern void fillTableEntry();
extern void delTable();



#define R OCT_REAL
#define I OCT_INTEGER
#define S OCT_STRING


extern tableEntry mosfetTable[];
extern tableEntry jfetTable[];
extern tableEntry bjtTable[];
extern tableEntry diodeTable[];
extern tableEntry resistorTable[];
extern tableEntry capacitorTable[];
extern tableEntry inductorTable[];
extern tableEntry transformerTable[];
extern tableEntry sourceTable[];
extern tableEntry dcSourceTable[];
extern tableEntry depSourceTable[];
extern tableEntry outputTable[];


extern tableEntry modelTable[];
extern tableEntry optionsTable[];
extern tableEntry analysisTable[];
extern tableEntry printTable[];
extern tableEntry stepsTable[];
extern tableEntry spiceDefaultsTable[];
extern tableEntry connectivityTable[];



