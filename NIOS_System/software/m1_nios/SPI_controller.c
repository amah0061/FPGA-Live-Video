
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
#include "io.h"
#include <altera_avalon_spi.h>
#include <altera_avalon_spi_regs.h>

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT)
#define GREYSCALE 0x0

int main(void)
{
	// TO RESEARCH: Apparently int is 4 bytes, but char is 1 byte so it might be better to use char??
	// Defining variables
	int *frame_buffer = (int *)NEW_SDRAM_CONTROLLER_0_BASE;
	int cam_mode = GREYSCALE;	//command for greyscale
	static int cam_buffer[76800];

	int address;
	int pixel;

	// Calling SPI Protocol (send cam mode and gets data stored in array "cam_buffer")
	// NOTE: BASED ASSUMPTION IS CAMERA SENDS ONE WHOLE FRAME (think it was mentioned in the doc somewhere)
	alt_avalon_spi_command(
			SPI_0_BASE,0, 			// SPI base and which sub device
			1, &cam_mode,			// Setting data capture mode
			FRAME_SIZE, cam_buffer,		// Saving camera data
			0);						// flags: told to set to 0

	// Sending Cam data to SDRAM
	for (int i = 0; i < FRAME_SIZE; i++) {
	        frame_buffer[i] = cam_buffer[i] >> 4;  // Shift down 4 bits
	    }

	// Copying cam data to pixel buffer
	while(1){
		// Writing each pixel
		for (int i = 0; i < FRAME_HEIGHT; i++){
			for (int j = 0; j < FRAME_WIDTH; j++){
				address = j + i*FRAME_HEIGHT;
				pixel = frame_buffer[address];
				IOWR(ADDRESS_BASE,0,address);
				IOWR(DATA_BASE,0,pixel);
			}
		}
	}
}


