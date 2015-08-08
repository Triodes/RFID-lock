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

// Import libraries.
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

// Define pin constants.
#define SERVO_PIN 4
#define DOOR_PIN 6
#define BUTTON_PIN 5
#define BUZZER_PIN 3
#define LED_PIN 7

// Define delays for locking.
#define LOCK_DELAY 7000
#define CLOSE_DELAY 2000

// Define SPI Slave Select and Reset pins.
#define SS_PIN 8
#define RST_PIN 9

// Define card reader library instance.
MFRC522 mfrc522(SS_PIN, RST_PIN);

/**
 *  Define state booleans.
 *    setLock:  flag to engage the lock next iteration.
 *    isLocked: flags if the lock is engaged.
 *    closed:   flags if the door is closed.
 */
bool setLock = true, isLocked = false;

void setup() 
{
  // Set pin modes for GPIO pins.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);

  delay(10);

  // Start the SPI bus.
  SPI.begin();
  delay(10);

  // Initialize the card reader.
  mfrc522.PCD_Init();
  delay(10);

  // Flash LED to show that setup is done.
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

// Time measurment variables.
unsigned long tOld, tNew, elapsed;

// Counter for keeping lock open for the amount specified by LOCK_DELAY.
int lockCounter = 0;

void loop() 
{
  // Measure elapsed time since last loop.
  tOld = tNew;
  tNew = millis();
  elapsed = tNew - tOld;

  // Set bool if door is closed.
  bool closed = dClosed();

  // Set bool if button is pressed.
  bool pressed = buttonPressed();
  
  if (isLocked)
  {
    // If the card id is good or the button was pressed: unlock.
    if (goodID(getID()) || pressed)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      setLock = false;
      unlock();
    }
  }

  if (!closed)
  {
  	// If the door is open: disable the buzzer.
  	digitalWrite(BUZZER_PIN, LOW);

		// If a good card is found. go into add/remove mode.
  	if (goodID(getID()))
  		addRemoveCard();
  }
  
  // If the lock flag is unset, start counting
  if (!setLock)
  {
    lockCounter += elapsed;
  }
  
  // If the LOCK_DELAY has expired: set the lock flag.
  if (lockCounter >= LOCK_DELAY)
  {
    setLock = true;
    lockCounter = 0;
  }

  // If the lock flag is set the door is unlocked and the door is closed: lock the door.
  if (setLock && !isLocked && closed)
    lock();
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

bool goodID(unsigned long cardID)
{
	return cardID != 0 && findID(cardID) != -1;
}

// Write a card id to the specified position in the EEPROM memory.
bool writeID(unsigned long cardID, int position)
{
  int start = (position) * 4 + 1;
  unsigned long mask = 0xFF000000;
  for (int j = 0; j < 4; j++) 
  { 
    byte toWrite = (cardID & mask) >> 24;
    EEPROM.write(start+j, toWrite);
    cardID = cardID << 8;
  }
  return true;
}

// Read the card id at the specified position.
unsigned long readID(int position) 
{
  int start = (position * 4 ) + 1;
  unsigned long result = 0;
  for ( int i = 0; i < 4; i++ ) 
  {
    result = result << 8;
    result += EEPROM.read(start+i);
  }
  return result;
}

// Try to read a card id from the card reader.
unsigned long getID() 
{
  // Check if a new card is present.
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return 0;
  }

  // Try to read the card and check if the read was successful.
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return 0;
  }

  // Copy the cardID into an unsigned long.
  unsigned long cardID = 0;
  for (int i = 0; i < 4; i++) 
  {
    cardID = cardID << 8;
    cardID += mfrc522.uid.uidByte[i];
  }

  // Halt the card reader.
  mfrc522.PICC_HaltA();

  // Return the card ID.
  return cardID;
}

// void printID(unsigned long id)
// {
//   Serial.print(id,HEX);
//   Serial.println("");
// }

// void printEEPROM()
// {
//   int count = EEPROM.read(0);
//   for (int i = 0; i < count; i++)
//   {
//     unsigned long fromMem = readID(i);
//     printID(fromMem);
//   }
// }

// Reports the state of the button.
int bsNew, bsOld;
bool buttonPressed()
{
  bsOld = bsNew;
  bsNew = digitalRead(BUTTON_PIN);
  if (bsNew == 0 && bsOld == 1)
  {
    return true;
  }
  return false;
}

// Reports if the door is closed and implements the delay after closing.
int doorCounter = 0;
bool cDoorState, pDoorState;
bool dClosed()
{
  doorCounter += elapsed;
  if (!digitalRead(DOOR_PIN))
  {
    if (doorCounter >= CLOSE_DELAY)
    {
      doorCounter = CLOSE_DELAY;
      return true;
    }
  }
  else
  {
    doorCounter = 0;
  }
  return false;
}

// Unlocks the door.
void unlock()
{
  TurnServo(175);

  digitalWrite(BUZZER_PIN, HIGH);
  isLocked = false;
}

// Locks the door.
void lock()
{
  digitalWrite(BUZZER_PIN, LOW);

  TurnServo(30);

  isLocked = true;
}

// Turns the servo to the specified position.
void TurnServo(int angle)
{
  int ms = map(angle, 0, 180, 544, 2400);
  for(int i=0; i<200; i++)
  {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(ms);
    digitalWrite(SERVO_PIN, LOW);
    delayMicroseconds(20000-ms);
  }
}
