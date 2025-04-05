#include "../m1_nios_bsp/system.h"
#include "io.h"
#include <altera_avalon_spi.h>
#include <altera_avalon_spi_regs.h>

//**********************
// TO DO
//
// Take data from ESP-CAM (from SPI Bus)
// place into buffer within nios processor
// after filling buffer, copy pixel info into hardware pixel buffer (this will display to the VGA)
//**********************

//**********************
// DETAILS
//
// SPI_SDI - sends data INTO ESP-CAM: this should be 0x0 (default settings)
// SPI_SDO - sends one frame of data back (needs to be shifted right by 4 bits)
//**********************



/*
alt_avalon_spi_command(
		// SPI CONTROLLER BASE ADDRESS	(from system.h)
		// SPI CS NUMBER				(should be 1 as 1 is set to ESP-CAM)
		// SIZE of send buffer			(should be 1 byte)
		// pointer to send buffer		(no clue)
		// size of receive buffer		(whatever 1 frame is)
		// pointer to receive buffer	(no clue)
		// flags						(left as 0)
)
*/
