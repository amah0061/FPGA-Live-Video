/* This file is for the processor which is in charge of
 * processing data to make it ready for display
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
int *doubleTapFlagS = (int*)0x03500000;
int *keyFlagS = (int*)0x03500004;
int *swFlagS = (int*)0x03500008;
int *yDataS = (int*)0x0350000C;
alt_u8 *singleFrameS = (alt_u8*)0x03500010;
alt_u8 *quadFrameS = (alt_u8*)0x03512C10;
int *keyFlagDisplay = (int*)0x03517710;
alt_u16 *processedSingleFrameS = (alt_u16*)0x03517714;
alt_u16 *processedTopLeft = (alt_u16*)0x0352A314;
alt_u16 *processedTopRight = (alt_u16*)0x0353CF14;
alt_u16 *processedBottomLeft = (alt_u16*)0x0354FB14;
alt_u16 *processedBottomRight = (alt_u16*)0x03562714;

// Define volatile ints
volatile int comm0Flag = 0;
volatile int comm1Flag = 0;

// communication with SPI interrupt handler
void comm_spi_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_PROCESSING0_IN_BASE,0);
	IOWR(P_PROCESSING0_IN_BASE, 3, 0);
	comm0Flag = 1;
}

// communication with display interrupt handler
void comm_display_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_PROCESSING1_IN_BASE,0);
	IOWR(P_PROCESSING1_IN_BASE, 3, 0);
	comm1Flag = 1;
}

// Image flipping algorithm
void flip(void *inputImage, void *outputImage, int width, int height) {
	// Defining variables
	alt_u16 *in = (alt_u16 *)inputImage;
	alt_u16 *out = (alt_u16 *)outputImage;
	for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++) {
            out[j+i*width] = in[(width-1-j)+(height-1-i)*width];
        }
    }
}

// Convolve function
void convolve(void *inputImage, void *outputImage, void *kernel, int width, int height) {

	// Defining variables
	alt_u16 *in = (alt_u16 *)inputImage;
	alt_16 *out = (alt_16 *)outputImage;
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
	int quadFrameSizeOrigin = row * col * 12 / 8;
	int singleFrameSizeOrigin = singleCol*singleRow*12/8;
	int quadFrameSize = row * col;
	int singleFrameSize = singleCol*singleRow;
	alt_u16 *singleImageOrigin = (alt_u16 *)malloc(singleFrameSizeOrigin * sizeof(alt_u16));
	alt_u16 *singleImageR = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageG = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageB = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImage = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageDisplay = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageAlteration1 = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageAlteration2 = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_u16 *singleImageAlteration3 = (alt_u16 *)malloc(singleFrameSize * sizeof(alt_u16));
	alt_16  *edgeX = (alt_16 *)malloc(singleFrameSize * sizeof(alt_16));
	alt_16  *edgeY = (alt_16 *)malloc(singleFrameSize * sizeof(alt_16));
	alt_u16 *quadImageOrigin = (alt_u16 *)malloc(quadFrameSizeOrigin * sizeof(alt_u16));
	alt_u16 *quadImage = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageR = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageG = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageB = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlterationR = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlterationG = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlterationB = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlteration1 = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlteration2 = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlteration3 = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageAlteration4 = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageTopLeft = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageTopRight = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageBottomLeft = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	alt_u16 *quadImageBottomRight = (alt_u16 *)malloc(quadFrameSize * sizeof(alt_u16));
	int topLeftFlag = 1;
	int topRightFlag = 1;
	int bottomLeftFlag = 1;
	int bottomRightFlag = 1;
	int selectedAlteration;
	int kernelBlur[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int kernelEdgeX[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
	int kernelEdgeY[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
	int thresholdValue = 18;
	int doubleTapFlag;
	int swFlag;
	int keyFlag;
	int yData;
	int idx;
	alt_u8 by0;
	alt_u8 by1;
	alt_u8 by2;
	alt_u8 r1;
	alt_u8 r2;
	alt_u8 g1;
	alt_u8 g2;
	alt_u8 b1;
	alt_u8 b2;

	// mutex
	alt_mutex_dev *mutex = altera_avalon_mutex_open("/dev/mutex_0");

	// Communication with SPI interrupt setup
	IOWR(P_PROCESSING0_IN_BASE, 3, 0); // Clear edge
	IOWR(P_PROCESSING0_IN_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_PROCESSING0_IN_IRQ_INTERRUPT_CONTROLLER_ID, P_PROCESSING0_IN_IRQ, comm_spi_isr, NULL, 0x0);

	// Communication with display interrupt setup
	IOWR(P_PROCESSING1_IN_BASE, 3, 0); // Clear edge
	IOWR(P_PROCESSING1_IN_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_PROCESSING1_IN_IRQ_INTERRUPT_CONTROLLER_ID, P_PROCESSING1_IN_IRQ, comm_display_isr, NULL, 0x0);

	while (1){

		// Wait for processor to send image
		while (comm0Flag == 0){

		}
		comm0Flag = 0;

		// Aquire mutex
		altera_avalon_mutex_lock(mutex, 1);
		// Read the flags
		doubleTapFlag = IORD(doubleTapFlagS,0);
		keyFlag = IORD(keyFlagS,0);
		swFlag = IORD(swFlagS,0);
		yData = IORD(yDataS,0);
		// Write shared information to new spot in SDRAM for display processor
		IOWR(keyFlagDisplay,0,keyFlag);
		// Read images
		if (keyFlag == 0){
			for (int i = 0; i < singleFrameSizeOrigin; i++) {
				singleImageOrigin[i] = IORD(singleFrameS, i);
		    }
		} else if (keyFlag == 1){
			for (int i = 0; i < quadFrameSizeOrigin; i++) {
				quadImageOrigin[i] = IORD(quadFrameS, i);
			}
		}
		// Unluck mutex
		altera_avalon_mutex_unlock(mutex);

		// Tell other processor it is ready for next image
		IOWR(P_PROCESSING0_OUT_BASE,0,1);
		IOWR(P_PROCESSING0_OUT_BASE,0,0);

		if (keyFlag == 0) {
			// Unpack data from camera
			for (int i = 0, p = 0; i < singleFrameSizeOrigin; i += 3, p += 2) {
				// Gather 3 bytes for 2 pixels
				by0 = singleImageOrigin[i];
				by1 = singleImageOrigin[i + 1];
				by2 = singleImageOrigin[i + 2];

				// Unpack and store first pixel
				r1 = by0 >> 4;
				g1 = by0 & 0x0F;
				b1 = by1 >> 4;
				singleImage[p+1] = (r1 << 8) | (g1 << 4) | b1;

				// Unpack and store second pixel
				r2 = by1 & 0x0F;
				g2 = by2 >> 4;
				b2 = by2 & 0x0F;
				singleImage[p] = (r2 << 8) | (g2 << 4) | b2;
			}
			// image alterations
			if (doubleTapFlag == 0) {			//Default image
				singleImageDisplay = singleImage;

			} else if (doubleTapFlag == 1) {	// Flipped image
				flip(singleImage, singleImageAlteration1, singleCol, singleRow);
				singleImageDisplay = singleImageAlteration1;

			} else if (doubleTapFlag == 2) {	//Blurred image
				// Split into RGB
				for (int i = 0; i < singleFrameSize; i++){
					singleImageR[i] = (singleImage[i] >> 8) & 0xf;
					singleImageG[i] = (singleImage[i] >> 4) & 0xf;
					singleImageB[i] = singleImage[i] & 0xf;
				}
				// Blur each colour
				convolve(singleImageR, singleImageAlteration1, kernelBlur, singleCol, singleRow);
				convolve(singleImageG, singleImageAlteration2, kernelBlur, singleCol, singleRow);
				convolve(singleImageB, singleImageAlteration3, kernelBlur, singleCol, singleRow);
				// Stitch back together
				for (int i = 0; i < singleFrameSize; i++){
					singleImageDisplay[i] = (singleImageAlteration1[i] << 8) | (singleImageAlteration2[i] << 4) | singleImageAlteration3[i];
				}

			} else if (doubleTapFlag == 3) {	//Edge Detection image
				// Convert back to greyscale
				for (int i = 0; i < singleFrameSize; i++){
					singleImageAlteration2[i] = 0.299*((singleImage[i] >> 8) & 0xf) + 0.587*((singleImage[i] >> 4) & 0xf) + 0.114*(singleImage[i] & 0xf);
				}
				convolve(singleImageAlteration2, edgeX, kernelEdgeX, singleCol, singleRow);
				convolve(singleImageAlteration2, edgeY, kernelEdgeY, singleCol, singleRow);

				// Combine X and Y Sobel filters and checking threshold
				for (int i = 1; i < singleRow - 1; i++){
					for (int j = 1; j < singleCol - 1; j++){
						singleImageAlteration1[j+i*singleCol] = abs(edgeX[j+i*singleCol]) + abs(edgeY[j+i*singleCol]);

						if (singleImageAlteration1[j+i*singleCol] < thresholdValue){
							singleImageAlteration1[j+i*singleCol] = 0;
						} else {
							singleImageAlteration1[j+i*singleCol] = 0xfff;
						}
					}
				}
				singleImageDisplay = singleImageAlteration1;
			}

			// Write frame to display processor
			altera_avalon_mutex_lock(mutex, 1);

			for (int i = 0; i < singleRow; i++) {
				for (int j = 0; j < singleCol; j++) {
					idx = j+i*singleCol;
					IOWR(processedSingleFrameS, idx, singleImageDisplay[idx]);
				}
			}

			// Unlock mutex
			altera_avalon_mutex_unlock(mutex);

		} else if (keyFlag == 1) {
			// Unpack data from camera
			for (int i = 0, p = 0; i < quadFrameSizeOrigin; i += 3, p += 2) {
				// Gather 3 bytes for 2 pixels
				by0 = quadImageOrigin[i];
				by1 = quadImageOrigin[i + 1];
				by2 = quadImageOrigin[i + 2];

				// Unpack and store first pixel
				r1 = by0 >> 4;
				g1 = by0 & 0x0F;
				b1 = by1 >> 4;
				quadImage[p+1] = (r1 << 8) | (g1 << 4) | b1;

				// Unpack and store second pixel
				r2 = by1 & 0x0F;
				g2 = by2 >> 4;
				b2 = by2 & 0x0F;
				quadImage[p] = (r2 << 8) | (g2 << 4) | b2;
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
				quadImageAlteration1 = quadImage;
			}

			// Call edge alteration if needed
			if (topLeftFlag == 2 || topRightFlag == 2 || bottomLeftFlag == 2 || bottomRightFlag == 2) {
				flip(quadImage, quadImageAlteration2, col, row);
			}

			// Call image blur if needed
			if (topLeftFlag == 3 || topRightFlag == 3 || bottomLeftFlag == 3 || bottomRightFlag == 3) {
				// Split into RGB
				for (int i = 0; i < quadFrameSize; i++){
					quadImageR[i] = (quadImage[i] >> 8) & 0xf;
					quadImageG[i] = (quadImage[i] >> 4) & 0xf;
					quadImageB[i] = quadImage[i] & 0xf;
				}
				// Blur each colour
				convolve(quadImageR, quadImageAlterationR, kernelBlur, col, row);
				convolve(quadImageG, quadImageAlterationG, kernelBlur, col, row);
				convolve(quadImageB, quadImageAlterationB, kernelBlur, col, row);
				// Stitch back together
				for (int i = 0; i < quadFrameSize; i++){
					quadImageAlteration3[i] = (quadImageAlterationR[i] << 8) | (quadImageAlterationG[i] << 4) | quadImageAlterationB[i];
				}
			}

			// Call edge detection if needed
			if (topLeftFlag == 4 || topRightFlag == 4 || bottomLeftFlag == 4 || bottomRightFlag == 4) {
				// Convert back to greyscale
				for (int i = 0; i < quadFrameSize; i++){
					quadImageAlterationR[i] = 0.299*((quadImage[i] >> 8) & 0xf) + 0.587*((quadImage[i] >> 4) & 0xf) + 0.114*(quadImage[i] & 0xf);
				}
				convolve(quadImageAlterationR, edgeX, kernelEdgeX, col, row);
				convolve(quadImageAlterationR, edgeY, kernelEdgeY, col, row);
				// Combine X and Y Sobel filters and checking threshold
				for (int i = 1; i < row - 1; i++){
					for (int j = 1; j < col - 1; j++){
						quadImageAlteration4[j+i*col] = abs(edgeX[j+i*col]) + abs(edgeY[j+i*col]);

						if (quadImageAlteration4[j+i*col] < thresholdValue){
							quadImageAlteration4[j+i*col] = 0;
						} else {
							quadImageAlteration4[j+i*col] = 0xfff;
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

			// Write quad frames to display processor
			altera_avalon_mutex_lock(mutex, 1);

			for (int i = 0; i < row; i++) {
				for (int j = 0; j < col; j++) {
					idx = j+i*col;
					IOWR(processedTopLeft, idx, quadImageTopLeft[idx]);
					IOWR(processedTopRight, idx, quadImageTopRight[idx]);
					IOWR(processedBottomLeft, idx, quadImageBottomLeft[idx]);
					IOWR(processedBottomRight, idx, quadImageBottomRight[idx]);

				}
			}

			// Unlock mutex
			altera_avalon_mutex_unlock(mutex);
		}

		// Tell display processor data has been shared and SDRAM is available
		IOWR(P_PROCESSING1_OUT_BASE,0,1);
		IOWR(P_PROCESSING1_OUT_BASE,0,0);

		// Wait for processor to send signal
		while (comm1Flag == 0){

		}
		comm1Flag = 0;

	}
}


