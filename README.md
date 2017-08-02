# spyrit-remote
An Arduino Program to control the T2M Spyrit Max 2 with your Computer via the Serial monitor.

## What you will need
To create a simple remote control for the Drone I simply used an Arduino Uno and an NRF24L01+, which you can buy [here](https://www.roboter-bausatz.de/24/nrf24l01-funkmodul?number=RBS10024)(**Germany**) or [here](https://www.ebay.com/sch/sis.html?_itemId=null&_nkw=NRF24L01+Wireless+Transceiver+Module+2+4GHz+Antenna+SPI+For+Arduino+Microcontrol&_trksid=p2047675.m4100.l9146).

## Hardware Setup
The Connection is pretty straightforward, you just need to hook up the pins of the NRF24L01+ to the Corresponding pins of your Arduino Board. You can look up how to connect your pins, with this Image: http://www.sunrom.com/media/content/402/nrf24l01-pinout-top.jpg 

| Arduino Pin   | NRF24L01 Pin  |
| ------------- |:-------------:|
|      3.3V     |       VCC     |
|      GND      |       GND     |
|      D7       |       CE      |
|      D8       |       CSN     |
|      D11      |       MOSI    |
|      D12      |       MISO    |
|      D13      |       SCK     |
| Not Connected |       IRQ     |

## HOW to use it
You CANT simply upload this Sketch to your Arduino and expect it to work. In order to access the registers via the Low-Level Interface of the RF24 Library by https://github.com/nRF24/RF24. The only thing I did, was switching all functions of the RF24 Class to public, so that I could set individual Registers. To use the library yourself, download this repostitory and move the "libraries" folder to the location of your Arduino sketches. If you don't know what I mean with this, you might want to look [here](https://www.arduino.cc/en/Guide/Libraries#toc5). After that, you have to Compile the Sketch and upload it to your Arduino. It might not work immediately, because I hard-coded several pairing adresses and codes, which *might* be **Device-Specific** - I simply don't know, because I only have one Drone to test my Sketch with.

## Drone Controls
### Pairing the Drone and the Arduino
    * Switch on the Drone -> The LEDs of the Drone should flash rapidly
    * Plug the Arduino connected to the NRF24 into your Computer -> The lights of the Drone should flash more slowly and stop flashing after you opened the Serial Monitor.
    
 ### Controlling the Drone
 **DISCLAIMER: I WONT TAKE ANY RESPONSIBILITY FOR WHAT WILL HAPPEN WITH YOUR DRONE. FOR ME IT WORKS FINE; BUT THERE IS A SLIGHT CHANCE THAT YOUR DRONE MIGHT GO NUTS AND SUFFER A FLY-AWAY, OR DAMAGE YOUR, OR OTHER PEOPLES PROPERTY!**
 
 The Drone is currently only controlled with Single-Char-Commands, but maybe I will develop an Interface to control it with, for example an **XBOX-Controller**. 
 
 Also all Commands are Lower-Case, and the program is **CASE-SENSITIVE**
 
 |    Command   |      Action     |
 |--------------|-----------------|
 |      "t"     |Increase Throttle|
 |      "g"     |Decrease Throttle|
 |      "w"     |   Fly Forwards  |
 |      "s"     |   Fly Backwards |
 |      "a"     |   Fly Left      |
 |      "d"     |   Fly Right     |
 |      "q"     | Rotate Counter-Clockwise  |
 |      "e"     | Rotate Clockwise|
 |      "y"     | Start Up Engines|
 |      "f"     | Set Throttle to 0 /**EMERGENCY STOP**|
 |      "o"     | Set Throttle to MAX|
 |      "x"     | Slow Descent for Landing|
 |      "r"     | Reset Transmitter **STEERING VALUES ARE NOT RESETTED!**|
 
 When you first start the Engines, the Drone wont lift off, before ther throttle is above 140. This is to ensure that the Drone wont flip over at the first start, and is a Hardware Limitation built into the Control Electronics. After your fist lift off however, you can adjust the throttle as you please. 
 
 ## Final Words
 
 If you want to try this out yourself, I suggest that you start slowly, maybe by firstly just train to reliably and fastly switch on and off the Engines. After that you can try to lift off and land. I hope you Guys have fun with this and it might help someone. If you have further questions or Issues, dont Hesitate to open an Issue. I am not that active on GitHub, but I check my page every Day in case there are Issues with my code. **One Last Dsclaimer: This Code was more or less bodged together as a little project, because I recently finished School and had a lot of free time. I don' have any kind of degree in electronics or programming, so don't expect my program to work flawlessly. I fixed all major bugs for me, but I cant tell you wehter your Drone will behave the same Way as mine did. So Please be careful with this program. Have Fun and Stay Safe :D**
 
