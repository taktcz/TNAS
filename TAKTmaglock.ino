#include <ETH.h>
#include <WiFiAP.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiGeneric.h>
#include <WiFiType.h>
#include <WiFiScan.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "variables.h"


#include <SPI.h>
#include <Wire.h>
#include "EEPROM.h"
#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial2);
PN532 nfc(pn532hsu);


#define MAX_SRV_CLIENTS 1

bool inOTA = false;

WiFiServer server(23); //create server on port 23
WiFiClient serverClient[MAX_SRV_CLIENTS];

/*
   TODO:
   At first run generate random arrays
   Save them to eeprom -> Done
   Do that as one time run.

   Prioroty 1-4

   (2) Add cli for backup of progressions for need of reflashing and possibly for user management -> DONE

   (3) Add telnet client for door usage reporting ->

   (4) Add OTA support -> must be protected with password (init trought cli?) -> DONE

   (1) Add timer for door opening ( with audiovisual warning when opened for too long)

   (1) Add security check against unauthorised opening -> DONE

   (1) Add input for opening door from inside eg. disable security reporting for short period of time. -> DONE
*/

/*
   EEPROM map:
   0-511 - memberProgressionValue
   512-1024 - memberProgressionPosition
*/

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
}

void setupClient() {
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if (i == 21) {
    Serial.print("Could not connect to"); Serial.println(ssid);
    while (1) delay(500);
  }
}



void setup_ota() {

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    inOTA = true;
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    inOTA = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.setHostname("maglock-OTA");

  ArduinoOTA.begin();
  Serial.println("Ready for OTA!!");
  inOTA = true;
  ArduinoOTA.handle();

  for (; inOTA == true;) {
    ArduinoOTA.handle();
    delay(1);
    Serial.println("IN OTA");
    //yield();
  }
}

void serialNextIndex(char *&databuf) {
  while (*databuf != ' ') databuf++;
  while (isspace(*databuf)) databuf++;
}

void parser(char *databuf) {
  if (databuf[0] == '#') {
    switch (databuf[1]) {

      //*****************************
      case 'b':
        Serial.print("char memberProgressionPosition[512] = {");
        for (uint16_t x = 0; x <= 512; x++) {
          Serial.print(EEPROM.read(x + 512), DEC);
          Serial.print(", ");
        }
        Serial.println("}");
        Serial.print("char memberProgressionValue[512] = {");
        for (uint16_t x = 0; x <= 512; x++) {
          Serial.print(EEPROM.read(x), DEC);
          Serial.print(", ");
        }
        Serial.println("}");
        Serial.print("uint8_t byteOrder[16] = {");
        for (uint16_t i = 0; i <= 15; i++) {
          Serial.print(byteOrder[i], DEC);
          Serial.print(", ");
        }
        Serial.println("}");
        Serial.print("uint8_t valueOrder[255] = {");
        for (uint8_t i = 0; i <= 254; i++) {
          Serial.print(valueOrder[i], DEC);
          Serial.print(", ");
        }
        Serial.println("}");
        break;

      case 'd':
        serialNextIndex(databuf);
        if (atol(databuf) == pwd) {
          openDoor(true);
        }
        break;

      case 'o':
        serialNextIndex(databuf);
        if (atol(databuf) == pwd) {
          setup_ota();
        }
        else {
          Serial.println("XX");
        }
        break;

      default:
        Serial.println("Accepted commands are #b(backup), #o(OTA), #d(doorOpen)");
        break;
    }
  }
  else {
  }
}

void telnet() {
  uint8_t i;
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClient[i] || !serverClient[i].connected()) {
        if (serverClient[i]) serverClient[i].stop();
        serverClient[i] = server.available();
        Serial.print("New client: "); Serial.println(i);
        continue;
      }
    }

    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClient[i] && serverClient[i].connected()) {
      if (serverClient[i].available()) {
        char databuf[128];
        int databufptr = 0; //index
        //get data from the telnet client and push it to the UART
        while (serverClient[i].available()) {
          char foo = serverClient[i].read();
          databuf[databufptr] = foo;
          Serial.println(foo);
          databufptr++;
          if (databufptr > 128 || foo == '\n') {
            databuf[databufptr] = '\0'; // add zero to the end of buffer
            parser(databuf);
          }
        }
      }
    }
  }
}


char databuf[128];
void serialCore() {

  uint8_t availableBytes = Serial.available();

  for (uint8_t i = 0; i <= availableBytes; i++) {
    databuf[i] = Serial.read();
    if (i > 123 || databuf[i] == '\n') {
      databuf[i] = '\0';
      parser(databuf);
    }
  }


}

void readFromEEPROM() {
  for (uint16_t x = 0; x <= 511; x++) {
    memberProgressionValue[x] = EEPROM.read(x);
    memberProgressionPosition[x] = EEPROM.read(x + 512);
  }
  Serial.println("Data succesfully retrieved");
}

bool saveToEEPROM() {

  for (int16_t i = 0; i <= 511; i++) {
    EEPROM.write(i, memberProgressionValue[i]);
    EEPROM.write((i + 512), memberProgressionPosition[i]);
  }
  if (!EEPROM.commit()) {
    Serial.println("Unable to write to EEPROM");
    return false;
  }
  else {
    Serial.println("Data writen to EEPROM");
    return true;
  }
}

//Check if UID from reader match with any member UID
uint16_t compareUID(uint64_t uid) {
  Serial.println("Before compare");
  for (uint16_t i = 0; i <= 511; i++) {
    if (cardUID[i] == uid) {
      Serial.print("Found: ");
      Serial.println(members[i]);
      return i;
    }
  }
  return 0;
}

void alarm() {
  digitalWrite(doorOpenAlarm, HIGH);
}

bool checkDoorStatus() {
  if (digitalRead(inductiveSensorPin) != HIGH) {
    Serial.println("Unauthorized door access");
    return true;
  }
  return false;
}

bool doorStatusOpen = false;

bool openDoor(bool open) {

  if (open) {
    doorStatusOpen = true;
    digitalWrite(maglockControlPin, HIGH);
    delay(20);
    if (digitalRead(maglockControlPin) != LOW) {
      Serial.println("Unable read to state of maglock");
    }
  }
  else {
    doorStatusOpen = false;
    digitalWrite(maglockControlPin, LOW);
  }
}


//Read block from card, check if stored informations are valid and then write new info
void cardRW(uint16_t a, uint8_t uid[], uint8_t uidLength) {
  uint8_t blockData[16] = {0};
  uint8_t newBlockData[16] = {0};
  uint8_t currentblock = 5; // Use 5th block

  //for reading authenticate with key A and read 5th block
  if (nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyA)) {
    nfc.mifareclassic_ReadDataBlock(currentblock, blockData);
  }
  else {
    Serial.println("Unable to authenticate for reading");
  }
  //Check data in block to determine progression
  uint8_t dataPosition = 0;
  uint8_t valueProgression = 0;
  for (uint8_t i = 0; i <= 15; i++) {
    if (blockData[i] != 0) {
      dataPosition = i;
      break;
    }
  }

  //Get position of data
  for (uint8_t i = 0; i <= 254; i++) {
    if (blockData[dataPosition] ==  valueOrder[i]) {
      valueProgression = i;
      break;
    }
  }
  //Check if data at position and value match progression
  Serial.print("valProgr: ");
  Serial.println(valueProgression, HEX);
  Serial.print("meValProgr: ");
  Serial.println(memberProgressionValue[a], HEX);
  Serial.print("progrPos: ");
  Serial.println(dataPosition, HEX);
  Serial.print("memProgrPos: ");
  Serial.println(memberProgressionPosition[a], HEX);

  if (valueProgression == memberProgressionValue[a] && dataPosition == memberProgressionPosition[a]) {

    if (valueProgression == 254) {
      dataPosition++;
      valueProgression = 0;
    }
    else if (dataPosition == 15) {
      dataPosition = 0;
    }
    else {
      valueProgression++;
    }

    //Write new data to card
    if (nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyA)) {
      newBlockData[dataPosition] = valueOrder[valueProgression];
      memberProgressionValue[a] = valueOrder[valueProgression];
      nfc.mifareclassic_WriteDataBlock(currentblock, newBlockData);

      //Save data to EEPROM
      saveToEEPROM();
      openDoor(true);
    }
    else {
      Serial.println("Unable to authenticate for writing");
    }
  }
  else {
    Serial.println("Invalid card data");
  }
}

uint8_t readUID() {
  bool success = false;
  uint8_t UID[7] = {0};
  uint8_t UIDLength = 0;
  uint64_t UIDPass = 0;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, UID, &UIDLength);

  if (success) {
    Serial.println("Card found");
    Serial.print("  UID Length: "); Serial.print(UIDLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(UID, UIDLength);

    if (UIDLength == 4) {
      UIDPass = UID[0];
      UIDPass = (UIDPass << 8) | UID[1];
      UIDPass = (UIDPass << 8) | UID[2];
      UIDPass = (UIDPass << 8) | UID[3];

      //Serial.println(UIDPass, HEX);
      uint16_t number = compareUID(UIDPass);
      if (number != 0) {
        cardRW(number, UID, UIDLength);
      }
      else {
        Serial.println("UID invalid");
      }

    } else if (UIDLength == 7) {
      UIDPass = UID[0];
      UIDPass = (UIDPass << 8) | UID[1];
      UIDPass = (UIDPass << 8) | UID[3];
      UIDPass = (UIDPass << 8) | UID[4];
      UIDPass = (UIDPass << 8) | UID[5];
      UIDPass = (UIDPass << 8) | UID[6];

      //Serial.println(UIDPass, HEX);
      int number = compareUID(UIDPass);
      if (number != 0) {
        cardRW(number, UID, UIDLength);
      }
      else {
        Serial.println("UID invalid");
      }
    }
  }
}

void IRAM_ATTR isr() {
  openDoor(true);
}

void setup() {
  Serial.begin(115200);
  nfc.begin();

#if AP
  setupAP();
#elif CLIENT
  setupClient();
#endif

  pinMode(maglockControlPin, OUTPUT);
  pinMode(magLockReadbackPin, INPUT_PULLUP);
  pinMode(inductiveSensorPin, INPUT_PULLUP);
  pinMode(manualOpenPin, INPUT_PULLUP);
  pinMode(doorOpenAlarm, OUTPUT);

  attachInterrupt(manualOpenPin, isr, FALLING);

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Unable to initialize EEPROM");
  }

  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 communication error, check connection and restart");
    while (true); // Stop
  }
  else {
    Serial.println("PN532 found");
  }

  //After start restore data from EEPROM
  readFromEEPROM();

}

uint16_t interval = 1000;
long previousReadMillis = 0;        // will store last time LED was updated
long previousOpenMillis = 0;

void loop() {

  telnet();
  serialCore();

  unsigned long currentMillis = millis();

  if (currentMillis - interval > previousReadMillis && doorStatusOpen == true) {
    previousReadMillis = currentMillis;
    readUID();
  }

  if (currentMillis - doorOpenCountdown > previousOpenMillis && doorStatusOpen == false) {
    previousOpenMillis = currentMillis;
    openDoor(true);
  }
  else {
    openDoor(false);
  }

  if (doorStatusOpen == false && checkDoorStatus() == true) {
    alarm();
  }

}
