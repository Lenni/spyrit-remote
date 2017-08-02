/*
Name:		doSymaFly.ino
Last updated : 21/11/15
(Created:	11/19/2015 9:34:07 PM)
Author:	Ran_the_User
*/

/* TODO:
	28/01/16
	check all control commands are available by keyboard/serial chars.
	  possible that only one of the directions is infrastructured.
*/

#include <RF24_config.h>
#include <RF24.h>
#include <printf.h>
#include <nRF24L01.h>
#include <SPI.h>
#include "SymaX.h"
#include <util/atomic.h>
#include <EEPROM.h>


#define MOSI_pin  MOSI  
#define SCK_pin   SCK   
#define MISO_pin  MISO  

#define CE_pin    7  //
#define CS_pin    8  // CSN pin

// pins definitions for Arduino Uno :
#define MOSI_on     PORTB |= _BV(3)  // PB3
#define MOSI_off    PORTB &= ~_BV(3) // PB3
#define SCK_on      PORTB |= _BV(5)  // PB5
#define SCK_off     PORTB &= ~_BV(5) // PB5

#define CE_on       PORTB |= _BV(1)   // PB1
#define CE_off      PORTB &= ~_BV(1)  // PB1
#define CS_on       PORTB |= _BV(2)   // PB2
#define CS_off      PORTB &= ~_BV(2)  // PB2

#define  MISO_on    (PINB & _BV(4)) // PB4

#define RF_POWER 1 // 0-3, it was found that using maximum power can cause some issues, so let's use 2... 

// PPM stream settings. recieve by serial commands.
//#define CHANNELS 12 // number of channels in ppm stream, 12 ideally
enum chan_order{
    THROTTLE,
    AILERON,
    ELEVATOR,
    RUDDER,
    AUX1,  // (CH5)  led light, or 3 pos. rate on CX-10, H7, or inverted flight on H101
    AUX2,  // (CH6)  flip control
    AUX3,  // (CH7)  still camera
    AUX4,  // (CH8)  video camera
    AUX5,  // (CH9)  headless
    AUX6,  // (CH10) calibrate Y (V2x2), pitch trim (H7), RTH (Bayang, H20), 360deg flip mode (H8-3D, H22)
    AUX7,  // (CH11) calibrate X (V2x2), roll trim (H7)
    AUX8,  // (CH12) Reset / Rebind
};

#define PPM_MIN           100 //1000
///#define PPM_SAFE_THROTTLE 110 // 1050 
#define PPM_MID           500
#define PPM_MAX           250 //2000
///#define PPM_MIN_COMMAND   130// 1300
#define PPM_MAX_COMMAND   700

// optional supported protocols. this version is SYMAX , (JJRC H8 mini, Floureon H101) limited.
enum {
    PROTO_BAYANG,       // EAchine H8 mini, H10, BayangToys X6, X7, X9, JJRC JJ850, Floureon H101
    PROTO_SYMAX5C1,     // Syma X5C-1 (not older X5C), X11, X11C, X12    
    PROTO_END
};

void toggle_thrust(RF24 radio);

// EEPROM locations
enum{
    ee_PROTOCOL_ID = 0,
    ee_TXID0,
    ee_TXID1,
    ee_TXID2,
    ee_TXID3
};

uint8_t               transmitterID[4];
uint8_t               current_protocol;
static volatile bool  ppm_ok	= false;
static bool           reset		= true;
uint8_t       steering_data[5] = { 80, 127, 127, 127};


byte cmd = 0;     // a place to put our serial data
//
#define GET_FLAG(ch, mask) (ppm[ch] > PPM_MAX_COMMAND ? mask : 0)
//


void setup()
{

	RF24 radio(CE_pin, CS_pin);
	//radio.begin();
	//radio.powerUp();

    pinMode(MOSI_pin, OUTPUT);
    pinMode(SCK_pin	, OUTPUT);
    pinMode(CS_pin	, OUTPUT);
    pinMode(CE_pin	, OUTPUT);
    pinMode(MISO_pin, INPUT);

	selectProtocol();

	Serial.begin(115200);

  Serial.println("control by : ");
  Serial.println(" t - go up (inc throttle),\n g - go down(dec throttle) ");
  Serial.println(" w - Forth(alleron-),\n s- Back(aleron+) ");
  Serial.println(" 'a'        - pitch up(elevetor-),\n 'd' - pitch down(elevetor+) ");
  Serial.println(" 'q'        - roll left(rudder-),\n 'e' - roll right(rudder+) ");

}

void loop()
{
	RF24 radio(CE_pin, CS_pin);
	printf_begin();
	//radio.printDetails(); 
	radio.begin();
	//radio.powerUp();

	while (true)
	{
		uint32_t timeout;
		// reset / rebind
		if (reset) {
			reset = false;
			RF24 radio(CE_pin, CS_pin);
			radio.begin();
			selectProtocol();
			init_protocol(radio);
			Serial.println("done reset");
		}

		/// check serial inputs from user    
		if (Serial.available())
		{

			cmd = Serial.read();
			Serial.write(cmd); Serial.write(','); Serial.write('\n');

			switch (cmd)
			{
			case 't':  if (steering_data[0] <= 254)	steering_data[0] += 10;	Serial.println(steering_data[0]);    break;
			case 'g':  if (steering_data[0] >= 1)	steering_data[0] -= 10;	Serial.println(steering_data[0]);    break;

			case 'w':  if (steering_data[1] <= 244) steering_data[1] -= 10;	Serial.println(steering_data[1]);    break;
			case 's':  if (steering_data[1] >= 10) steering_data[1] += 10;	Serial.println(steering_data[1]);    break;

			case 'q':  if (steering_data[2] <= 127+20) steering_data[2] += 1;	Serial.println(steering_data[2]);    break;
			case 'e':  if (steering_data[2] >= 127-20) steering_data[2] -= 1;	Serial.println(steering_data[2]);    break;

			case 'a':  if (steering_data[3] <= 244) steering_data[3] -= 10;	Serial.println(steering_data[3]);    break;
			case 'd':  if (steering_data[3] >= 10) steering_data[3] += 10;	Serial.println(steering_data[3]);    break;

			case 'y':  toggle_thrust(radio);    break;

			case 'o':  steering_data[0] = 255;  Serial.println("Full Throttle");  break;
			case 'x':  steering_data[0] = 70;  Serial.println("LANDING");  break;
			case 'f':  steering_data[0] = 0;  Serial.println("Throttle Off");  break;

			case 'r':
				Serial.println("//reseting//, MIN");   reset = true;
				break;
				break;

			case 'p':
				Serial.println("packet status:");
				for (int j = 0; j < 32; j++)
				{
					Serial.print(packet[j]);
					Serial.print(" , ");
				}
				Serial.println(" .. ");
				break;
			}
			cmd = 0;

		}

		timeout = process_SymaX(radio, steering_data);

		// wait before dealing & sending next packet
		while (micros() < timeout) {};
	}
}

void set_txid(bool renew)
{
    uint8_t i;
    for(i=0; i<4; i++)
        transmitterID[i] = EEPROM.read(ee_TXID0+i);
    if(renew || (transmitterID[0]==0xFF && transmitterID[1]==0x0FF)) {
        for(i=0; i<4; i++) {
            transmitterID[i] = random() & 0xFF;
            EEPROM.update(ee_TXID0+i, transmitterID[i]); 
        }            
    }
}

void selectProtocol() 
{
    // protocol selection
   current_protocol= PROTO_SYMAX5C1 ; 
   EEPROM.update(ee_PROTOCOL_ID, current_protocol);
}

void init_protocol(RF24 NRF24L01)
{   
	Symax_init(NRF24L01);

	///SymaX_bind();
    Serial.println("done init & bind protocol");

}

void toggle_thrust(RF24 radio)
{

	uint8_t timeout = 0;

	for (int i = 0; i < 20; i++)
	{
		static uint8_t       steering[5] = { 0xFF, 0x00, 0x00, 0x00 };
		timeout = process_SymaX(radio, steering);
			
		// wait before dealing & sending next packet
		while (micros() < timeout) {};

	}
	Serial.println("THRUST TOGGLE HIGH");
	for (int i = 0; i < 20; i++)
	{
		static uint8_t       steering[5] = { 150, 0x00, 0x00, 0x00 };
		timeout = process_SymaX(radio, steering);
			
		// wait before dealing & sending next packet
		while (micros() < timeout) {};
			

	}
	Serial.println("THRUST TOGGLE LOW");
	Serial.println("THRUST ON");
	return;
}
