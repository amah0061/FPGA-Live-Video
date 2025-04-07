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
	alt_u8 camMode = 0x0;	//command for grayscale
	alt_u8 camBuffer[frameSize]; // Array to store frame data
	int camReady;
	int address;
	int startTime;
	int endTime;
	int frameRate;
	alt_u8 pixel;

	while(1){
		startTime = IORD(USEC_COUNTER_BASE,0);
		// Receiving the frame
		alt_avalon_spi_command(
			SPI_0_BASE,  // SPI base
			0, 			// sub device (slave select)
			1, 			// Transmit buffer size (1 bit command)
			&camMode,	// Setting data capture mode
			frameSize,  // Size of receive buffer
			camBuffer,  // Saving camera data to destination buffer
			0			// flags: told to set to 0
		);

		// Reset camera to not being ready
		camReady = 0;

		// Writing frame
		while (camReady == 0){
			for (int i = 0; i < row; i++){
				for (int j = 0; j < col; j++){
					// Calculate address
					address = j + i*row;
					// Shift data to the right by 4 bits
					pixel = camBuffer[address]>>4;
					// Write address and data to pixel buffer
					IOWR(ADDRESS_BASE,0,address);
					IOWR(DATA_BASE,0,pixel);
				}
			}
			endTime = IORD(USEC_COUNTER_BASE,0);

			if (endTime < startTime)
				frameRate = endTime - startTime + 2^32;
			else
				frameRate = endTime - startTime;

			frameRate = frameRate * 0.02; // frame rate in micro seconds now

			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}
