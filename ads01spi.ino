/* 
 * ADS01SPI
 * Acquisition and interpretation of signals with ADC Sigma-Delta and communication protocol SPI
 * 
 * Authors:
 * Rodolfo Lemes Saraiva
 * Leonardo Garbuggio Armelin
 * 
 * Project made for the discipline "Projeto de Microcontroladores"
 * State University of Maringá (UEM)
 * Professor: Rubens Sakiyama
 * 
 * -----------------------------------------------------------
 * ADC AD7322
 * 13 - SCLK
 * 12 - MISO / DIN
 * 11 - MOSI / DOUT
 * 9 - SS / CS
 */ 

#include <SPI.h> // include the SPI library
#include <digitalWriteFast.h> // from http://code.google.com/p/digitalwritefast/downloads/list

#define SLAVESELECT 9

// Variables
long result = 0;
byte sig = 0; // sign bit
byte d, e;
float v = 0;
byte samples[1700]; // 850 samples
short cont = 0;
short serialCont = 0;

// Timer1 function
ISR(TIMER1_COMPA_vect){
  // Set SS/CS to LOW
  digitalWriteFast(SLAVESELECT,LOW);

  // Transfer and receive 2 bytes
  samples[cont] = SPI.transfer(0);
  samples[cont+1] = SPI.transfer(0);
  cont = cont + 2;
  if(cont >= 1700) cont = 0;

  // Set SS/CS to HIGH
  digitalWriteFast(SLAVESELECT,HIGH);
}


void setup() {
  cli(); // stop interrupts
  
  // TIMER 1 for interrupt frequency 150 kHz:
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 105; // = 16000000 / (1 * 150k) - 1 (must be <65536)
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);

  // Serial in 57600 bytes per second
  Serial.begin(57600); // set up serial comm to PC at this baud rate
  pinMode(SLAVESELECT, OUTPUT);

  // SPI in 16 MHz, MSBFIRST, clock down
  SPI.begin();
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  sei(); // allow interrupts
}

void loop() {
  // Receive the stored bytes in buffer samples
  d = samples[serialCont];
  e = samples[serialCont+1];

  // Byte Handling with bitwise operators
  sig = (d & 0x10) == 0x10;
  d = d & 0xf;
  result = d;
  result = result << 8;
  result |= e;
  if (sig) {
    result = result - 1;
    result = ~result;
    result = result & 0b111111111111;
    result = result*(-1);
  }

  // Resulting voltage
  v = result;
  v = (v* 5 / 4096)*8;
  Serial.print(v);
  Serial.print(',');
  //Serial.print(cont);
  //Serial.print(',');
  //Serial.println(serialCont);
  serialCont = serialCont + 2;
  if(serialCont >= 1700) serialCont = 0;
}
