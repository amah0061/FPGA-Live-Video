/* This file generates the controller for SPI bus, which allows data to be sent and received through
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
#include "../m1_nios_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi_regs.h"
#include "stdlib.h"

// Gyro write Registers
#define    BW_RATE        0x2c
#define    POWER_CONTROL  0x2d
#define    DATA_FORMAT    0x31
#define    INT_ENABLE     0x2E
#define    INT_MAP        0x2F
#define    THRESH_ACT     0x24
#define    THRESH_INACT   0x25
#define    TIME_INACT     0x26
#define    ACT_INACT_CTL  0x27
#define    THRESH_FF      0x28
#define    TIME_FF        0x29
#define    TAP_AXES       0x2a
#define    TAP_THRES      0x1d
#define    LATENT         0x22
#define    DUR            0x21
#define    WINDOW         0x23

// Gyro read Registers
#define    INT_SOURCE     0x30
#define    X_LB           0x32
#define    X_HB           0x33
#define    Y_LB           0x34
#define    Y_HB           0x35
#define    Z_LB           0x36
#define    Z_HB           0x37
#define CONFIG_LENGHT 16 * 2
#define MAX_COUNT 500000
#define READ_X_AXIS 0xc0 | X_LB // enable read bit and multi byte
#define READ_Y_AXIS 0xc0 | Y_LB // enable read bit and multi byte
#define READ_Z_AXIS 0xc0 | Z_LB // enable read bit and multi byte

// Global interrupt flags
volatile int doubleTapFlag = 0;
volatile int keyFlag = 0;
volatile int swFlag = 0;

// Configure the gyro
alt_u8 gyro_config[CONFIG_LENGHT] = {
    DATA_FORMAT, 0x0b,    // 4-wire SPI, full resolution, +/- 16g
    THRESH_ACT, 0x04,
    THRESH_INACT, 0x02,
    TIME_INACT, 0x02,
    ACT_INACT_CTL, 0xff,
    THRESH_FF, 0x09,
    TIME_FF, 0x46,
    TAP_THRES, 0x10,
    TAP_AXES, 0x07,
    LATENT, 0x85,
    DUR, 0x40,
    WINDOW, 0xc0,
    BW_RATE, 0x0a,
    INT_ENABLE, 0x60,
    INT_MAP, 0x20,
    POWER_CONTROL, 0x08
  };

// Interrupt handler
void gyro_isr(void * context) {
    // clear the interrupt
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(GSENSOR_INT2_BASE,0);
	IOWR(GSENSOR_INT2_BASE, 3, 0);
	doubleTapFlag = (doubleTapFlag + 1) % 4;
}

// key interrupt handler
void key_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE,0);
	IOWR(KEY_BASE, 3, 0);
	keyFlag = 1 - keyFlag;
}

// switch interrupt handler
void switch_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(SW_BASE,0);
	IOWR(SW_BASE, 3, 0);
	swFlag = IORD(SW_BASE,0);
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

// Convolution function
void convolve(void *inputImage, void *outputImage, void *kernel, int width, int height) {
	// Defining variables
	alt_u8 *in = (alt_u8 *)inputImage;
	alt_u8 *out = (alt_u8 *)outputImage;
	float runningValue;
	float *k = (float *)kernel;

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

        	// Assigning value to current pixel
        	out[j+i*width] = runningValue;
        }
    }
}

int main(void) {

	// Defining variables
	int singleCol = 320;
	int singleRow = 240;
	int col = 160;
	int row = 120;
	int totalCol = 320;
	int quadFrameSize = row * col;
	int singleFrameSize = singleCol*singleRow;
	alt_u8 camModeQuad = 0x12;
	alt_u8 camModeSingle = 0x10;
	alt_u8 *singleImage = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *singleImageDisplay = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *singleImageAlteration1 = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *singleImageAlteration2 = (alt_u8 *)malloc(singleFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageOrigin = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration1 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration2 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration3 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration4 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageAlteration5 = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageTopRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomLeft = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	alt_u8 *quadImageBottomRight = (alt_u8 *)malloc(quadFrameSize * sizeof(alt_u8));
	int topLeftFlag = 1;
	int topRightFlag = 1;
	int bottomLeftFlag = 1;
	int bottomRightFlag = 1;
	int deviceSelect = 2;
	int selectedAlteration;
	int bufferSize = 1;
	int flag = 0;
	int camReady;
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
	float kernelBlur[9] = {0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11, 0.11};
	float kernelEdgeX[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
	float kernelEdgeY[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
	int thresholdValue = 100;

	// Gyro
	alt_u8 gyro_data_in;
	alt_u8 gyro_data_out;
	alt_u8 readX = READ_X_AXIS;
	alt_u8 readY = READ_Y_AXIS;
	alt_u8 readZ = READ_Z_AXIS;
	alt_16 xData;
	alt_16 yData;
	alt_16 zData;
    alt_u8 regData;
    alt_u8 isrRes = 0xff;

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

	// Gyro initialisation
	for (int i = 0; i < CONFIG_LENGHT; i += 2) // load config into module
	{
	    alt_avalon_spi_command(SPI_0_BASE, 1, 2, gyro_config + i, 0, &gyro_data_out, 0);
	}

	// Gyro interrupt setup
	void* context = (void *) &isrRes;
	IOWR(GSENSOR_INT2_BASE, 3, 0);
	IOWR(GSENSOR_INT2_BASE, 2, 0x1);
	int gyroISR_res = alt_ic_isr_register(GSENSOR_INT2_IRQ_INTERRUPT_CONTROLLER_ID,GSENSOR_INT2_IRQ, gyro_isr, context, 0x0);

	// Key interrupt setup
	IOWR(KEY_BASE, 3, 0); // Clear edge
	IOWR(KEY_BASE, 2, 0x2); // Enable key interrupts for key 1
	int keyISR_res = alt_ic_isr_register(KEY_IRQ_INTERRUPT_CONTROLLER_ID, KEY_IRQ, key_isr, NULL, 0x0);

	// Switch interrupt setup
	IOWR(SW_BASE, 3, 0); // Clear edge
	IOWR(SW_BASE, 2, 0xF); // Enable interrupts for switch 0 to 3 (first 4 switches to select which of the 4 displays to turn on)
	int swISR_res = alt_ic_isr_register(SW_IRQ_INTERRUPT_CONTROLLER_ID, SW_IRQ, switch_isr, NULL, 0x0);



	// main loop
	while(1){
		// startTime reads the current microsecond count in order to track the FPS
		startTime = IORD(USEC_COUNTER_BASE, 0);


		if (keyFlag == 0) { 		// Receiving the frame (Single)
			alt_avalon_spi_command(
				SPI_0_BASE, 		// SPI base
				deviceSelect, 		// sub device (slave select)
				bufferSize, 		// Transmit buffer size (1 bit command)
				&camModeSingle,		// Setting data capture mode
				singleFrameSize,	// Size of receive buffer
				singleImage ,  		// Saving camera data to destination buffer
				flag				// flags: told to set to 0
			);

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
				convolve(singleImage, singleImageAlteration1, kernelBlur, singleCol, singleRow);;
				singleImageDisplay = singleImageAlteration1;

			} else if (doubleTapFlag == 3) {	//Edge Detection image
				convolve(singleImage, singleImageAlteration1, kernelEdgeX, singleCol, singleRow);
				convolve(singleImage, singleImageAlteration2, kernelEdgeY, singleCol, singleRow);

				// Combine X and Y Sobel filters and checking threshold
				for (int i = 1; i < singleRow - 1; i++){
					for (int j = 1; j < singleCol - 1; j++){
						singleImageAlteration1[j+i*singleCol] = abs(singleImageAlteration1[j+i*singleCol]) + abs(singleImageAlteration2[j+i*singleCol]);

						if (singleImageAlteration1[j+i*singleCol] < thresholdValue){
							singleImageAlteration1[j+i*singleCol] = 0;
						}
					}
				}
				singleImageDisplay = singleImageAlteration1;
			}
		} else if (keyFlag == 1) { 	// Receiving the frame (Quad)
			alt_avalon_spi_command(
				SPI_0_BASE, 		// SPI base
				deviceSelect, 		// sub device (slave select)
				bufferSize, 		// Transmit buffer size (1 bit command)
				&camModeQuad,		// Setting data capture mode
				quadFrameSize,  	// Size of receive buffer
				quadImageOrigin,	// Saving camera data to destination buffer
				flag				// flags: told to set to 0
			);

			// Shift each pixel 4 bits to the right to get rid of junk
			for (int i = 0; i < row; i++){
				for (int j = 0; j < col; j++){
					quadImageOrigin[j+i*col] = quadImageOrigin[j+i*col]>>4;
				}
			}
		}

		// Reset camera to be in a state to not except data
		camReady = 0;

		// Writing frame to pixel bugger
		while (camReady == 0){

			if (keyFlag == 0) {
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
				// Read y position of DE-10
				alt_avalon_spi_command(SPI_0_BASE, 1, 1, &readY, 2, &yData, 0x0);

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
					convolve(quadImageOrigin, quadImageAlteration4, kernelEdgeX, col, row);
					convolve(quadImageOrigin, quadImageAlteration5, kernelEdgeY, col, row);
					// Combine X and Y Sobel filters and checking threshold
					for (int i = 1; i < row - 1; i++){
						for (int j = 1; j < col - 1; j++){
							quadImageAlteration4[j+i*col] = abs(quadImageAlteration4[j+i*col]) + abs(quadImageAlteration5[j+i*col]);

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
						int pixelAddress = j + i * col;
						int topLeftAddress = j + i * totalCol;
						int topRightAddress = (j + 160) + i * totalCol;
						int bottomLeftAddress = j + (i + 120) * totalCol;
						int bottomRightAddress = (j + 160) + (i + 120) * totalCol;

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

			gyro_data_in = INT_SOURCE | 0x80;
			// read interrupt source
			alt_avalon_spi_command(SPI_0_BASE, 1, 1, &gyro_data_in, 1, &regData, 0x0);

			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}

