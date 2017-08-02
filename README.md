# Developer's Guide for a Wireless Game

The goal of this projects was to create a fun and interesting project which utilized a variety of different hardware sensors and components. Along with that, I wanted to give readers example code which utilizes current technologies like mesh networks, microcontroller displays, and accelerometers, with intention that you yourself can go off and do amazing things with the technology. In this project, I put a twist on a classic arcade game utilizing Arm Mbed [devices](https://developer.mbed.org/platforms/) and [components](https://developer.mbed.org/components/), and hope that you have as much fun as I did building something similar. To see the project in action, check out the accompanying video [here](TODO: insert youtube link).

## Project overview
<img src="images/devices.png" width="800">

In this project, there are three different microcontrollers (a master and two slaves). The master is responsible for bringing up the mesh network, assigning IP addresses to its slaves, and updating the game screen. The slave devices are issued out to game players, and are used like ping-pong paddles. When a player raises and lowers their paddles, the slave device's accelerometer data is transmitted to the master, and the master will update the player's paddle location on the screen based on that data. The players try to get as many points as possible by sending the ping-pong ball into the other player's goal. Once the max score is reached, a winner is determined and the game ends.

<img src="images/screenshot.png" width="600">

The devices in this project communicate with each other using a newer 2.4GHz wireless protocol named [Thread](http://threadgroup.org/What-is-Thread/Connected-Home), which has recently become an industry standard and is backed by an alliance of big name companies. For more information of Thread, an introduction is found [here](https://docs.mbed.com/docs/arm-ipv66lowpan-stack/en/latest/thread_intro/).

## Requirements
In order to build this project, you will need the following:
* [FRDM-K64F](https://developer.mbed.org/platforms/FRDM-K64F/) (x3).
* [6LoWPAN shield](TODO: update with components page link when available) (x3).
* [ST7735 LCD display](https://www.adafruit.com/product/358).
* [Male header pins](https://www.adafruit.com/product/392).
* [Female/Female jumper wires](https://www.adafruit.com/product/1950).
* [Male/Male jumper wires](https://www.adafruit.com/product/1956).
* Soldering Iron and solder.

Optional items to make devices battery powered:
* [PowerBoost 500 charger](https://www.adafruit.com/product/1944) (x3).
* [LiPo battery](https://www.adafruit.com/product/1578) (x3).
* [6in USB cable](https://www.adafruit.com/product/898) (x3).

## Hardware setup
In this section, I will explain all the necessary hardware connections that are needed to get the project running. First, I will go over how to build the master device, followed by setting up its two slave devices. If you decide to power the project with batteries, there is the optional final step here which will instruct you how to do so.

### Master device
The master device requires use of two different SPI module blocks found on the FRDM-K64F.  The first block is consumed by the 6LowPAN shield, which provides the Thread mesh network.  The ST7735 screen will use the second module block. The second SPI module block is accessed by soldering male pin headers onto the FRDM-K64F at the location marked in red in the image below.

<img src="images/without_headers.png" width="500">

Now prepare some male pin headers into a 2x4 configuration. Below is what they should look like.

<img src="images/header.png" width="500">

Below is what the device should look like after soldering the pins to the board.

<img src="images/with_headers.png" width="500">

The pin numbers and pin names for this new header is provided below. The numbers in this image corrispond to the numbers printed on the boards silkscreen next to the header.

<img src="images/pinout.png" width="200">

Now with these additional SPI pins available, you can connect the ST7735 LCD screen to them with jumper wires. The ST7735 LCD screen does not come with header pins, so those will have to be soldered on as well, as seen along the left side of the image below.

<img src="images/st7735.png" width="500">

The pinout table below shows how you should connect the ST7735 LCD to the FRDM-K64F. Note that the 2x4 header that you soldered onto the FRDM-K64F only has one Gnd pin, and no +3.3V pins. The remaining Gnd and +3.3V pins that are required can come from any other acceptable power/gnd pin found on the FRDM-K64F.

| ST7735 LCD pin | FRDM-K64F pin |
| :------------- |:------------- |
| LITE           | +3.3V         |
| MISO           | PTD7          |
| SCK            | PTD5          |
| MOSI           | PTD6          |
| TFT_CS         | PTD4          |
| CARD_CS        | Unconnected   |
| D/C            | PTC18         |
| RESET          | +3.3V         |
| VCC            | +3.3V         |
| Gnd            | Gnd           |

After the screen is fully connected to the board, the last step is to connect the 6LoWPAN shield to the FRDM-K64F. The 6LoWPAN shields have Arduino R3 headers, and should connect easily to the FRDM-K64F. The master device will now look something similar to the first image in this document.

### Slave devices
The only hardware needed for the slave devices is two FRDM-K64F microcontrollers and two 6LowPAN Shields. Simply attach the 6LoWPAN shields to the headers of the FRDM-K64F. The 6LoWPAN shields have Arduino R3 headers, and should connect easily to the FRDM-K64F.

### Battery powered devices (optional step)
I recommend powering the devices via batteries so that they are untethered during play. This makes playing the ping-pong game a more "realistic" experience. Below is an image of how I connected the PowerBoost 500 charger and LiPo battery to the FRDM-K64F devices. I simply placed the LiPo battery flat against the FRDM-K64F's backside, put the PowerBoost 500 charger on top of that, and then zip tied them together. Feel free to get creative here!  The PowerBoost 500 chargers are awesome, as they can charge your LiPos and also provide the required steady +5V that the FRDM-K64F required.

<img src="images/power.png" width="600">

## Software setup
Clone the repository onto your computer. Make sure mbed CLI is installed with the requirements as shown in this [video](https://www.youtube.com/watch?v=PI1Kq9RSN_Y&t=2s).
