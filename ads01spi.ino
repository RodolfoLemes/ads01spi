/*

BEALE CODE

Interface between Arduino DM board and Linear Tech LTC2440 24-bit ADC
Oct. 24 2012 John Beale

LTC2440 <----------> Arduino
10: /EXT : ground (use external serial clock)
11: /CS <- to digital pin 10 (SS pin)
12: MISO -> to digital pin 12 (MISO pin)
13: SCLK <- to digital pin 13 (SCK pin)
15: BUSY -> to digital pin 9 (low when result ready)

1,8,9,16: GND : all grounds must be connected
2: Vcc : +5V supply
3: REF+ : +5.0V reference
4: REF- : GND
5: IN+ : Input+
6: IN- : Input-
7: SDI : wire ADC Pin 7 high (+5 VDC) for a 6.9 Hz output rate, or low (GND) for 880 samples per second.

NOTE: I strongly recommend wiring Pin 7 to +5 VDC to start - this will give you a very slow but, easy to work with, sample rate. "nsamples" is set to one in the code below. This will give you a 6.9 sample per second output rate.

14: Fo : GND (select internal 9 MHz oscillator)

*/

#include <SPI.h> // include the SPI library
#include <digitalWriteFast.h> // from http://code.google.com/p/digitalwritefast/downloads/list
#include <LiquidCrystal.h>
// I had to update the WriteFast library for Arduino 1.0.1: replace "#include wiring.h" and "#include WProgram.h"
// with "#include <Arduino>" in libraries/digitalwritefast/digitalWriteFast.h

#define VREF (5.0) // ADC voltage reference
#define PWAIT (19) // milliseconds delay between readings
#define SLAVESELECT 10 // digital pin 10 for CS/
#define BUSYPIN 9 // digital pin 9 for BUSY or READY/
#define VSELECT 8 // Voltage reference selected

// LiquidCrystal
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

const int nsamples = 1; // how many ADC readings to average together or CPS setting. nsamples =  10 yeilds (880/10) or 110 samples per second.

// SPI_CLOCK_DIV16 gives me a 1.0 MHz SPI clock, with 16 MHz crystal on Arduino

void setup() {

  Serial.begin(9600); // set up serial comm to PC at this baud rate
  
	lcd.begin(16, 2);
	lcd.print("quero um abraço");
	lcd.setCursor(0, 1);
	
	pinMode(VSELECT, INPUT);
  pinMode(SLAVESELECT, OUTPUT);
  pinMode(BUSYPIN, INPUT);
  digitalWriteFast(SLAVESELECT,LOW); // take the SS pin low to select the chip
  delayMicroseconds(1);
  digitalWriteFast(SLAVESELECT,HIGH); // take the SS pin high to start new ADC conversion
  
  SPI.begin(); // initialize SPI, covering MOSI,MISO,SCK signals
  SPI.setBitOrder(MSBFIRST); // data is clocked in MSB first
  SPI.setDataMode(SPI_MODE0); // SCLK idle low (CPOL=0), MOSI read on rising edge (CPHI=0)
  SPI.setClockDivider(SPI_CLOCK_DIV16); // set SPI clock at 1 MHz. Arduino xtal = 16 MHz, LTC2440 max = 20 MHz
  
  for (int i=0;i<2;i++) { // throw away the first few readings, which seem to be way off
    SpiRead();
  }
} // end setup()

// =============================================================================
// Main Loop:
// acquire 'nsamples' readings, convert to units of volts, and send out on serial port

void loop() {
	
  int i;
  float uVolts; // average reading in microvolts
  float datSum; // accumulated sum of input values
  float sMax;
  float sMin;
  long n; // count of how many readings so far
  float x,mean,delta,sumsq;
  
  sMax = -VREF; // set max to minimum possible reading
  sMin = VREF; // set min to max possible reading
  sumsq = 0; // initialize running squared sum of differences
  n = 0; // have not made any ADC readings yet
  datSum = 0; // accumulated sum of readings starts at zero

  for (i=0; i<nsamples; i++) {
    x = SpiRead(); /// ORIGINAL DATA?
    //Serial.println(x, 7);
    datSum += x;
    if (x > sMax) sMax = x;
    if (x < sMin) sMin = x;
    n++;
    delay(198);
  } // end for (i..)
  
  uVolts = datSum / n; //Average over N modified original-works
  
  Serial.println(uVolts,7); //original code included...Serial.print(uVolts,6);
	lcd.clear();
	lcd.home();
	lcd.print("VOLTAGEM");
	lcd.setCursor(0, 1);
	lcd.print(uVolts, digitalReadFast(VSELECT) ? 5 : 7);
} // end main loop

// =================================================================
// SpiRead() -- read 4 bytes from LTC2440 chip via SPI, return Volts
// =================================================================

float SpiRead(void) {

  long result = 0;
  byte sig = 0; // sign bit
  byte b;
  float v;
  
  /*
  
  (ADC) "Gain" is innitally set at 1.0 and offset is at 0.0. Adjust these values as needed. A typical value
  for gain with a 1 uV souce voltage  and  an op amp gain of 1000 would be GAIN = 1000.
  
  These "word" equations might help you to get started:
  
  ADC OUTPUT READING = (SOURCE VOLTS) * (OP AMP GAIN) * (ADC GAIN)  [  = 1.0 in example above ]
  
  OP AMP GAIN = (ADC OUTPUT READING) / [ (ADC GAIN) * (SOURCE VOLTS) ]
  
  SOURCE VOLTS =  ADC OUTPUT READING / [ (OP AMP GAIN) * (ADC GAIN ]
  
  ADC input voltage = (SOURCE VOLTS) * (OP AMP GAIN)  >>> The max safe neg #
  of this expression is - 0.250 Volts
  
  */
  
  float GAIN = 2*(digitalReadFast(VSELECT) ? 20.60784314 : 4.96240602);      
	// Set GAIN to 1 to start - this varible is the ADC
  // multiplacation factor of the ADC output number
  
  float OFFSET = 0; //Offset of the output from zero
  
  while (digitalReadFast(BUSYPIN)==HIGH) {} // wait until ADC result is ready
  
  digitalWriteFast(SLAVESELECT,LOW); // take the SS pin low to select the chip
  delayMicroseconds(1); // probably not needed, only need 25 nsec delay
  
  b = SPI.transfer(0xff); // B3
  if ((b & 0x20) ==0) sig=1; // is input negative ?
  b &=0x1f; // discard bits 25..31
  result = b; //org code..."result = b";
  result <<= 8;
  b = SPI.transfer(0xff); // B2
  result |= b;
  result = result<<8;
  b = SPI.transfer(0xff); // B1
  result |= b;
  result = result<<8;
  b = SPI.transfer(0xff); // B0
  result |= b;
  
  digitalWriteFast(SLAVESELECT,HIGH); // take the SS pin high to bring MISO to hi-Z and start new conversion

  //Serial.println(result);
  //Serial.println(sig);
  
  if (sig) result |= 0xf0000000; // if input is negative, insert sign bit (0xf0.. or 0xe0... ?)
  v = result;
  v = v / 16.0; // scale result down , last 4 bits are "sub-LSBs"
  v = v * VREF / (2*(16777216)); // ORIGINAL CODE: +Vfullscale = +Vref/2, max scale (2^24 = 16777216)
  
  /*
  THE LINES BELOW ALLOW CALIBRATION OF THE ADC SERIAL OUTPUT VALUE
  
  Change GAIN or OFFSET of the ADC OUTPUT for calibration of your source voltage. The line below uses
  the equation -> v = (GAIN) * (v) + (OFFSET). This is the straight line graph equation "y=ax+b".
  
  The initial value of the GAIN is "1" and OFFSET is set to zero. For a non-linear source voltage output change the equation
  to fit your source equation.
  
  */
  
  v=v*GAIN + OFFSET;
  return(v);

}//END CODE HERE
