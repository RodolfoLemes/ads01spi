/*

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


void setup() {

  Serial.begin(9600); // set up serial comm to PC at this baud rate
  
  lcd.begin(16, 2);
  lcd.print("quero um abraço");
  lcd.setCursor(0, 1);
  
  pinMode(VSELECT, INPUT);
  pinMode(SLAVESELECT, OUTPUT);
  digitalWriteFast(SLAVESELECT,LOW); // take the SS pin low to select the chip
  delayMicroseconds(1);
  digitalWriteFast(SLAVESELECT,HIGH); // take the SS pin high to start new ADC conversion

  //SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  for (int i=0;i<2;i++) { // throw away the first few readings, which seem to be way off
    SpiRead();
  }
} // end setup()

// =============================================================================
// Main Loop:
// acquire 'nsamples' readings, convert to units of volts, and send out on serial port

void loop() {
  
  SpiRead(); /// ORIGINAL DATA?
  
  //Serial.println(uVolts,7); //original code included...Serial.print(uVolts,6);
  /* lcd.clear();
  lcd.home();
  lcd.print("VOLTAGEM");
  lcd.setCursor(0, 1);
  lcd.print(uVolts, digitalReadFast(VSELECT) ? 5 : 7); */
} // end main loop


// =================================================================
// SpiRead() -- read 2 bytes
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
  
  float GAIN = 2*(digitalReadFast(VSELECT) ? 20 : 4);      
  // Set GAIN to 1 to start - this varible is the ADC
  // multiplacation factor of the ADC output number
  
  float OFFSET = 0; //Offset of the output from zero
  
  digitalWriteFast(SLAVESELECT,LOW); // take the SS pin low to select the chip
  //delayMicroseconds(1); // probably not needed, only need 25 nsec delay
  
  b = SPI.transfer(0); // B3
  sig = (b & 0x10) == 0x10;
  b = b & 0xf;
  result = b;

  b = SPI.transfer(0); // B3
  result = result << 8;

  result |= b;
  if (sig) {
    result = result - 1;
    result = ~result;
    result = result & 0b111111111111;
    result = result*(-1);
  }
  
  digitalWriteFast(SLAVESELECT,HIGH); // take the SS pin high to bring MISO to hi-Z and start new conversion
  
  //if (sig) result |= 0xf0000000; // if input is negative, insert sign bit (0xf0.. or 0xe0... ?)
  v = result;
  v = (v * 5 / 4096)*2; // scale result down , last 4 bits are "sub-LSBs"
  
  /*
  THE LINES BELOW ALLOW CALIBRATION OF THE ADC SERIAL OUTPUT VALUE
  
  Change GAIN or OFFSET of the ADC OUTPUT for calibration of your source voltage. The line below uses
  the equation -> v = (GAIN) * (v) + (OFFSET). This is the straight line graph equation "y=ax+b".
  
  The initial value of the GAIN is "1" and OFFSET is set to zero. For a non-linear source voltage output change the equation
  to fit your source equation.
  
  */
  Serial.println(v);
  return(v);

}//END CODE HERE