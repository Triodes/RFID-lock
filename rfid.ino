/*
 * MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * The library file MFRC522.h has a wealth of useful info. Please read it.
 * The functions are documented in MFRC522.cpp.
 *
 * Based on code Dr.Leong   ( WWW.B2CQSHOP.COM )
 * Created by Miguel Balboa (circuitito.com), Jan, 2012.
 * Rewritten by Søren Thing Andersen (access.thing.dk), fall of 2013 (Translation to English, refactored, comments, anti collision, cascade levels.)
 * Released into the public domain.
 *
 * Sample program showing how to read data from a PICC using a MFRC522 reader on the Arduino SPI interface.
 *----------------------------------------------------------------------------- empty_skull 
 * Aggiunti pin per arduino Mega
 * add pin configuration for arduino mega
 * http://mac86project.altervista.org/
 ----------------------------------------------------------------------------- Nicola Coppola
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin
 *            Arduino Uno      Arduino Mega      MFRC522 board
 * ------------------------------------------------------------
 * Reset      9                5                 RST
 * SPI SS     10               53                SDA
 * SPI MOSI   11               51                MOSI
 * SPI MISO   12               50                MISO
 * SPI SCK    13               52                SCK
 *
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */

#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

Servo ser;
int servoPin = 3;
int doorPin = 5;
int buttonPin = 8;
int buzzerPin = 6;

int lockDelay = 5000;

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool setLock = true, isLocked = false, closed = true;

unsigned long cardID = 0;

void setup() 
{
  pinMode(buttonPin,INPUT_PULLUP);
  pinMode(doorPin,INPUT_PULLUP);
  pinMode(buzzerPin,OUTPUT);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
}

int counter = 0;
unsigned long tOld, tNew, elapsed;
void loop() 
{
  closed = !digitalRead(doorPin);

  bool pressed = bPressed();
  
  if (isLocked)
  {
    bool goodCard = getID() && findID(cardID) != -1;
    if (pressed || goodCard)
    {
      setLock = false;
    }
  }
  
  if (pressed && !closed)
  {
    addRemoveCard();
    printEEPROM();
  }
  
  tOld = tNew;
  tNew = millis();
  elapsed = tNew - tOld;
  
  if (!setLock)
    counter += elapsed;
    
  if (counter >= lockDelay)
  {
    setLock = true;
    counter = 0;
  }
  
  if (!setLock && isLocked)
    unlock();
  if (setLock && !isLocked && closed)
    lock();
    
  if (!closed) digitalWrite(buzzerPin, LOW);
}

void addRemoveCard()
{
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(buzzerPin,HIGH);
    delay(10);
    digitalWrite(buzzerPin,LOW);
    delay(240);
    if (getID())
    {
      int number;
      if ((number = findID(cardID)) == -1)
      {
        int count = EEPROM.read(0);
        writeID(cardID, count);
        EEPROM.write(0, count + 1);
        for (int j = 0; j < 4; j++) 
        {
          delay(10);
          digitalWrite(buzzerPin, HIGH);
          delay(10);
          digitalWrite(buzzerPin, LOW);
          delay(100);
        }
        return;
      }
      else
      {
        int count = EEPROM.read(0);
        unsigned long toMove = readID(count - 1);
        writeID(toMove, number);
        EEPROM.write(0, count - 1);
        for (int j = 0; j < 2; j++) 
        {
          digitalWrite(buzzerPin, HIGH);
          delay(400);
          digitalWrite(buzzerPin, LOW);
          delay(200);
        }
        return;
      }
    }
  }
}

int findID(unsigned long cardID)
{
  int count = EEPROM.read(0);
  for (int i = 0; i < count; i++)
  {
    unsigned long fromMem = readID(i);
    if (cardID == fromMem)
      return i;
  }
  return -1;
}

bool writeID(unsigned long cardID, int number)
{
  int start = (number) * 4 + 1;
  unsigned long mask = 0xFF000000;
  for ( int j = 0; j < 4; j++ ) 
  { 
    byte toWrite = (cardID & mask) >> 24;
    EEPROM.write( start+j, toWrite);
    cardID = cardID << 8;
  }
  return true;
}

unsigned long readID( int number ) 
{
  int start = (number * 4 ) + 1;
  unsigned long result = 0;
  for ( int i = 0; i < 4; i++ ) 
  {
    result = result << 8;
    result += EEPROM.read(start+i);
  }
  return result;
}

bool getID() 
{
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return false;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return false;
  }
  cardID = 0;
  for (int i = 0; i < 4; i++) 
  {
    cardID = cardID << 8;
    cardID += mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA();
  return true;
}

void printID(unsigned long id)
{
  Serial.print(id,HEX);
  Serial.println("");
}

void printEEPROM()
{
  int count = EEPROM.read(0);
  for (int i = 0; i < count; i++)
  {
    unsigned long fromMem = readID(i);
    printID(fromMem);
  }
}

int bsNew, bsOld;
bool bPressed()
{
  bsOld = bsNew;
  bsNew = digitalRead(buttonPin);
  if (bsNew == 0 && bsOld == 1)
  {
    return true;
  }
  return false;
}

void unlock()
{
  ser.attach(servoPin);
  delay(10);
  ser.write(180);
  delay(600);
  ser.detach();
  digitalWrite(buzzerPin,HIGH);
  isLocked = false;
}

void lock()
{
  digitalWrite(buzzerPin,LOW);
  ser.attach(servoPin);
  delay(10);
  ser.write(0);
  delay(1000);
  ser.detach();
  isLocked = true;
}
