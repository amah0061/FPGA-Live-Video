/* This file generates the controller for SPI bus, which allows data to be sent and recieved through
 * the corresponding hardware and peripherals. The file also contains the calculations to generate
 * the frames per second (FPS) of the system.
 */
/* Created By :
* Aadi Mahajan 33855994
* James Thomson 33856257
* Lance Miranda 31481795
* Ryan Shanta 32284470
* Xavier Hasiotis-Welsh 33880271
*/
// Created Date: 29/03/2025
// version ='1.0'


#include "../m1_nios_bsp/system.h"
#include "../m1_nios_bsp/HAL/inc/io.h"
#include "../m1_nios_bsp/HAL/inc/alt_types.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi_regs.h"

int main(void) {

	// Defining variables
	int col = 300;
	int row = 240;
	int frameSize = row * col;
	alt_u8 camMode = 0x0;	// this is the command for the camera mode, where 0x0 is grayscale
	alt_u8 *camBuffer = (alt_u8 *)malloc(frameSize * sizeof(alt_u8));
	int deviceSelect = 0;
	int bufferSize = 1;
	int flag = 0;
	int camReady;
	int address;
	int startTime;
	int endTime;
	int frameTime;
	float timeConversion = 1000000.0;
	float frameRate;
	int rateInt;
	int d0, d1, d2, d3;
	alt_u8 hex3, hex2, hex1, hex0;
	alt_u32 hexHigh, hexLow;
	alt_u8 pixel;


	// HEX display conversion table
	alt_u8 hexDigits[10] = {
	    0xC0, // 0
	    0xF9, // 1
	    0xA4, // 2
	    0xB0, // 3
	    0x99, // 4
	    0x92, // 5
	    0x82, // 6
	    0xF8, // 7
	    0x80, // 8
	    0x90  // 9
	};



	while(1){
		// startTime reads the current microsecond count in order to track the FPS
		startTime = IORD(USEC_COUNTER_BASE, 0);
		// Receiving the frame
		alt_avalon_spi_command(
			SPI_0_BASE, // SPI base
			deviceSelect, 			// sub device (slave select)
			bufferSize, 			// Transmit buffer size (1 bit command)
			&camMode,	// Setting data capture mode
			frameSize,  // Size of receive buffer
			camBuffer,  // Saving camera data to destination buffer
			flag		// flags: told to set to 0
		);

		// Reset camera to be in a state to not except data
		camReady = 0;
		// Writing frame
		while (camReady == 0){
			for (int i = 0; i < row; i++){
				for (int j = 0; j < col; j++){
					// Calculate address
					address = j + i * row;
					// Shift data to the right by 4 bits
					pixel = camBuffer[address]>>4;
					// Write address and data to pixel buffer
					IOWR(ADDRESS_BASE, 0, address);
					IOWR(DATA_BASE, 0, pixel);
				}
			}
			// endTime reads the current microsecond count which ends the measurement for the FPS
			endTime = IORD(USEC_COUNTER_BASE,0);
			// frameTime is the time taken to send and recieve data
			frameTime = endTime - startTime;
			// convert frameTime to seconds in order to retrieve frameRate
			frameRate = timeConversion / frameTime;
			// Multiply by 100 and truncate to integer in order to have 4 digits above the decimal place
			rateInt = (int)(frameRate * 100);

			// Extract decimal digits
			d0 = (rateInt / 1000) % 10;  // Tens
			d1 = (rateInt / 100) % 10;   // Ones
			d2 = (rateInt / 10) % 10;    // Tenths
			d3 = rateInt % 10;           // Hundredths

			// Convert to HEX display encoding
			hex3 = hexDigits[d0];
			hex2 = hexDigits[d1] & 0x7F; // Decimal point on HEX[2]
			hex1 = hexDigits[d2];
			hex0 = hexDigits[d3];

			// Pack into 24-bit words
			hexHigh  = hex3;                    // HEX[3] only
			hexLow = (hex2 << 16) | (hex1 << 8) | hex0; // HEX[2:0]

			// Write to PIOs
			IOWR(HEX_3_BASE, 0, hexHigh);
			IOWR(HEX_0_BASE, 0, hexLow);

			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}

