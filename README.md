# SmartLight

## Instructions

### Downloads (skip on Linux):
	- Download and Install the VM: https://www.oracle.com/virtualization/technologies/vm/downloads/virtualbox-downloads.html
	- Download the Ubuntu Server ISO: https://ubuntu.com/download/server (Option 2 - Manual server installation)

### Setting up the VM (skip on Linux):
	- In virtualbox click create new, give it a name and select Linux - Ubuntu(64-bit) and change the values as you need them.
	- Clink on your VM -> Settings -> Storage -> Add Optical Drive
	- If the iso is not listed, click Add, browse your files, select it and open
	- Click on the iso and choose
	- Click ok
	- Start the VM normally and procede with the installation of the OS
	- When done, reboot and log in

### Installing the dependencies: 
	- (If you don't have Linux, all of those will be done in the VM's cmd)
	- git clone https://github.com/Treefold/SmartLight.git
	- sudo add-apt-repository ppa:pistache+team/unstable
	- sudo apt update
	- sudo apt install libpistache-dev
	- sudo apt-get install -y nlohmann-json3-dev
	- sudo apt install g++

### Compile out project:
	- (If you don't have Linux, all of those will be done in the VM's cmd)
	- Go in the Smartlight folder: cd Smartlight
	- Compile the project: g++ smartlight.cpp -o smartlight -lpistache -lcrypto -lssl -lpthread -std=c++17 
	- Run the our project: ./smartlight 

