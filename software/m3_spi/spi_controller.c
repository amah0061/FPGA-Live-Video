/* This file generates the controller for SPI bus, which allows data to be sent and received through
 * the corresponding hardware and peripherals.
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
#include "altera_avalon_spi.h"
#include "altera_avalon_spi_regs.h"
#include "stdlib.h"
#include "altera_avalon_mutex.h"

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

// define shared buffer variables:
int *doubleTapFlagS = (int*)0x03500000;
int *keyFlagS = (int*)0x03500004;
int *swFlagS = (int*)0x03500008;
int *yDataS = (int*)0x0350000C;
alt_u16 *singleFrameS = (alt_u16*)0x03500010;
alt_u16 *quadFrameS = (alt_u16*)0x03525810;

// Global interrupt flags
volatile int doubleTapFlag = 0;
volatile int keyFlag = 0;
volatile int swFlag = 0;
volatile int commFlagProcessing = 0;
volatile int commFlagDisplay = 1;

// Configure the gyro
alt_u8 gyro_config[CONFIG_LENGHT] = {
    DATA_FORMAT, 0x0b,    // 4-wire SPI, full resolution, +/- 16g
    THRESH_ACT, 0x04,
    THRESH_INACT, 0x02,
    TIME_INACT, 0x02,
    ACT_INACT_CTL, 0xff,
    THRESH_FF, 0x09,
    TIME_FF, 0x46,
    TAP_THRES, 0x12,
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
	//doubleTapFlag = (doubleTapFlag + 1) % 4;
	doubleTapFlag = 0;
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

// communication interrupt handler
void comm_processing_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_SPI_IN_BASE,0);
	IOWR(P_SPI_IN_BASE, 3, 0);
	commFlagProcessing = 1;
}

// communication interrupt handler
void comm_display_isr () {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(P_SPI_IN_DISPLAY_BASE,0);
	IOWR(P_SPI_IN_DISPLAY_BASE, 3, 0);
	commFlagDisplay = 1;
}

int main(void) {

	// Defining variables
	int singleCol = 320;
	int singleRow = 240;
	int quadCol = 160;
	int quadRow = 120;
	int quadFrameSize = quadRow * quadCol * 1.5;
	int singleFrameSize = singleCol * singleRow * 1.5;
	alt_u8 camModeQuad = 0x17;
	alt_u8 camModeSingle = 0x15;
	alt_u8 *singleImage = (alt_u8 *)malloc(singleFrameSize);


	alt_u8 *quadImageOrigin = (alt_u8 *)malloc(quadFrameSize);
	int deviceSelect = 0;
	int bufferSize = 1;
	int flag = 0;
	int camReady;
	int idx;

	// mutex
	alt_mutex_dev *mutex = altera_avalon_mutex_open("/dev/mutex_0");

	// Gyro
	alt_u8 gyro_data_in;
	alt_u8 gyro_data_out;
	alt_u8 readY = READ_Y_AXIS;
	alt_16 yData;
    alt_u8 regData;
    alt_u8 isrRes = 0xff;

	// Gyro initialisation
	for (int i = 0; i < CONFIG_LENGHT; i += 2) // load config into module
	{
	    alt_avalon_spi_command(SPI_0_BASE, 1, 2, gyro_config + i, 0, &gyro_data_out, 0);
	}

	// Gyro interrupt setup
	void* context = (void *) &isrRes;
	IOWR(GSENSOR_INT2_BASE, 3, 0);
	IOWR(GSENSOR_INT2_BASE, 2, 0x1);
	alt_ic_isr_register(GSENSOR_INT2_IRQ_INTERRUPT_CONTROLLER_ID,GSENSOR_INT2_IRQ, gyro_isr, context, 0x0);

	// Key interrupt setup
	IOWR(KEY_BASE, 3, 0); // Clear edge
	IOWR(KEY_BASE, 2, 0x2); // Enable key interrupts for key 1
	alt_ic_isr_register(KEY_IRQ_INTERRUPT_CONTROLLER_ID, KEY_IRQ, key_isr, NULL, 0x0);

	// Switch interrupt setup
	IOWR(SW_BASE, 3, 0); // Clear edge
	IOWR(SW_BASE, 2, 0xF); // Enable interrupts for switch 0 to 3 (first 4 switches to select which of the 4 displays to turn on)
	alt_ic_isr_register(SW_IRQ_INTERRUPT_CONTROLLER_ID, SW_IRQ, switch_isr, NULL, 0x0);

	// Communication interrupt setup
	IOWR(P_SPI_IN_BASE, 3, 0); // Clear edge
	IOWR(P_SPI_IN_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_SPI_IN_IRQ_INTERRUPT_CONTROLLER_ID, P_SPI_IN_IRQ, comm_processing_isr, NULL, 0x0);

	// Communication interrupt setup
	IOWR(P_SPI_IN_DISPLAY_BASE, 3, 0); // Clear edge
	IOWR(P_SPI_IN_DISPLAY_BASE, 2, 0x1); // Enable interrupts
	alt_ic_isr_register(P_SPI_IN_DISPLAY_IRQ_INTERRUPT_CONTROLLER_ID, P_SPI_IN_DISPLAY_IRQ, comm_display_isr, NULL, 0x0);


	// main loop
	while(1){

		gyro_data_in = INT_SOURCE | 0x80;
		// read interrupt source
		alt_avalon_spi_command(SPI_0_BASE, 1, 1, &gyro_data_in, 1, &regData, 0x0);

		if (keyFlag == 0) { 		// Receiving the frame (Single)
			alt_avalon_spi_command(
				SPI_0_BASE, 		// SPI base
				deviceSelect, 		// sub device (slave select)
				bufferSize, 		// Transmit buffer size (1 bit command)
				&camModeSingle,		// Setting data capture mode
				singleFrameSize,	// Size of receive buffer
				singleImage,  		// Saving camera data to destination buffer
				flag				// flags: told to set to 0
			);
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

			// Read y position of DE-10
			alt_avalon_spi_command(SPI_0_BASE, 1, 1, &readY, 2, &yData, 0x0);
		}

		// Reset camera to be in a state to not except data
		camReady = 0;

		// Wait for communication from other display processor
		while (commFlagDisplay == 0){

		}
		commFlagDisplay = 0;

		// Write frame, SW/Key/Doubletap/yData flags to memory
	    altera_avalon_mutex_lock(mutex, 1);
	    // Write flags
	    IOWR(doubleTapFlagS,0,doubleTapFlag);
	    IOWR(keyFlagS,0,keyFlag);
	    IOWR(swFlagS,0,swFlag);
	    IOWR(yDataS,0,yData);
	    // Write frames
	    if (keyFlag == 0){
	    	int spiIdx = -1;
				for (int i = 0; i < singleRow; i++) {
					for (int j = 0; j < singleCol; j += 2) {
						alt_u8 byte1 = singleImage[spiIdx++];
						alt_u8 byte2 = singleImage[spiIdx++];
						alt_u8 byte3 = singleImage[spiIdx++];

						// Pixel 1
						alt_u8 r1 = (byte1 & 0xF0) >> 4;
						alt_u8 g1 = (byte1 & 0x0F);
						alt_u8 b1 = (byte2 & 0xF0) >> 4;
						alt_u16 pixel1 = (r1 << 8) | (g1 << 4) | b1;

						// Pixel 2
						alt_u8 r2 = (byte2 & 0x0F);
						alt_u8 g2 = (byte3 & 0xF0) >> 4;
						alt_u8 b2 = (byte3 & 0x0F);
						alt_u16 pixel2 = (r2 << 8) | (g2 << 4) | b2;

						int idx1 = j + i * singleCol;
						int idx2 = idx1 + 1;

						IOWR((alt_u16*)singleFrameS, idx1, pixel1);
						IOWR((alt_u16*)singleFrameS, idx2, pixel2);
					}
				}
	    } else if (keyFlag == 1){
	    	int spiIdx = -1;
				for (int i = 0; i < quadRow; i++) {
					for (int j = 0; j < quadCol; j += 2) {
						alt_u8 byte1 = quadImageOrigin[spiIdx++];
						alt_u8 byte2 = quadImageOrigin[spiIdx++];
						alt_u8 byte3 = quadImageOrigin[spiIdx++];

						// Pixel 1
						alt_u8 r1 = (byte1 & 0xF0) >> 4;
						alt_u8 g1 = (byte1 & 0x0F);
						alt_u8 b1 = (byte2 & 0xF0) >> 4;
						alt_u16 pixel1 = (r1 << 8) | (g1 << 4) | b1;

						// Pixel 2
						alt_u8 r2 = (byte2 & 0x0F);
						alt_u8 g2 = (byte3 & 0xF0) >> 4;
						alt_u8 b2 = (byte3 & 0x0F);
						alt_u16 pixel2 = (r2 << 8) | (g2 << 4) | b2;

						int idx1 = j + i * quadCol;
						int idx2 = idx1 + 1;



					}
				}
	    }
	    // Unlock mutex
	    altera_avalon_mutex_unlock(mutex);

	    // Set swFlag back to 0
	    swFlag = 0;

	    // Tell other processor that it has written images and flags
	    IOWR(P_SPI_OUT_BASE,0,1);
	    IOWR(P_SPI_OUT_BASE,0,0);

	    // Wait for communication from other processing processor
	    while (commFlagProcessing == 0){

	    }
	    commFlagProcessing = 0;

		// Writing frame to pixel bugger
		while (camReady == 0){
			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}


