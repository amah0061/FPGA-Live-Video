#include "system.h"
#include "io.h"
int main(void)
{
int address;
int col = 320;
int row = 240;
int colour;
while(1)
{
for (int i = 0; i < row; i++){
	for (int j = 0; j < col; j++){
		address = j + i*row;
		colour = ((j / 20 + i / 20) % 2) ? 0xF : 0x0;
		IOWR(ADDRESS_BASE,0,address);
		IOWR(DATA_BASE,0,colour);
	}
}


}


}
