# Project Name: ECE3073 Project - Milestone 1 (B06)

## Group members
- Aadi Mahajan            [33855994]   (amah0061@student.monash.edu)
- James Thomson           [33856257]   (jtho0098@student.monash.edu)
- Lance Miranda           [31481795]   (lmir0004@student.monash.edu)
- Ryan Shanta             [32284470]   (rsha0057@student.monash.edu)
- Xavier Hasiotis-Welsh   [33880271]   (xhas0001@student.monash.edu)

## Project Description:
This project takes an image from an ESP-Camera and utilises a NIOS processor to store the image in SDRAM. This image is then sent to a pixel buffer which then forwards it to the VGA Controller. THe VGA Controller then outputs this image to an output peripheral. 
In order to complete this, the project encompasses the qsys system made using the Quartus Platform Designer. It also utilises the Quartus IP Block to create supporting hardware. Using this hardware setup, Eclipse software (c code) enables an image to be output.

## Installation / Setup Instructions:
- Open Quartus and select the open project option
- Navigate through your directory to the M1 folder and select the .qpf project file
- Open the platform designer and select the m1_nios_system.qsys file and generate HDL
- Compile the main Quartus program
- Go to the programmer and add the .sof file and run
- Open eclipse and set the workspace to the base ECE3073 Project folder
- Click file -> import -> general -> existing projects into workspace -> Next
- To select the root directory, browse your files for the ECE3073 Project folder and click ok. The m1_nios and m1_nios_bsp folders should show up and make sure they are selected, then click finsih.
- Right-click on the nios_system_bsp and select the NIOS II option. And then generate BSP
- Save the file and then select project at the top and clean project and then press crtl + B to build the project
- Create a run configuration and then run the project

## Usage Examples
This project has wide usage applications, one of which is a way to learn the basics of input/output peripherals and understanding how processor memory operates. 

## Licensing
To ensure the project runs correctly, a Quartus license is required. Make sure you are connected to eduroam while on Monash campus or connect to Monash VPN if you are at home.
