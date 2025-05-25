# Project Name: ECE3073 Project - Milestone 1 (B06)

## Group members
- Aadi Mahajan            [33855994]   (amah0061@student.monash.edu) amah0061
- James Thomson           [33856257]   (jtho0098@student.monash.edu) James-Thomson1
- Lance Miranda           [31481795]   (lmir0004@student.monash.edu) LanceMir
- Ryan Shanta             [32284470]   (rsha0057@student.monash.edu) rsha0057
- Xavier Hasiotis-Welsh   [33880271]   (xhas0001@student.monash.edu) Xavier714714

## Project Description:
This project takes an image from an ESP-Camera and utilises a NIOS processor to store the image in SDRAM. The camera can take in a larger image or an image a quarter of the size depending on if the user wants to output a single image or four images at once. The processor can then apply alterations to the image including flipping, blurring and edge detection depending on what the user wishes to display. In single image mode the VGA Controller then outputs the desired image to an output peripheral and in quad image mode the VGA Controller then displays the desired four smaller images to the output peripheral. The user controls these images using the DE-10s switches, keys, and onboard accelerometer. The images are collected from the camera using a packed data format and are displayed in colour. In addition, the project utilises a total of three processors to boost system performance.

In order to complete this, the project encompasses the qsys system made using the Quartus Platform Designer. It also utilises the Quartus IP Block to create supporting hardware. Using this hardware setup, Eclipse software (c code) enables an image to be output.

## Installation / Setup Instructions:
- Open Quartus and select the open project option
- Navigate through your directory to the local repository and select the .qpf project file
- Open the platform designer and select the m1_nios_system.qsys file and generate HDL
- Compile the main Quartus program
- Go to the programmer and add the .sof file and run
- Open eclipse and set the workspace to the base ECE3073 Project folder
- Click file -> new -> NIOS II Application and BSP from Template
- Select the .sopc file in the project directory and create a hello world small project and select the spi processor
- Repeat the above two steps to create projects for the display and processing processors
- Ensure each processor is using the right memory space by checking the linker script of each bsp
- In each bsp editor under drivers ensure the "enable jtag uart ignore fifo full error" box is ticked
- Right-click on the each bsp folder and select the NIOS II option. And then generate BSP
- Select project at the top and clean project and then press crtl + B to build the project
- Create a run configuration and then run the project

## Usage Examples
This project has wide usage applications, one of which is a way to learn the basics of input/output peripherals and understanding how processor memory operates. The project can also apply a number of alterations to images and display and compare them. Additionally, the project can display a video feed in colour.

## Licensing
To ensure the project runs correctly, a Quartus license is required. Make sure you are connected to eduroam while on Monash campus or connect to Monash VPN if you are at home.
