# SmartLight

## Analysis report

[Raport de analiza SmartLight.pdf](https://github.com/Treefold/SmartLight/blob/aef9e5cbc2f3b164e6b45a3e0472d3f155057536/Raport%20de%20analiza%20SmartLight.pdf)

## Sistem requirements

- Linux (preferably Unbuntu 18 or later)
- Windows / MacOS with VM (see more about this in [**Instructions**](https://github.com/Treefold/SmartLight/blob/main/README.md#instructions))

### Libraries and dependecies needed

See more on how to install them in [**Installing the dependencies**](https://github.com/Treefold/SmartLight/blob/aef9e5cbc2f3b164e6b45a3e0472d3f155057536/README.md#installing-the-dependencies)

- pistache
- nlohmann-json3-dev
- gcc (for c++14 or later)

## Instructions

### Downloads (skip on Linux):

Download and Install the VM: https://www.oracle.com/virtualization/technologies/vm/downloads/virtualbox-downloads.html

Download the Ubuntu Server ISO: https://ubuntu.com/download/server (Option 2 - Manual server installation)

### Setting up the VM (skip on Linux):

In virtualbox click create new, give it a name and select `Linux - Ubuntu(64-bit)` and change the values as you need them.

Clink on your `VM -> Settings -> Storage -> Add Optical Drive`

If the iso is not listed, click `Add`, browse your files, select and open it.
Click on the iso and choose.
Click `Ok`.

Start the VM normally and procede with the installation of the OS.

When done, **reboot** and **log in**.

### Installing the dependencies: 

_If you don't have Linux, all of those should be done in the VM's cmd_

First, clone this repository
	
	git clone https://github.com/Treefold/SmartLight.git

Then get the `pistache` repository:

	sudo add-apt-repository ppa:pistache+team/unstable
	
And install it:

	sudo apt update \
	
	sudo apt install libpistache-dev

Then install `nlohmann-json3-dev`:

	sudo apt-get install -y nlohmann-json3-dev

And install `g++` command:

	sudo apt install g++

### Compile and run our project:
_If you don't have Linux, all of those will be done in the VM's cmd_

Go in the `Smartlight` folder: 

	cd Smartlight
	
Compile and run the project:

	g++ smartlight.cpp -o smartlight -lpistache -lcrypto -lssl -lpthread -std=c++17 && \
	./smartlight 

