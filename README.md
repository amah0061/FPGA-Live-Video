# FPGA Image Processing System — Multi-Core Nios II Architecture

**Monash University | ECE3073 Digital Systems Project | 2025**

---

## Overview
This project implements a real-time **image acquisition and processing system** on an FPGA platform using a multi-core **Nios II processor architecture**. An ESP32-CAM module captures image data, which is transferred to the FPGA and stored in SDRAM for processing and display.

The system supports both **single-image and quad-image display modes**, allowing multiple processed outputs to be visualised simultaneously. A range of image processing operations — including flipping, blurring, and edge detection — are performed in real time, with user interaction handled through onboard hardware inputs.

To improve performance and throughput, the design leverages **three independent Nios II processors**, distributing tasks across acquisition, processing, and display pipelines.

---

## Key Features
- Real-time image capture from ESP32-CAM  
- Multi-core processing using **three Nios II processors**  
- Dual display modes:
  - Single full-resolution image  
  - Quad-image comparison mode  
- Hardware-based user control (switches, keys, accelerometer)  
- Image processing operations:
  - Flip transformations  
  - Gaussian-style blur  
  - Edge detection  
- SDRAM-based frame buffering for efficient data handling  

---

## System Architecture

### 1. Image Acquisition
Image data is captured from the ESP32-CAM in a **packed pixel format** and streamed into SDRAM via the Nios II system.

### 2. Multi-Core Processing
The system distributes workload across three processors:
- **Input Processor** → Handles camera data ingestion  
- **Processing Processor** → Applies image transformations  
- **Display Processor** → Manages VGA output  

This parallel architecture significantly improves system responsiveness and throughput.

### 3. Display Output
A VGA controller reads processed image data from SDRAM and outputs:
- A single processed image, or  
- Four processed variants simultaneously (quad mode)

---

## Tech Stack
- **Hardware:** Intel DE10-Standard FPGA, ESP32-CAM  
- **Platform:** Quartus + Platform Designer (Qsys)  
- **Processors:** Nios II (multi-core system)  
- **Language:** C  
- **Libraries:** Custom drivers, FPGA IP cores  
- **Memory:** SDRAM for frame buffering  

---

## Setup & Build Instructions

### FPGA Configuration
1. Open project in Quartus (`.qpf`)
2. Open Platform Designer (`m1_nios_system.qsys`)
3. Generate HDL
4. Compile the project
5. Program FPGA using `.sof` file via Quartus Programmer

### Nios II Software Setup
1. Open Eclipse and set workspace to project directory  
2. Create Nios II Application + BSP for each processor:
   - Input processor  
   - Processing processor  
   - Display processor  
3. Select correct `.sopcinfo` file  
4. Configure BSP settings:
   - Enable *JTAG UART ignore FIFO full error*  
5. Generate BSPs and build all projects  
6. Create run configurations and execute  

---

## Applications
- Embedded image processing systems  
- FPGA-based vision pipelines  
- Real-time multi-core processing demonstrations  
- Educational platform for hardware/software co-design  

---

## Key Challenges
- Efficiently managing shared SDRAM access across multiple processors  
- Synchronising inter-processor communication and data flow  
- Handling real-time constraints on FPGA-based systems  
- Optimising memory bandwidth for image throughput  

---

## Acknowledgement
This project was originally developed as part of a group project (Team B06, Monash University).

This repository represents a personal copy with my contributions in:
- Multi-processor system design  
- Memory management and SDRAM interfacing  
- Image processing pipeline implementation  
- System integration and optimisation  

---

## License
This project is intended for educational use.  
A valid Intel Quartus license is required to build and run the system.
