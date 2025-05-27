/**
  @AUTHOR Pluimvee
 
  based on the work of
  https://github.com/rhmswink/Resideo-R200C2-ESPHome-Mod
  https://github.com/RobTillaart/CHT8305/tree/master
  
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf
 */
#include <Arduino.h>
#include "Sensors.h"

#define PIN_SDA 9 // GPIO9
#define PIN_SCL 10 // GPIO10

#define RXD0 20 // GPIO20
#define TXD0 21 // GPIO21

static volatile byte device_address;
static volatile byte device_register[256];
static volatile byte device_register_ptr;

static volatile bool i2cIdle = true;  // true if the I2C BUS is idle
static volatile int bitIdx  = 0;      // the bitindex within the frame
static volatile int byteIdx = 0;      // nr of bytes within tis frame
static volatile byte data = 0;        // the byte under construction, which will persited in device_register when acknowledged
static volatile bool writing = true;  // is the master reading or writing to the device

////////////////////////////
//#define LOG_REMOTE
#define LOG_LEVEL 3
#include <Logging.h>

////////////////////////////
//// Interrupt handlers
/////////////////////////////

// Rising SCL makes reading the SDA
void IRAM_ATTR i2cTriggerOnRaisingSCL() 
{
  if (i2cIdle)    // we didnt get a start signal yet
    return;

	//get the value from SDA
	int sda = digitalRead(PIN_SDA);

	//decide where we are and what to do with incoming data
	int i2cCase = 0;    // data bit

	if (bitIdx == 8)    // we are already at 8 bits, so this is the (N)ACK bit
		i2cCase = 1; 

	if (bitIdx == 7 && byteIdx == 0 ) // first byte is 7bits address, 8th bit is R/W
		i2cCase = 2;

 	bitIdx++; // we found the first bit (out of the switch as its also needed for case 2!)

	switch (i2cCase)
	{
    default:
		case 0: // data bit
      data = (data << 1) | (sda>0?1:0);
	  	break;//end of case 0 general

		case 1: //(N)ACK
      switch (byteIdx)
      {
        case 0:
          if (sda == 0) // SDA LOW ->  ACK 
            device_address = data;
          break;
        case 1:
          if (writing && sda == 0) {  // if the master is writing, the first byte is the address in the device registers
            device_register_ptr = data;
            break;
          }
        // fall thru
        default:
          // it seems that while reading the master signals the slave to stop sending by giving a nack
          // this last byte still needs to be stored in the register
          // so we ignore here if it was a ack or nack
          //if (sda ==0)
          device_register[device_register_ptr++] = data;
          break;
      }
			byteIdx++;  // next byte
      data = 0;   // reset this data byte
			bitIdx = 0; // start with bit 0
  		break;

		case 2:
      writing = (sda == 0);  // if SDA is LOW, the master wants to write 
  		break;
	}
}

/**
 * This is for recognizing I2C START and STOP
 * This is called when the SDA line is changing
 * It is decided inside the function wheather it is a rising or falling change.
 * If SCL is on High then the falling change is a START and the rising is a STOP.
 * If SCL is LOW, then this is the action to set a data bit, so nothing to do.
 */
void IRAM_ATTR i2cTriggerOnChangeSDA()
{
  if (digitalRead(PIN_SCL) == 0)  // if SCL is low we are still communicating
    return;

	if (digitalRead(PIN_SDA) > 0) //RISING if SDA is HIGH (1) -> STOP 
		i2cIdle = true;
	else //FALLING if SDA is LOW -> START?
	{
		if (i2cIdle) //If we are idle than this is a START
		{
			bitIdx  = 0;
			byteIdx = 0;
      data = 0;
      device_register_ptr = 0;
			i2cIdle = false;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CHT8305::setup() 
{
	//Define pins for SCL, SDA
  pinMode(PIN_SCL, INPUT_PULLUP);   
  pinMode(PIN_SDA, INPUT_PULLUP);

  //reset variables
  memset((void *) device_register, sizeof(device_register), 0);
	i2cIdle = true;

  //Atach interrupt handlers to the interrupts on GPIOs
  attachInterrupt(PIN_SCL, i2cTriggerOnRaisingSCL, RISING); //trigger for reading data from SDA
  attachInterrupt(PIN_SDA, i2cTriggerOnChangeSDA,  CHANGE); //for I2C START and STOP

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
float CHT8305::temperature() { 
  float T = (device_register[0] <<8 | device_register[1]);
  T = (T * 165.0 / 65535.0) - 40.0;
  T -= 1.4;   // as seen on display

  return T;
}

float CHT8305::humidity() { 
  float H = (device_register[2] <<8 | device_register[3]);
  H = (H * 100.0 / 65535.0);
  H += 2.0;   // as seen on display
 
  return H;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// The expected response consists of 8 bytes
// | 0    | 1   | 2   | 3     | 4     | 5     | 6     | 7  |
// | HEAD | LEN | CMD | DATA1 | DATA2 | DATA3 | DATA4 | CS |
//
// PPM = DATA1 << 8 | DATA2
// DATA3  b7    b6      b5      b4          b3            b2            b1            b0
//      1 na    drift   aging   non-cali    below_range   above_range   sensor_error  pre-heating
//      0 na    normal  normal  calibrated  normal        normal        normal
//  DATA4 reserved
//
#define NUM_MSG_BYTES 8
uint16_t cached_ppm; // cached value

// Checksum: 256-(HEAD+LEN+CMD+DATA)%256
uint8_t calcCRC(uint8_t* response, size_t len) 
{
  uint8_t crc = 0;
  // last byte of response is checksum, don't calculate it
  for (int i = 0; i < len - 1; i++) {
      crc -= response[i];
  }
  return crc;
}

void getCO2PPM() 
{
    uint8_t response[NUM_MSG_BYTES] = {0};

    // All read responses start with 0x16
    // The payload length for the Co2 message is 0x05
    // The command for the Co2 message is 0x01
    uint8_t expectedHeader[] = {0x16, 0x05, 0x01};
    int currentPos = 0;

    int availableBytes = Serial0.available();

    if (availableBytes < NUM_MSG_BYTES) {   // can there be a complete message available?
        return;
    }

    // We are only interested in the last message, drop all others
    while (availableBytes >= (2*NUM_MSG_BYTES)) {
        Serial0.readBytes(response, NUM_MSG_BYTES);
        availableBytes = Serial0.available();
    }

    // Find the expected header
    while (currentPos < sizeof(expectedHeader)) {
        if (Serial0.available()) {
            Serial0.readBytes(response+currentPos, 1);
        } else {
            return;
        }
        
        if (response[currentPos] == expectedHeader[currentPos]) {
            currentPos++;
        }
    }

    // If present, read the data and checksum
    if (Serial0.available() >= NUM_MSG_BYTES - sizeof(expectedHeader)) {
        Serial0.readBytes(response+currentPos, NUM_MSG_BYTES - sizeof(expectedHeader));
    } else {
        ERROR("The last message in the buffer was not complete");
        return;
    }

    // Process the Co2 value and checksum
    uint8_t checksum = calcCRC(response, sizeof(response));
    int16_t ppm = response[3] << 8 | response[4];
    if (response[7] == checksum) {
        INFO("CM1106 Received CO2=%uppm DF3=%02X DF4=%02X", ppm, response[5], response[6]);
        cached_ppm = ppm;
    } else {
        ERROR("Got wrong UART checksum: 0x%02X - Calculated: 0x%02X, ppm data: %u", response[7], checksum, ppm);
        return;
    }
}

bool CM1106::setup() 
{
	Serial0.begin(9600, SERIAL_8N1, RXD0, TXD0);
  cached_ppm = 0;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t CM1106::ppm() 
{
  getCO2PPM();
  return cached_ppm;
}
