#include "nios2_ctrl_reg_macros.h"
#include "system.h"
#include "io.h"
// function prototypes int main(void);
void interrupt_handler(void); void the_exception (void);

/* Declare volatile pointers to I/O registers. This will ensure that the resulting code will bypass the cache*/



int main(void)
{
int address;
int col = 360;
int row = 240;
while(1)
{
for (int i = 0; i < row; i++){
	for (int j = 0; j < col; j++){
		address = j + i*row;
		IOWR(ADDRESS_BASE,0,address);
		IOWR(DATA_BASE,0,0x3);
	}
}


}


}
