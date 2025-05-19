/* This file gets data and sends it to a pixel buffer for display
 */
/*
* Created By :
* Aadi Mahajan 33855994
* James Thomson 33856257
* Lance Miranda 31481795
* Ryan Shanta 32284470
* Xavier Hasiotis-Welsh 33880271
*/
// Last edited: 07/05/2025
// version ='3.0'
#include "sys/alt_stdio.h"
#include "system.h"
#include "io.h"
#include "alt_types.h"
#include "altera_avalon_pio_regs.h"
#include "stdlib.h"
#include "altera_avalon_mutex.h"

// define shared buffer variables:
int *keyFlagDisplay = (int*)0x03517710;
alt_u8 *processedSingleFrameS = (alt_u8*)0x03517714;
alt_u8 *processedTopLeft = (alt_u8*)0x0352A314;
alt_u8 *processedTopRight = (alt_u8*)0x0353CF14;
alt_u8 *processedBottomLeft = (alt_u8*)0x0354FB14;
alt_u8 *processedBottomRight = (alt_u8*)0x03562714;

// Define volatile ints
volatile int commFlag = 0;

// communication interrupt handler
void comm_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_DISPLAY_IN_BASE,0);
	IOWR(P_DISPLAY_IN_BASE, 3, 0);
	commFlag = 1;
}

int main(void){

	// Defining variables
	int singleCol = 320;
	int singleRow = 240;
	int col = 160;
	int row = 120;
	int totalCol = 320;
	int quadFrameSize = row * col;
	int singleFrameSize = singleCol*singleRow;
	alt_u8 *singleImage = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	int address;
	int pixelAddress;
	int topLeftAddress;
	int topRightAddress;
	int bottomLeftAddress;
	int bottomRightAddress;
	int startTime;
	int endTime;
	int frameTime;
	float timeConversion = 1000000.00;
	float frameRate;
	int rateInt;
	int d0, d1, d2, d3;
	alt_u8 hex3, hex2, hex1, hex0;
	alt_u32 hexHigh, hexLow;
	alt_u8 pixelTopLeft;
	alt_u8 pixelTopRight;
	alt_u8 pixelBottomLeft;
	alt_u8 pixelBottomRight;
	alt_u8 pixel;
	int keyFlag;
	int idx;

	// mutex
	alt_mutex_dev *mutex = altera_avalon_mutex_open("/dev/mutex_0");

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

	// Communication interrupt setup
	IOWR(P_DISPLAY_IN_BASE, 3, 0); // Clear edge
	IOWR(P_DISPLAY_IN_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_DISPLAY_IN_IRQ_INTERRUPT_CONTROLLER_ID, P_DISPLAY_IN_IRQ, comm_isr, NULL, 0x0);

	while (1){

		// startTime reads the current microsecond count in order to track the FPS
		startTime = IORD(USEC_COUNTER_BASE, 0);

		// Wait for processor to send image
		while (commFlag == 0){

		}
		commFlag = 0;

		// Aquire mutex
		altera_avalon_mutex_lock(mutex, 1);
		// Read the flags
		keyFlag = IORD(keyFlagDisplay,0);
		// Read images
		if (keyFlag == 0){
			for (int i = 0; i < singleRow; i++) {
				for (int j = 0; j < singleCol; j++) {
					idx = j+i*singleCol;
					singleImage[idx] = IORD(processedSingleFrameS, idx);
				}
			}
		} else if (keyFlag == 1){
			for (int i = 0; i < row; i++) {
				for (int j = 0; j < col; j++) {
					idx = j+i*col;
					quadImageTopLeft[idx] = IORD(processedTopLeft, idx);
					quadImageTopRight[idx] = IORD(processedTopRight, idx);
					quadImageBottomLeft[idx] = IORD(processedBottomLeft, idx);
					quadImageBottomRight[idx] = IORD(processedBottomRight, idx);
				}
			}
		}
		// Unluck mutex
		altera_avalon_mutex_unlock(mutex);

		// Tell spi processor to start preparing the next image
		IOWR(P_DISPLAY_OUT_SPI_BASE,0,1);
		IOWR(P_DISPLAY_OUT_SPI_BASE,0,0);

		// Tell processing processor to start preparing the next image
		IOWR(P_DISPLAY_OUT_BASE,0,1);
		IOWR(P_DISPLAY_OUT_BASE,0,0);

		if (keyFlag == 0) {
			// writing Single image to pixel buffer
			for (int i = 0; i < singleRow; i++){
				for (int j = 0; j < singleCol; j++){
					// Calculate address
					address = j + i * totalCol;
					pixel = singleImage[address];
					// Write address and data to pixel buffer
					IOWR(ADDRESS_BASE, 0, address);
					IOWR(DATA_BASE, 0, pixel);
				}
			}
		} else if (keyFlag == 1) {

			// Now write pixels for all quadrants
			for (int i = 0; i < row; i++) {
				for (int j = 0; j < col; j++) {
					pixelAddress = j + i * col;
					topLeftAddress = j + i * totalCol;
					topRightAddress = (j + 160) + i * totalCol;
					bottomLeftAddress = j + (i + 120) * totalCol;
					bottomRightAddress = (j + 160) + (i + 120) * totalCol;

					pixelTopLeft = quadImageTopLeft[pixelAddress];
					pixelTopRight = quadImageTopRight[pixelAddress];
					pixelBottomLeft = quadImageBottomLeft[pixelAddress];
					pixelBottomRight = quadImageBottomRight[pixelAddress];

					IOWR(ADDRESS_BASE, 0, topLeftAddress);
					IOWR(DATA_BASE, 0, pixelTopLeft);

					IOWR(ADDRESS_BASE, 0, topRightAddress);
					IOWR(DATA_BASE, 0, pixelTopRight);

					IOWR(ADDRESS_BASE, 0, bottomLeftAddress);
					IOWR(DATA_BASE, 0, pixelBottomLeft);

					IOWR(ADDRESS_BASE, 0, bottomRightAddress);
					IOWR(DATA_BASE, 0, pixelBottomRight);
				}
			}
		}

		// benchmarking stuff

		// endTime reads the current microsecond count which ends the measurement for the FPS
		endTime = IORD(USEC_COUNTER_BASE,0);
		// frameTime is the time taken to send and receive data
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

	}
}
