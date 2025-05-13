/* This file alters the image received from the camera in a number of different ways depending
 * on user requirements. The file then outputs the relevant image/images to an external
 * monitor. The file also contains the calculations to generate the frames per second (FPS) of
 * the system.
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
int *doubleTapFlagS = (int*)0x02000000;
int *keyFlagS = (int*)0x02000004;
int *swFlagS = (int*)0x02000008;
int *yDataS = (int*)0x0200000C;
alt_u8 *singleFrameS = (alt_u8*)0x02000010;
alt_u8 *quadFrameS = (alt_u8*)0x02012C10;

// Define volatile ints
volatile int commFlag = 1;

// communication interrupt handler
void comm_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_PROCESSING_DISPLAY_IN_BASE,0);
	IOWR(P_PROCESSING_DISPLAY_IN_BASE, 3, 0);
	commFlag = 1;
}

// Image flipping algorithm
void flip(void *inputImage, void *outputImage, int width, int height) {
	// Defining variables
	alt_u8 *in = (alt_u8 *)inputImage;
	alt_u8 *out = (alt_u8 *)outputImage;
	for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++) {
            out[j+i*width] = in[(width-1-j)+(height-1-i)*width];
        }
    }
}

// Convolve function
void convolve(void *inputImage, void *outputImage, void *kernel, int width, int height) {
	// Defining variables
	alt_u8 *in = (alt_u8 *)inputImage;
	alt_8 *out = (alt_8 *)outputImage;
	int runningValue;
	int *k = (int *)kernel;
    int kernelSum = 0;

    // Find sum of kernel for normalisation
    for (int i = 0; i < 9; i++) kernelSum += k[i];

    // Main convolution loop
    for (int i = 1; i < height - 1; i++){
        for (int j = 1; j < width - 1; j++){
        	runningValue = 0;

        	// Update running value for each of 9 pixels, starting at top left
        	runningValue = runningValue + in[j+(i-1)*width-1]*k[0];
        	runningValue = runningValue + in[j+(i-1)*width]*k[1];
        	runningValue = runningValue + in[j+(i-1)*width+1]*k[2];
        	runningValue = runningValue + in[j+(i)*width-1]*k[3];
        	runningValue = runningValue + in[j+(i)*width]*k[4];
        	runningValue = runningValue + in[j+(i)*width+1]*k[5];
        	runningValue = runningValue + in[j+(i+1)*width-1]*k[6];
        	runningValue = runningValue + in[j+(i+1)*width]*k[7];
        	runningValue = runningValue + in[j+(i+1)*width+1]*k[8];

        	// Normalise
        	if (kernelSum != 0) {
        		runningValue /= kernelSum;
        	}

        	// Assigning value to current pixel
        	out[j+i*width] = runningValue;
        }
    }
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
	alt_u8 *singleImageDisplay = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *singleImageAlteration1 = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_8  *edgeX = (alt_8 *)malloc(singleFrameSize * sizeof(alt_8));
	alt_8  *edgeY = (alt_8 *)malloc(singleFrameSize * sizeof(alt_8));
	alt_u8 *quadImageOrigin = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration1 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration2 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration3 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration4 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	int topLeftFlag = 1;
	int topRightFlag = 1;
	int bottomLeftFlag = 1;
	int bottomRightFlag = 1;
	int selectedAlteration;
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
	int kernelBlur[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int kernelEdgeX[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
	int kernelEdgeY[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
	int thresholdValue = 18;
	int doubleTapFlag;
	int swFlag;
	int keyFlag;
	int yData;
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
	IOWR(P_PROCESSING_DISPLAY_IN_BASE, 3, 0); // Clear edge
	IOWR(P_PROCESSING_DISPLAY_IN_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_PROCESSING_DISPLAY_IN_IRQ_INTERRUPT_CONTROLLER_ID, P_PROCESSING_DISPLAY_IN_IRQ, comm_isr, NULL, 0x0);

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
		doubleTapFlag = IORD(doubleTapFlagS,0);
		keyFlag = IORD(keyFlagS,0);
		swFlag = IORD(swFlagS,0);
		yData = IORD(yDataS,0);
		// Read images
		if (keyFlag == 0){
			for (int i = 0; i < singleRow; i++) {
			    for (int j = 0; j < singleCol; j++) {
				    idx = j+i*singleCol;
				    singleImage[idx] = IORD(singleFrameS, idx);
			    }
		    }
		} else if (keyFlag == 1){
			for (int i = 0; i < row; i++) {
				for (int j = 0; j < col; j++) {
					idx = j+i*col;
					quadImageOrigin[idx] = IORD(quadFrameS, idx);
				}
			}
		}
		// Unluck mutex
		altera_avalon_mutex_unlock(mutex);

		// Tell other processor it is ready for next image
		IOWR(P_PROCESSING_DISPLAY_OUT_BASE,0,1);
		IOWR(P_PROCESSING_DISPLAY_OUT_BASE,0,0);

		if (keyFlag == 0) {
			// Shift each pixel 4 bits to the right to get rid of junk
			for (int i = 0; i < singleRow; i++){
				for (int j = 0; j < singleCol; j++){
					singleImage[j+i*singleCol] = singleImage[j+i*singleCol]>>4;
				}
			}
			// image alterations
			if (doubleTapFlag == 0) {			//Default image
				singleImageDisplay = singleImage;

			} else if (doubleTapFlag == 1) {	// Flipped image
				flip(singleImage, singleImageAlteration1, singleCol, singleRow);
				singleImageDisplay = singleImageAlteration1;

			} else if (doubleTapFlag == 2) {	//Blurred image
				convolve(singleImage, singleImageAlteration1, kernelBlur, singleCol, singleRow);
				singleImageDisplay = singleImageAlteration1;

			} else if (doubleTapFlag == 3) {	//Edge Detection image
				convolve(singleImage, edgeX, kernelEdgeX, singleCol, singleRow);
				convolve(singleImage, edgeY, kernelEdgeY, singleCol, singleRow);

				// Combine X and Y Sobel filters and checking threshold
				for (int i = 1; i < singleRow - 1; i++){
					for (int j = 1; j < singleCol - 1; j++){
						singleImageAlteration1[j+i*singleCol] = abs(edgeX[j+i*singleCol]) + abs(edgeY[j+i*singleCol]);

						if (singleImageAlteration1[j+i*singleCol] < thresholdValue){
							singleImageAlteration1[j+i*singleCol] = 0;
						}
					}
				}
				singleImageDisplay = singleImageAlteration1;
			}
			// writing Single image to pixel buffer
			for (int i = 0; i < singleRow; i++){
				for (int j = 0; j < singleCol; j++){
					// Calculate address
					address = j + i * totalCol;
					pixel = singleImageDisplay[address];
					// Write address and data to pixel buffer
					IOWR(ADDRESS_BASE, 0, address);
					IOWR(DATA_BASE, 0, pixel);
				}
			}
		} else if (keyFlag == 1) {
			// Shift each pixel 4 bits to the right to get rid of junk
			for (int i = 0; i < row; i++){
				for (int j = 0; j < col; j++){
					quadImageOrigin[j+i*col] = quadImageOrigin[j+i*col]>>4;
				}
			}

			// Select alteration based on y position
			if (yData >= -20 && yData <= 20) {
				selectedAlteration = 1;
			} else if (yData >= -90 && yData < -20) {
				selectedAlteration = 2;
			} else if (yData > 20 && yData <= 60) {
				selectedAlteration = 3;
			} else if (yData > 60 && yData <= 90) {
				selectedAlteration = 4;
			}

			// Update quadrant flag to display relevant image
			if (swFlag & (1 << 0)) { // SW0 - Top Left
				topLeftFlag = selectedAlteration;
			}
			if (swFlag & (1 << 1)) { // SW1 - Top Right
				topRightFlag = selectedAlteration;
			}
			if (swFlag & (1 << 2)) { // SW2 - Bottom Left
				bottomLeftFlag = selectedAlteration;
			}
			if (swFlag & (1 << 3)) { // SW3 - Bottom Right
				bottomRightFlag = selectedAlteration;
			}
			// Reset flag
			swFlag = 0;

			// Call original image if needed
			if (topLeftFlag == 1 || topRightFlag == 1 || bottomLeftFlag == 1 || bottomRightFlag == 1) {
				quadImageAlteration1 = quadImageOrigin;
			}

			// Call edge alteration if needed
			if (topLeftFlag == 2 || topRightFlag == 2 || bottomLeftFlag == 2 || bottomRightFlag == 2) {
				flip(quadImageOrigin, quadImageAlteration2, col, row);
			}

			// Call image blur if needed
			if (topLeftFlag == 3 || topRightFlag == 3 || bottomLeftFlag == 3 || bottomRightFlag == 3) {
				convolve(quadImageOrigin, quadImageAlteration3, kernelBlur, col, row);
			}

			// Call edge detection if needed
			if (topLeftFlag == 4 || topRightFlag == 4 || bottomLeftFlag == 4 || bottomRightFlag == 4) {
				convolve(quadImageOrigin, edgeX, kernelEdgeX, col, row);
				convolve(quadImageOrigin, edgeY, kernelEdgeY, col, row);
				// Combine X and Y Sobel filters and checking threshold
				for (int i = 1; i < row - 1; i++){
					for (int j = 1; j < col - 1; j++){
						quadImageAlteration4[j+i*col] = abs(edgeX[j+i*col]) + abs(edgeY[j+i*col]);

						if (quadImageAlteration4[j+i*col] < thresholdValue){
							quadImageAlteration4[j+i*col] = 0;
						}
					}
				}
			}

			// Store relevant image in top left quadrant
			if (topLeftFlag == 1) {
				quadImageTopLeft = quadImageAlteration1;
			} else if (topLeftFlag == 2) {
				quadImageTopLeft = quadImageAlteration2;
			} else if (topLeftFlag == 3) {
				quadImageTopLeft = quadImageAlteration3;
			} else if (topLeftFlag == 4) {
				quadImageTopLeft = quadImageAlteration4;
			}

			// Store relevant image in top right quadrant
			if (topRightFlag == 1) {
				quadImageTopRight = quadImageAlteration1;
			} else if (topRightFlag == 2) {
				quadImageTopRight = quadImageAlteration2;
			} else if (topRightFlag == 3) {
				quadImageTopRight = quadImageAlteration3;
			} else if (topRightFlag == 4) {
				quadImageTopRight = quadImageAlteration4;
			}

			// Store relevant image in bottom left quadrant
			if (bottomLeftFlag == 1) {
				quadImageBottomLeft = quadImageAlteration1;
			} else if (bottomLeftFlag == 2) {
				quadImageBottomLeft = quadImageAlteration2;
			} else if (bottomLeftFlag == 3) {
				quadImageBottomLeft = quadImageAlteration3;
			} else if (bottomLeftFlag == 4) {
				quadImageBottomLeft = quadImageAlteration4;
			}

			// Store relevant image in bottom right quadrant
			if (bottomRightFlag == 1) {
				quadImageBottomRight = quadImageAlteration1;
			} else if (bottomRightFlag == 2) {
				quadImageBottomRight = quadImageAlteration2;
			} else if (bottomRightFlag == 3) {
				quadImageBottomRight = quadImageAlteration3;
			} else if (bottomRightFlag == 4) {
				quadImageBottomRight = quadImageAlteration4;
			}

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

	}
}


