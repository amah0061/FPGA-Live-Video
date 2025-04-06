
//**********************
// TO DO (HIGH LEVEL)
//
// Emulating the bottom NIOS II Processor
// SPI Sends 0x0 to ESP-CAM, takes ESP-CAM data and writes it into local memory (internal buffer)
// NIOS II then moves frame data from local memory to SDRAM
//
// Emulating the top NIOS II Processor
// NIOS II takes out Frame Data
// Based on the type of collection mode, NIOS II transforms Frame Data into Pixel Data
// NIOS II shoves Pixel Data INTO Pixel buffer (and then VGA Controller)
//**********************

#include "../m1_nios_bsp/system.h"
#include "../m1_nios_bsp/HAL/inc/io.h"
#include "../m1_nios_bsp/HAL/inc/alt_types.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi_regs.h"

int main(void)
{

	// Defining variables
	int col = 320;
	int row = 240;
	int frameSize = row*col;
	// TO RESEARCH: Apparently int is 4 bytes, but char is 1 byte so it might be better to use char??
	alt_u8 *frame_buffer = (alt_u8 *)NEW_SDRAM_CONTROLLER_0_BASE;
	alt_u8 camMode = 0x0;	//command for grayscale
	alt_u8 camBuffer[76800];
	int camReady;
	int address;
	alt_u8 pixel;

	while(1){
		// Receiving the frame
		alt_avalon_spi_command(
			SPI_0_BASE,  // SPI base
			0, 			// sub device
			1, 			// Size of buffer in bytes
			&camMode,	// Setting data capture mode
			frameSize,  // size of receive buffer
			camBuffer,// Saving camera data
			0			// flags: told to set to 0
		);

		// Sending Cam data to SDRAM
		for (int i = 0; i < frameSize; i++) {
			frame_buffer[i] = camBuffer[i] >> 4;  // Shift down 4 bits
		}
		// Writing each pixel
		camReady = 0;
		while (camReady == 0){
			for (int i = 0; i < row; i++){
				for (int j = 0; j < col; j++){
					address = j + i*row;
					pixel = frame_buffer[address];
					pixel &= 0x0f;
					IOWR(ADDRESS_BASE,0,address);
					IOWR(DATA_BASE,0,pixel);
				}
			}
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}


