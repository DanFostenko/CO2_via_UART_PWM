#include <SoftwareSerial.h>
#define pwmPin 10
SoftwareSerial swSerial(A0, A1); // RX, TX

void setup() {
  Serial.begin(9600);
  swSerial.begin(9600);
  pinMode(pwmPin, INPUT);

  /*
  Specs https://revspace.nl/MHZ19
  5000 ppm range: 0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB
  */

  // Write command in the 6th and 7th byte
  //           bytes:                         3     4           6     7
  byte setrangeA_cmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB}; // set the range 0 - 5000ppm
  unsigned char setrangeA_response[9]; 
  swSerial.write(setrangeA_cmd,9);
  swSerial.readBytes(setrangeA_response, 9);
  int setrangeA_i;
  byte setrangeA_crc = 0;
  for (setrangeA_i = 1; setrangeA_i < 8; setrangeA_i++) setrangeA_crc+=setrangeA_response[setrangeA_i];
  setrangeA_crc = 255 - setrangeA_crc;
  setrangeA_crc += 1;
  if ( !(setrangeA_response[0] == 0xFF && setrangeA_response[1] == 0x99 && setrangeA_response[8] == setrangeA_crc) ) {
    Serial.println("Range CRC error: " + String(setrangeA_crc) + " / "+ String(setrangeA_response[8]) + " (bytes 6 and 7)");
  } else {
    Serial.println("350 - 450 ppm: Нормальный уровень на открытом воздухе.");
    Serial.println("< 600 ppm: Приемлемые уровни. Уровень. рекомендованный для спален, детских садов и школ.");
    Serial.println("600 - 1000 ppm: Жалобы на несвежий воздух, возможно снижение концентрации внимания.");
    Serial.println("1000 ppm: Максимальный уровень стандартов ASHRAE (American Society of Heating, Refrigerating and Air-Conditioning Engineers) и OSHA (Occupational Safety & Health Administration).");
    Serial.println("1000 - 2500 ppm: Общая вялость, снижение концентрации внимания, возможна головная боль.");
    Serial.println("2500 - 5000 ppm: Возможны нежелательные эффекты на здоровье.");
  }
  Serial.println("------------");
}

void loop() {

  byte measure_cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  unsigned char measure_response[9]; 
  unsigned long th, tl, ppm5000 = 0;

  // get CO2 concentration via UART 
  swSerial.write(measure_cmd,9);
  swSerial.readBytes(measure_response, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=measure_response[i];
  crc = 255 - crc;
  crc += 1;
  if ( !(measure_response[0] == 0xFF && measure_response[1] == 0x86 && measure_response[8] == crc) ) {
    Serial.println("CRC error: " + String(crc) + " / "+ String(measure_response[8]));
  } 
  unsigned int responseHigh = (unsigned int) measure_response[2];
  unsigned int responseLow = (unsigned int) measure_response[3];
  unsigned int ppm = (256*responseHigh) + responseLow;

  // get CO2 concentration via PWM 
  do {
    th = pulseIn(pwmPin, HIGH, 1004000) / 1000;
    tl = 1004 - th;
    ppm5000 =  5000 * (th-2)/(th+tl-4); // calculation for the range 0 - 5000ppm 
  } while (th == 0);

  Serial.print(ppm);
  Serial.println(" ppm (UART)");
  Serial.print(ppm5000);
  Serial.println(" ppm (PWM)");
  Serial.println("------------");
  delay(6000);
}