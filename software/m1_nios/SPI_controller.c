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
#include "../m1_nios_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi.h"
#include "../m1_nios_bsp/drivers/inc/altera_avalon_spi_regs.h"

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
	doubleTapFlag = 1;
}

int main(void) {

	// Defining variables
	int col = 320;
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
	float timeConversion = 1000000.00;
	float frameRate;
	int rateInt;
	int d0, d1, d2, d3;
	alt_u8 hex3, hex2, hex1, hex0;
	alt_u32 hexHigh, hexLow;
	alt_u8 pixel;
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
	int gyroISR_res = alt_ic_isr_register(GSENSOR_INT2_IRQ_INTERRUPT_CONTROLLER_ID,GSENSOR_INT2_IRQ,gyro_isr, context, 0x0);

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

			// Gyro stuff
			if (doubleTapFlag == 1){
				// read axis data
				alt_avalon_spi_command(SPI_0_BASE, 1, 1, &readX, 2, &xData, 0x0);
				alt_avalon_spi_command(SPI_0_BASE, 1, 1, &readY, 2, &yData, 0x0);
				alt_avalon_spi_command(SPI_0_BASE, 1, 1, &readZ, 2, &zData, 0x0);
				printf("X axis: %4d\t Y axis: %4d\t Z axis %4d\n", xData, yData, zData);
				doubleTapFlag = 0;
			}
			gyro_data_in = INT_SOURCE | 0x80;
			// read interrupt source
			alt_avalon_spi_command(SPI_0_BASE, 1, 1, &gyro_data_in, 1, &regData, 0x0);


			// Check if camera is ready with a frame
			camReady = IORD(CAMERA_BASE,0);
		}
	}
}

