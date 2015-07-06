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
#define SERVO_PIN 4
#define DOOR_PIN 6
#define BUTTON_PIN 5
#define BUZZER_PIN 3
#define LED_PIN 7

#define LOCK_DELAY 7000

#define SS_PIN 8
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool setLock = true, isLocked = false, closed = false;

void setup() 
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  //Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

void beep(int pin, int nTimes)
{
  for (int i = 0; i < nTimes; i++)
  {
    digitalWrite(pin, HIGH);
    delay(10);
    digitalWrite(pin, LOW);
    delay(10);
  }
}

int counter = 0;
unsigned long tOld, tNew, elapsed;
void loop() 
{
//  getID();
//  return;
  tOld = tNew;
  tNew = millis();
  elapsed = tNew - tOld;

  closed = !digitalRead(DOOR_PIN);

  bool pressed = bPressed();
  
  if (isLocked)
  {
    unsigned long cardID = getID();
    bool goodCard = cardID != 0 && findID(cardID) != -1;
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
  
  if (!setLock)
    counter += elapsed;
    
  if (counter >= LOCK_DELAY)
  {
    setLock = true;
    counter = 0;
  }
  
  if (!setLock && isLocked)
    unlock();
  if (setLock && !isLocked && closed)
    lock();
    
  if (!closed) digitalWrite(BUZZER_PIN, LOW);
}

void addRemoveCard()
{
  for (int i = 0; i < 30; i++)
  {
    digitalWrite(BUZZER_PIN,HIGH);
    delay(10);
    digitalWrite(BUZZER_PIN,LOW);
    delay(240);

    unsigned long cardID;
    if ((cardID = getID()) != 0)
    {
      int index;

      if ((index = findID(cardID)) == -1)
      {

        int count = EEPROM.read(0);
        writeID(cardID, count);
        EEPROM.write(0, count + 1);

        for (int j = 0; j < 4; j++) 
        {
          delay(10);
          digitalWrite(BUZZER_PIN, HIGH);
          delay(10);
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
        }

        return;
      }
      else
      {
        int count = EEPROM.read(0);
        unsigned long toMove = readID(count - 1);
        writeID(toMove, index);
        EEPROM.write(0, count - 1);

        for (int j = 0; j < 2; j++) 
        {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(400);
          digitalWrite(BUZZER_PIN, LOW);
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

unsigned long getID() 
{
  beep(LED_PIN,1);
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    //Serial.println("no card present");
    return 0;
  }
  beep(BUZZER_PIN,1);
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    //Serial.println("read failed");
    return 0;
  }
  unsigned long cardID = 0;
  for (int i = 0; i < 4; i++) 
  {
    cardID = cardID << 8;
    cardID += mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA();
  beep(BUZZER_PIN,1);
  return cardID;
}

void printID(unsigned long id)
{
  //Serial.print(id,HEX);
  //Serial.println("");
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
  bsNew = digitalRead(BUTTON_PIN);
  if (bsNew == 0 && bsOld == 1)
  {
    return true;
  }
  return false;
}

void unlock()
{
  ser.attach(SERVO_PIN);
  delay(10);
  ser.write(175);
  delay(1000);
  ser.detach();
  digitalWrite(BUZZER_PIN, HIGH);
  isLocked = false;
}

void lock()
{
  digitalWrite(BUZZER_PIN, LOW);
  ser.attach(SERVO_PIN);
  delay(10);
  ser.write(30);
  delay(1000);
  ser.detach();
  isLocked = true;
}
