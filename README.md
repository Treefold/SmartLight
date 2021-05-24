# SmartLight

## Analysis report

[Raport de analiza SmartLight.pdf](https://github.com/Treefold/SmartLight/blob/aef9e5cbc2f3b164e6b45a3e0472d3f155057536/Raport%20de%20analiza%20SmartLight.pdf)

## Sistem requirements

- Linux (preferably Unbuntu 18 or later)
- Windows / MacOS with VM (see more about this in [**Instructions**](https://github.com/Treefold/SmartLight/blob/main/README.md#instructions))

### Libraries and dependecies needed

See more on how to install them in [**Installing the dependencies**](https://github.com/Treefold/SmartLight/blob/aef9e5cbc2f3b164e6b45a3e0472d3f155057536/README.md#installing-the-dependencies)

- pistache
- mosquitto
- nlohmann-json3-dev
- gcc (for c++17 or later)

## Setup Instructions

### Downloads (skip on Linux)

Download and Install the VM: https://www.oracle.com/virtualization/technologies/vm/downloads/virtualbox-downloads.html

Download the Ubuntu Server ISO: https://ubuntu.com/download/server (Option 2 - Manual server installation)

### Setting up the VM (skip on Linux)

In virtualbox click create new, give it a name and select `Linux - Ubuntu(64-bit)` and change the values as you need them.

Clink on your `VM -> Settings -> Storage -> Add Optical Drive`

If the iso is not listed, click `Add`, browse your files, select and open it.
Click on the iso and choose.
Click `Ok`.

Start the VM normally and procede with the installation of the OS.

When done, **reboot** and **log in**.

### Install the dependencies

_If you don't have Linux, all of those should be done in the VM's cmd_

First, clone this repository
	
	git clone https://github.com/Treefold/SmartLight.git

Then install `pistache`:

	sudo add-apt-repository ppa:pistache+team/unstable
	
	sudo apt update
	
	sudo apt install libpistache-dev

Then installn`mosquitto`:

	sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
	
	sudo apt update
	
	sudo apt install mosquitto
	sudo apt install mosquitto-clients
	sudo apt install libmosquitto-dev

Then install `nlohmann-json3-dev`:

	sudo apt-get install -y nlohmann-json3-dev

And install `g++` command:

	sudo apt install g++

### Compile and run our project
_If you don't have Linux, all of those will be done in the VM's cmd_

Go in the `Smartlight` folder: 

	cd Smartlight
	
Compile and run the project:

	g++ ServerMQTT.cpp -o server -lpistache -lcrypto -lssl -lpthread -std=c++17 -lmosquitto \
	&& ./server

### Shut down

Press `Ctrl-C` and then `Enter`.


## Interaction

### Samples

Open another console tab and run one of the following:
	
	curl -X POST http://localhost:9080/init/0
	
	curl -X GET  http://localhost:9080/settings/0
	
	curl -X POST -H "Content-Type: application/json" -d @window_settings.json http://localhost:9080/settings
	
	mosquitto_pub -t test/t1 -m "impact: 10"

### Light

To give your custom light settings run the following command:
	
	curl -X POST -H "Content-Type: application/json" -d @user_settings_sample.json http://localhost:9080/settings
	
you can replace `user_settings_sample.json` with any `.json` with the desired settings.

If `manual` is set to `false` (meaning the light is set to automatic), the server receives data from the sensors and automatically sets the values for luminosity and temperature.

### Music

To play short_sample.mp3 right now on the device number 1 run:

	curl -X POST -d @short_sample.mp3 http://localhost:9080/play/1/0

To add short_sample.mp3 to the queue on the device number 3 run:

	curl -X POST -d @short_sample.mp3 http://localhost:9080/play/3/0

### Alarm
To use the alarm API use:
	
	curl -X POST http://localhost:9080/alarm/1/10/30
	
	curl -X GET http://localhost:9080/alarm/1
	
	curl -X DELETE http://localhost:9080/alarm/1/10/30

### Alerts

To trigger temper alerts run:
	
	mosquitto_pub -t test/t1 -m "impact: 10"
	mosquitto_pub -t test/t1 -m "impact: 10"
	mosquitto_pub -t test/t1 -m "impact: 10"

or

	mosquitto_pub -t test/t1 -m "impact: 70"
