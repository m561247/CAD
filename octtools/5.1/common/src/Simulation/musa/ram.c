/***********************************************************************
 * RAM Module for Musa
 * Filename: ram.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 *	Andrea Casotto				Created				10/22/88
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define RAM_MUSA
#include "musa.h"

/* data inside of latch instance */
#define READ_ACTIVE_LOW	   0x01	/* latch is Active Low (master is AL) */
#define WRITE_ACTIVE_LOW   0x02	/* s AL) */

#define READ_TERM  0
#define WRITE_TERM 1

typedef struct {
	int size;
	int width;
	int addressLines;
	int readActiveHigh;
	int writeActiveHigh;
	int *data;
	short *flags;
} RAM;

/**************************************************************************
 * INITRAM
 */
RAM *initRAM(adrwidth,datawidth, rah,wah)
	int adrwidth;		/* address bits */
	int datawidth;		/* data bits */
	int rah;			/* read active high flag */
	int wah;			/* write active high flag */
{
	int i;
	RAM *ram;

	if (datawidth > 32) {
		FatalError("Cannot handle word width greater that 32");
	} /* if... */
	if (adrwidth < 1) {
		FatalError("No address lines in this RAM");
	} /* if... */
    
	ram = ALLOC(RAM,1);
	ram->width = datawidth;
	ram->addressLines = adrwidth;
	ram->size = 1 << adrwidth;
	ram->readActiveHigh = rah;
	ram->writeActiveHigh = wah;
	ram->data = ALLOC(int , ram->size);
	ram->flags = ALLOC(short , ram->size);

	for (i=0;i<ram->size;i++) {
		ram->data[i] = 0;
		ram->flags[i] = 0;
	} /* for... */
	return(ram);
} /* initRAM... */

/**************************************************************************
 * RAMSTORE
 */
RAMstore(ram,adr,data)
	RAM *ram;
	int adr;
	int data;
{
	int mask = 0xffffffff >> (32 - ram->width);

	if (ram->width == 32) mask = 0xffffffff;

	if (adr >= ram->size) {
		(void) sprintf(errmsg,"Illegal address %d for RAM having size %d", adr,ram->size);
		Warning(errmsg);
	} /* if... */
    
	ram->data[adr] = data & mask;
	ram->flags[adr] = 1;	/* The location has been initialized */
} /* RAMstore... */

/************************************************************************
 * RAMREAD
 */
RAMread(ram,adr)
	RAM *ram;
	int adr;
{
	int data;

	if (adr >= ram->size) {
		(void) sprintf(errmsg,"Illegal address %d for RAM having size %d", adr,ram->size);
		Warning(errmsg);
	} /* if... */
	if (ram->flags[adr] == 0) {
		(void) sprintf(errmsg,"Reading RAM location %d which has not been initialized",adr);
		Warning(errmsg);
	} /* if... */
	data = ram->data[adr];
	return(data);
} /* RAMread... */


/****************************************************************************
 * RAM_READ
 * try to see if we can read a latch from Oct
 * Return 0 if this is not a MEMORY element
 */
int16 ram_read(ifacet, inst, celltype)
	octObject *ifacet, *inst;
	char *celltype;
{
	musaInstance *newmusaInstance;
	octObject read, write;
	octObject address, data;
	char addressBaseName[50];
	char dataBaseName[50];
	octObject term, prop;
	octObject ActiveLevel;	/* active high signal or active low signal  */
	octGenerator termgen;
	char name[128];
	int i;
	int addressBits = 0;
	int dataBits = 0;
	int readActiveLevel = 1;
	int writeActiveLevel = 1;

	(void) ignore(celltype);

	if (ohGetByPropName(ifacet,&prop,"MEMORYMODEL") != OCT_OK) {
		return 0;
	} /* if... */

	OH_ASSERT(octInitGenContents(ifacet, OCT_TERM_MASK, &termgen));
	while (octGenerate(&termgen, &term) == OCT_OK) {
		if (ohGetByPropName(&term, &prop, "RAM_TERM") == OCT_OK) {
			if (strcmp(prop.contents.prop.value.string, "READ_ENABLE") == 0) {
				OH_ASSERT(ohGetByTermName(inst, &read, term.contents.term.name));
				if (ohGetByPropName(&term,&ActiveLevel,"ACTIVE_LEVEL") == OCT_OK) {
					if (strcmp(ActiveLevel.contents.prop.value.string,"LOW")==0) {
						readActiveLevel = 0;
					} /* if... */
				} /* if... */
			} else if (strcmp(prop.contents.prop.value.string, "WRITE_ENABLE") == 0) {
				OH_ASSERT(ohGetByTermName(inst, &write, term.contents.term.name));
				if (ohGetByPropName(&term,&ActiveLevel,"ACTIVE_LEVEL") == OCT_OK) {
					if (strcmp(ActiveLevel.contents.prop.value.string,"LOW")==0) {
						writeActiveLevel = 0;
					} /* if... */
				} /* if... */
			} else if (strcmp(prop.contents.prop.value.string, "ADDRESS") == 0) {
				if (!addressBits) {
					(void) strcpy(addressBaseName,term.contents.term.name);
					{
						char *c;

						c = strchr(addressBaseName,'<');
						if (c != (char *) 0) *c = '\0';
					}
				} /* if... */
				addressBits++;
			} else if ((strcmp(prop.contents.prop.value.string, "DATA") == 0)) {
				if (!dataBits) {
					(void) strcpy(dataBaseName,term.contents.term.name);
					{
						char *c;

						c = strchr(dataBaseName,'<');
						if (c != (char *) 0) *c = '\0';
					}
				} /* if... */
				dataBits++;
			} else {
				(void) sprintf(errmsg,"\n\tUnknown value (%s) for RAM_TERM\n\tterm %s of %s:%s",
					prop.contents.prop.value.string,
					term.contents.term.name,
					ifacet->contents.facet.cell,
					ifacet->contents.facet.view);
				Warning(errmsg);
			} /* if... */
		} else {
		} /* if... */
    } /* while... */

	make_leaf_inst(&newmusaInstance, DEVTYPE_RAM, inst->contents.instance.name, 2+dataBits+addressBits);
	connect_term(newmusaInstance, &read, 0);
	connect_term(newmusaInstance, &write, 1);
	for (i=0; i < addressBits; i++) {
		(void) sprintf(name,"%s<%d>",addressBaseName,i);
		if (ohGetByTermName(inst,&address,name)!=OCT_OK) {
			FatalError("missing address terminal");
		} /* if... */
		connect_term(newmusaInstance, &address, 2+i);
	} /* for... */
	for (i=0; i < dataBits; i++) {
		(void) sprintf(name,"%s<%d>",dataBaseName,i);
		if (ohGetByTermName(inst,&data,name)!=OCT_OK) {
			FatalError("missing data terminal");
		} /* if... */
		connect_term(newmusaInstance, &data, 2 + addressBits + i);
	} /* for... */

	newmusaInstance->userData = (char *) initRAM(addressBits,dataBits, readActiveLevel,writeActiveLevel);
	return 1;
} /* ram_read... */

/***********************************************************************
 * RAM_INFO
 */
ram_info(inst)
	musaInstance *inst;
{
    int terms;			/* terminals in the RAM instance */
    RAM *ram;
    char buf[256];

    if (inst->type == DEVTYPE_RAM) {
	ram = (RAM *) inst->userData;
	(void)sprintf(buf,"  -- Size=%d Width=%d Read/Write Active Level %d/%d\n", ram->size, ram->width,
		     ram->readActiveHigh, ram->writeActiveHigh );
	MUSAPRINT( buf );
	{
	    int i;
	    int last = 0;
	    for ( i = 0; i < ram->size; i++ ) {
		if ( ram->flags[i] && last != ram->data[i] ) {
		    sprintf( buf, "%6d -- %6d\n", i, ram->data[i] );
		    MUSAPRINT( buf );
		    last = ram->data[i];
		}
	    }
	}
	terms = ram->addressLines + ram->width + 2;
	ConnectInfo(inst, terms);
    } 
} 

/***********************************************************************
 * SIM_RAM
 */
sim_ram(inst)
    musaInstance *inst;
{
    int adr = 0;
    int data = 0;		/*  */
    int readVoltage;
    int writeVoltage;
    int readAction = 0;
    int writeAction = 0;
    int i;
    int voltage;
    int index;
    RAM *ram;

    if (inst->type == DEVTYPE_RAM) {
	ram = (RAM *) inst->userData;
	readVoltage = inst->connArray[READ_TERM].node->voltage & L_VOLTAGEMASK;
	writeVoltage = inst->connArray[WRITE_TERM].node->voltage & L_VOLTAGEMASK;
	
	if (ram->readActiveHigh) {
	    if (readVoltage == L_HIGH) readAction = 1;
	} else {
	    if (readVoltage == L_LOW) readAction = 1;
	}	

	if (ram->writeActiveHigh) {
	    if (writeVoltage == L_HIGH) writeAction = 1;
	} else {
	    if (writeVoltage == L_LOW) writeAction = 1;
	}	

	if (readAction && writeAction) {
	    Warning("Both READ and WRITE lines are active in RAM");
	}	
	
	for (i=0; i < ram->addressLines; i++) {
	    index = 2 + i;	/* index for the address line */
	    voltage = inst->connArray[index].node->voltage & L_VOLTAGEMASK;
	    if (voltage == L_UNKNOWN) {
		Warning("Undefined voltage on RAM address line");
	    } else if (voltage == L_HIGH) {
		adr |= (1 << i);
	    }	
	}	

	if (readAction) {
	    data = RAMread(ram,adr);
	    index = 2 + ram->addressLines;
	    for (i=0; i < ram->width; i++) {
		if ((data >> i) & 0x1) {
		    voltage = L_HIGH; /* logic 1 */
		} else {
		    voltage = L_LOW; /* logic 0 */
		}	
		drive_output(&(inst->connArray[index]), voltage);
		index++;
	    }		
	} else {
	    /* If not reading, make sure the data lines are not driven */
	    index = 2 + ram->addressLines;
	    for (i=0; i < ram->width; i++) {
		if (inst->connArray[index].lout || inst->connArray[index].hout) {
		    undrive_output(&(inst->connArray[index]));
		}
		index++;
	    }		
	}		
	if (writeAction) {
	    index = 2 + ram->addressLines;
	    for (i=0; i < ram->width; i++) {
		voltage = inst->connArray[index].node->voltage & L_VOLTAGEMASK;
		if (voltage == L_UNKNOWN) {
		    Warning("Undefined voltage on RAM data line");
		} else if (voltage == L_HIGH) {
		    data |= (1 << i);
		}	
		index++;
	    }		
	    RAMstore(ram,adr,data);
	}		
    }			
}				/* sim_ram... */


