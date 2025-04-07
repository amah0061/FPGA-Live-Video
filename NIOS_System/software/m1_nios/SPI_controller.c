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
	int frameTime;
	float frameRate;
	int rateInt;
	int d0, d1, d2, d3;
	alt_u8 hex3, hex2, hex1, hex0;
	alt_u32 hex_high, hex_low;

	alt_u8 pixel;


	//
	alt_u8 hex_digits[10] = {
	    0x3F, // 0
	    0x06, // 1
	    0x5B, // 2
	    0x4F, // 3
	    0x66, // 4
	    0x6D, // 5
	    0x7D, // 6
	    0x07, // 7
	    0x7F, // 8
	    0x6F  // 9
	};

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

			frameTime = endTime - startTime;

			frameRate = 1000000.0 / frameTime;

			// Multiply by 100 and truncate to int
			rateInt = (int)(frameRate * 100);

			// Extract decimal digits
			d0 = (rateInt / 1000) % 10;  // Tens
			d1 = (rateInt / 100) % 10;   // Ones
			d2 = (rateInt / 10) % 10;    // Tenths
			d3 = rateInt % 10;           // Hundredths

			// Convert to HEX display encoding
			hex3 = hex_digits[d0];
			hex2 = hex_digits[d1] | 0x80; // Decimal point on HEX[2]
			hex1 = hex_digits[d2];
			hex0 = hex_digits[d3];

			// Pack into 24-bit words
			hex_high  = hex3;                    // HEX[3] only
			hex_low = (hex2 << 16) | (hex1 << 8) | hex0; // HEX[2:0]

			// Write to PIOs
			IOWR(HEX_3_BASE, 0, hex_high);
			IOWR(HEX_0_BASE, 0, hex_low);


			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}
