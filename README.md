# Developer's Guide for a Wireless Game

The goal of this projects was to create a fun and interesting project which utilized a variety of different hardware sensors and components. Along with that, I wanted to give readers example code which utilizes current technologies like mesh networks, microcontroller displays, and accelerometers, with intention that you yourself can go off and do amazing things with the technology. In this project, I put a twist on a classic arcade game utilizing Arm Mbed [devices](https://developer.mbed.org/platforms/) and [components](https://developer.mbed.org/components/), and hope that you have as much fun as I did building something similar. To see the project in action, check out the accompanying video [here](TODO: insert youtube link).

## Project Overview

![](images/devices.png "Master device and two slave devices.")

In this project, there are three different microcontrollers (a master and two slaves). The master is responsible for bringing up the mesh network, assigning IP addresses to its slaves, and updating the game screen. The slave devices are issued out to game players, and are used like ping-pong paddles. When a player raises and lowers their paddles, the slave device's accelerometer data is transmitted to the master, and the master will update the player's paddle location on the screen based on that data. The players try to get as many points as possible by sending the ping-pong ball into the other player's goal. Once the max score is reached, a winner is determined and the game ends.

![](images/screenshot.png "Screenshot of the game.")

## Requirements
In order to build this project, you will need the following:
* [FRDM-K64F](https://developer.mbed.org/platforms/FRDM-K64F/) microcontrollers (x3).
* [ST7735 LCD Display](https://www.adafruit.com/product/358).

These devices communicate with each other using a newer wireless protocol named [Thread](http://threadgroup.org/What-is-Thread/Connected-Home), which has recently become an industry standard and is backed by an alliance of big name companies.
