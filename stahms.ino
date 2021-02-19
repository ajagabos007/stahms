
//< (1)------------------------------Soldier's Details--------------------------------->

String forceNumber, fullName;         // soldiers unique RFID number in hex and fullname
int BPM ;                             // Heart Beats Per Minutes
int longitude, latitude ;             // Position; Longitude and latitude
bool online = false;                   // online status of soldier with default value of false.


//< (1)------------------------------RFID Setup--------------------------------->
/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read new NUID from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to the read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the type, and the NUID if a new card has been detected. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5   ,,,,      D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];


//< (2) ------------------------------Pulse rate sensor setup--------------------------------->

/*  Getting_BPM_to_Monitor prints the BPM to the Serial Monitor, using the least lines of code and PulseSensor Library.
    Tutorial Webpage: https://pulsesensor.com/pages/getting-advanced

  --------Use This Sketch To------------------------------------------
  1) Displays user's live and changing BPM, Beats Per Minute, in Arduino's native Serial Monitor.
  2) Print: "♥  A HeartBeat Happened !" when a beat is detected, live.
  2) Learn about using a PulseSensor Library "Object".
  4) Blinks LED on PIN 13 with user's Heartbeat.
  --------------------------------------------------------------------*/

#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library.   

//  Variables
const int PulseWire = 0;       // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
const int LED13 = 13;          // The on-board Arduino LED, close to PIN 13.
int Threshold = 550;           // Determine which Signal to "count as a beat" and which to ignore.
// Use the "Gettting Started Project" to fine-tune Threshold Value beyond default setting.
// Otherwise leave the default "550" value.

PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object called "pulseSensor"


void setup() {

  Serial.begin(9600);          // For Serial Monitor

  Serial.println("<----- STaHMS - Soldier's Tracking and Health Monitoring System----->");

  initRFID();

  initPulseRateSensor();

  Serial.println("\n Pleae scan your smart card to come online");
  
}



void loop() {
  RFID();
  if (online)
    Serial.println("Welcome " + forceNumber + ", you are now succesffully online");

  while (online) {
    bool isPulsePresent = heartBeats();
    delay(20);                    // considered best practice in a simple sketch.

  }
}

//< ---------------------------------RFID initialization------------------------------------>

void initRFID() {

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  Serial.println();
}

void RFID() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    // Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    //    Serial.println(F("The NUID tag is:"));
    //    Serial.print(F("In hex: "));
    //    printHex(rfid.uid.uidByte, rfid.uid.size);
    //    Serial.println();
    //    Serial.print(F("In dec: "));
    //    printDec(rfid.uid.uidByte, rfid.uid.size);
    //    Serial.println();

    setForceNumber(rfid.uid.uidByte, rfid.uid.size);

  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void setForceNumber(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    forceNumber += String(buffer[i], HEX);
  }
  forceNumber.toUpperCase();
  online = true;
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

//< --------------------------Pulse rate sensor initialization---------------------------->


void initPulseRateSensor() {

  // Configure the PulseSensor object, by assigning our variables to it.
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);

  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object!");  //This prints one time at Arduino power-up,  or on Arduino reset.
  }

}
bool heartBeats() {
  int bpm = pulseSensor.getBeatsPerMinute();      // Calls function on our pulseSensor object that returns BPM as an "int".
  // "bpm" hold this BPM value now.
  if (pulseSensor.sawStartOfBeat()) {             // Constantly test to see if "a beat happened".
    Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
    Serial.print("BPM: ");                        // Print phrase "BPM: "
    BPM = bpm;                                    //  store the value of bpm in BPM
    Serial.println(BPM);                          // Print the value inside of BPM.
    return true;
  }
  return false;
}