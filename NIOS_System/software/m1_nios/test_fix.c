//#include "system.h"
//#include "io.h"
//#include "altera_avalon_spi.h"
//#include "altera_avalon_spi_regs.h"
//
//
//
//int main(void)
//{
//// Initialise variables
//
//int address;
//int col = 320;
//int row = 240;
//int colour;
//
//while(1)
//{
//// Writing each pixel
//
//for (int i = 0; i < row; i++){
//	for (int j = 0; j < col; j++){
//		address = j + i*row;
//		colour = ((j / 20 + i / 20) % 2) ? 0xF : 0x0;
//		IOWR(ADDRESS_BASE,0,address);
//		IOWR(DATA_BASE,0,colour);
//	}
//}
//
////	alt_avalon_spi_command(
////	    SPI_0_BASE,               // SPI base
////	    0,                        // CS number
////	    1,                        // Size of send buffer
////	    &tx_data,                 // Pointer to byte to be sent
////	    1,                        // Read a byte
////	    &rx_data,                 // Pointer to store the byte
////	    0
////	);
//
//
//}
//
//
//}
