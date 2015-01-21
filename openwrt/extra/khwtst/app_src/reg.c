#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#define DRVNAME "/dev/hwtst"

// --- MyDrv API ----
typedef enum {
	NONE,
	WRITE_VALUE
} mydrv_op_t;

typedef struct {
	unsigned long base_addr;
	mydrv_op_t    operation;
	unsigned long value;
} mydrv_write_t;

typedef struct {
	unsigned long value;
} mydrv_read_t;



int main(int argc, char** argv)
{
	FILE* f = NULL;
	mydrv_write_t w = {0};
	mydrv_read_t  r = {0};
	
	if (argc==1) {
		printf("read:  reg 0xaddr\n");
		printf("write: reg 0xaddr 0xvalue\n");
		return 0;
	}
		
	/* Do operation ! */
	f = fopen(DRVNAME,"wb");
	if(f==NULL) { printf("No driver"); return 1; }	
	w.base_addr = strtoul(argv[1], NULL, 16);
	w.operation = (argc==3)?WRITE_VALUE:NONE;
	w.value     = (argc==3)?strtoul(argv[2], NULL, 16):0x00;		
	fwrite(&w , sizeof(mydrv_write_t), 1, f);
	fclose(f);
	
	/* And then read back */
	f = fopen(DRVNAME,"rb");
	if(f==NULL) { printf("No driver"); return 1; }
	fread(&r , sizeof(mydrv_read_t), 1, f);
	printf("0x%08X\n",r.value);
	fclose(f);
	
	return 0;
}
