
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>


#define SYMAX_BIND_COUNT	345
#define SYMAX_PACKET_PERIOD 3900
#define SYMAX_PACKET_SIZE	10
#define SYMAX_RF_CHANNELS	4

// flags going to packet[4]
#define SYMAX_FLAG_PICTURE  0x40
#define SYMAX_FLAG_VIDEO    0x80
// flags going to packet[6]
#define SYMAX_FLAG_FLIP     0x40
// flags going to packet[7]
#define SYMAX_FLAG_HEADLESS 0x20

static uint32_t SYMAX_packet_counter;
static uint8_t  SYMAX_rx_tx_addr[5];
static uint8_t  SYMAX_current_chan;
uint8_t SYMAX_chans[] = { 0x3A, 0x46, 0x37, 0x43 };
uint8_t packet[32];
uint8_t status_ans;



uint32_t process_SymaX(RF24 NRF24L01, uint8_t steering_data[]);
void Symax_init(RF24 NRF24L01);
void SymaX_bind(RF24 NRF24L01);
static uint8_t SymaX_checksum(uint8_t *data);
static void SYMAX_set_channels(u8 address);
static void SYMAX_build_packet(u8 bind, uint8_t throttle, uint8_t yaw, uint8_t pitch, uint8_t roll);
static void SYMAX_send_packet(uint8_t bind, RF24 NRF24L01, uint8_t steering_data[]);

uint32_t process_SymaX(RF24 NRF24L01, uint8_t steering_data[])
{
    uint32_t nextPacket = micros() + SYMAX_PACKET_PERIOD;
    SYMAX_send_packet(false, NRF24L01, steering_data);
    return nextPacket;
}

void Symax_init(RF24 NRF24L01)
{
    const uint8_t bind_rx_tx_addr[] = {0xab,0xac,0xad,0xae,0xaf};
    uint8_t first_packet[] = {0x20, 0xA1, 0x20, 0xBD, 0x1F, 0x7A, 0x20, 0xF9, 0x00, 0x00};  
    SYMAX_packet_counter = 0;
	
    
    status_ans = NRF24L01.read_register(NRF_STATUS);
	Serial.print("STATUS REG: ");   Serial.println(status_ans, HEX);
    NRF24L01.write_register(NRF_CONFIG, _BV(EN_CRC) | _BV(CRCO));
    NRF24L01.write_register(EN_AA, 0x00);      // No Auto Acknowledgment
    NRF24L01.write_register(EN_RXADDR, 0x00);  // Enable all data pipes (even though not used?)
    NRF24L01.write_register(SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01.write_register(SETUP_RETR, 0x00); //ff  // 4mS retransmit t/o, 15 tries (retries w/o AA?)
    NRF24L01.write_register(RF_CH, 0x10);
	status_ans = NRF24L01.read_register(RF_CH);
	Serial.print("RF_CH REG: ");   Serial.println(status_ans, HEX);
	NRF24L01.write_register(RF_SETUP, 0x22);
	status_ans = NRF24L01.read_register(RF_SETUP);
	Serial.print("RF_SETUP REG: ");   Serial.println(status_ans, HEX);
	NRF24L01.write_register(NRF_CONFIG, 0x4E);

    //NRF24L01_SetBitrate(NRF24L01_BR_250K);
    //NRF24L01_SetPower(RF_POWER);
    NRF24L01.write_register(NRF_STATUS	, 0x77);     // Clear data ready, data sent, and retransmit
    NRF24L01.write_register(OBSERVE_TX, 0x00);
    NRF24L01.write_register(CD, 0x00);
	NRF24L01.write_register(EN_AA, 0x00);


	status_ans = NRF24L01.read_register(FEATURE);
	Serial.print("FEATURE REG: ");   Serial.println(status_ans, HEX);

	NRF24L01.toggle_features(0x73);

	status_ans = NRF24L01.read_register(NRF_STATUS);  //RAN addition
	Serial.print("STATUS REG: ");   Serial.println(status_ans, HEX);

    NRF24L01.write_register(TX_ADDR, bind_rx_tx_addr, sizeof(bind_rx_tx_addr));
	uint8_t buf[] = {0x00,0x00,0x00,0x00,0x00};
	status_ans = NRF24L01.read_register(TX_ADDR, buf, 5);  //RAN addition
  Serial.print("TX ADDR REG: ");
  Serial.print(buf[0], HEX);
  Serial.print(buf[1], HEX);
  Serial.print(buf[2], HEX);
  Serial.print(buf[3], HEX);
  Serial.println(buf[4], HEX);
	NRF24L01.flush_tx();
    /*

    NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01.write_register(NRF24L01_07_STATUS	, 0x0e);
    NRF24L01_ReadReg(NRF24L01_00_CONFIG);
    NRF24L01.write_register(NRF24L01_00_CONFIG	, 0x0c);
    NRF24L01.write_register(NRF24L01_00_CONFIG	, 0x0e);  // power on*/

	delay(1);
    NRF24L01.write_register(RF_CH, 0x09);
	status_ans = NRF24L01.read_register(RF_CH);
	Serial.print("RF_CH REG: ");   Serial.println(status_ans, HEX);
	NRF24L01.setPayloadSize(SYMAX_PACKET_SIZE);
    NRF24L01.write(first_packet, sizeof(first_packet));
    SYMAX_packet_counter	= 0;
    delay(12);


	///Init 2


	uint8_t chans_bind[] = { 0x20, 0x30, 0x40, 0x09 };
	uint8_t pairing_packet1[] = { 0xA1, 0x0A, 0x20, 0x81, 0xBA, 0xAA, 0xAA, 0xBB, 0xB1, 0x0F };
	uint8_t pairing_packet2[] = { 0x32, 0x38, 0x3E, 0x44, 0x35, 0x3B, 0x41, 0x47, 0xB1, 0x1E };
	NRF24L01.setPayloadSize(SYMAX_PACKET_SIZE);

	for (int h = 0; h < 10; h++)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 2; j++)
			{

				NRF24L01.setChannel(chans_bind[i]);
					NRF24L01.write(pairing_packet1, sizeof(pairing_packet1));
					delay(4);
			}
		}

		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				NRF24L01.setChannel(chans_bind[i]);
				NRF24L01.write(pairing_packet2, sizeof(pairing_packet2));
				delay(4);
			}
		}
	}

	

	///Init 3

	const uint8_t bind_rx_tx_addr2[] = { 0xBA,0x81,0x20,0x0A,0xA1 };

	uint8_t data_packet[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x4, 0x00, 0x01, 0x00, 0x98 };
	uint8_t data_packet2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x4, 0x00, 0x01, 0x00, 0x03 };

	status_ans = NRF24L01.read_register(NRF_STATUS);
	Serial.print("STATUS REG: ");   Serial.println(status_ans, HEX);

	NRF24L01.write_register(CD, 0x00);
	NRF24L01.write_register(OBSERVE_TX, 0x00);
	NRF24L01.write_register(NRF_STATUS, 0x77);
	status_ans = NRF24L01.read_register(NRF_STATUS);
	Serial.print("STATUS REG: ");   Serial.println(status_ans, HEX);
	NRF24L01.write_register(RF_SETUP, 0x27);
	NRF24L01.write_register(RF_CH, 0x10);
	NRF24L01.write_register(SETUP_RETR, 0x00);
	NRF24L01.write_register(SETUP_AW, 0x03);
	NRF24L01.write_register(EN_RXADDR, 0x00);
	NRF24L01.write_register(EN_AA, 0x00);
	NRF24L01.write_register(NRF_CONFIG, 0x4E);
	status_ans = NRF24L01.read_register(NRF_CONFIG);
	Serial.print("CONFIG REG: ");   Serial.println(status_ans, HEX);

	status_ans = NRF24L01.read_register(FEATURE);
	Serial.print("FEATURE REG: ");   Serial.println(status_ans, HEX);

	NRF24L01.toggle_features(0x73);

	status_ans = NRF24L01.read_register(NRF_STATUS);
	Serial.print("STATUS REG: ");   Serial.println(status_ans, HEX);

	NRF24L01.write_register(TX_ADDR, bind_rx_tx_addr2, sizeof(bind_rx_tx_addr2));

	NRF24L01.write_register(RF_CH, 0x37);
	status_ans = NRF24L01.read_register(RF_CH);
	Serial.print("RF_CH REG: ");   Serial.println(status_ans, HEX);


	for (int i = 0; i < 3; i++)
	{
		for (int i = 0; i < 9; i++)
		{
			NRF24L01.write(data_packet, sizeof(data_packet));
			delay(4);
		}

		NRF24L01.write(data_packet, sizeof(data_packet2));
		delay(4);
	}


}


void SymaX_bind(RF24 NRF24L01)
{
    uint16_t bind_counter = SYMAX_BIND_COUNT;
	uint8_t steering_data[4] = { 0, 0, 0, 0 };
    while(bind_counter) {
		SYMAX_send_packet(true, NRF24L01, steering_data);
        delayMicroseconds(SYMAX_PACKET_PERIOD);
    }
    SYMAX_set_channels(SYMAX_rx_tx_addr[0]);
    NRF24L01.write_register(TX_ADDR, SYMAX_rx_tx_addr, 5);
    SYMAX_current_chan		= 0;
    SYMAX_packet_counter	= 0;
}

static uint8_t SymaX_checksum(uint8_t *data)
{
    uint8_t sum = data[0];
    for (uint8_t i=1; i < SYMAX_PACKET_SIZE-1; i++)
        sum ^= data[i];
    return sum + 0x55;
}

// channels determined by last byte of tx address
static void SYMAX_set_channels(u8 address) {
    static const uint8_t start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
    static const uint8_t start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
	static const uint8_t start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};
	/*uint8_t laddress = address & 0x1f;
    uint8_t i;
    uint32_t *pchans = (uint32_t *)SYMAX_chans;
    if (laddress < 0x10) {
        if (laddress == 6) laddress = 7;
        for(i=0; i < SYMAX_RF_CHANNELS; i++) {
            SYMAX_chans[i] = start_chans_1[i] + laddress;
        }
    } else if (laddress < 0x18) {
        for(i=0; i < SYMAX_RF_CHANNELS; i++) {
            SYMAX_chans[i] = start_chans_2[i] + (laddress & 0x07);
        }
        if (laddress == 0x16) {
            SYMAX_chans[0] += 1;
            SYMAX_chans[1] += 1;
        }
    } else if (laddress < 0x1e) {
        for(i=0; i < SYMAX_RF_CHANNELS; i++) {
            SYMAX_chans[i] = start_chans_3[i] + (laddress & 0x07);
        }
    } else if (laddress == 0x1e) {
        *pchans = 0x38184121;
    } else {
        *pchans = 0x39194121;
    } */
}

static void SYMAX_build_packet(u8 bind, uint8_t throttle, uint8_t yaw, uint8_t pitch, uint8_t roll) {
    if (bind) {
        packet[0] = SYMAX_rx_tx_addr[4];
        packet[1] = SYMAX_rx_tx_addr[3];
        packet[2] = SYMAX_rx_tx_addr[2];
        packet[3] = SYMAX_rx_tx_addr[1];
        packet[4] = SYMAX_rx_tx_addr[0];
        packet[5] = 0xaa;
        packet[6] = 0xaa;
        packet[7] = 0xaa;
        packet[8] = 0x00;
    } else {
        packet[0] = throttle; 
		/// my change
		if (yaw > 127)
		{
			packet[1]  = yaw ^ 0b00000000;
		}
		else
		{
			packet[1] = yaw ^ 0b01111111;
		}
		
		if (pitch > 127)
		{
			packet[2] = pitch ^ 0b00000000;
		}
		else
		{
			packet[2] = pitch ^ 0b01111111;
		}

		if (roll > 127)
		{
			packet[3] = roll ^ 0b00000000;
		}
		else
		{
			packet[3] = roll ^ 0b01111111;
		}

		///
        packet[4] = 0x00; 
        // use trims to extend controls
        packet[5] = 0x42;  // always high rates (bit 7 is rate control)
        packet[6] = 0x00;
		///
        packet[7] = 0x00;
        packet[8] = 0x01;
    }
    packet[9] = SymaX_checksum(packet);
}

static void SYMAX_send_packet(uint8_t bind, RF24 NRF24L01, uint8_t steering_data[])
{
	
    SYMAX_build_packet(bind, steering_data[0], steering_data[1], steering_data[2], steering_data[3]);
    NRF24L01.write_register(NRF_STATUS, 0x70);
    NRF24L01.write_register(RF_CH, SYMAX_chans[SYMAX_current_chan]);
	//Serial.println(SYMAX_chans[SYMAX_current_chan], HEX);
	NRF24L01.setPayloadSize(SYMAX_PACKET_SIZE);
    NRF24L01.write(packet, sizeof(packet));
    if (SYMAX_packet_counter%2 == 1) 
	{   // use each channel twice
		if (SYMAX_current_chan == 3)
		{
			SYMAX_current_chan = 0;
		}
		else 
		{
			SYMAX_current_chan++;
		}
    }

	SYMAX_packet_counter++;
}