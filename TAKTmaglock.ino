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


WiFiServer server(23); //create server on port 23
WiFiClient serverClient[MAX_SRV_CLIENTS];

/*
   TODO:
   At first run generate random arrays
   Save them to eeprom -> Done
   Do that as one time run.

   Prioroty 1-4

   (1) Modify EEPROM writing, so only changes are actually writen

   (4) Use FRAM for storage

   (2) Use PN532 GPIOs for controllimg status LEDs outside
*/

/*
   EEPROM map:
   0 - Init bit - if zero save arrays to EEPROM else load.
   1-512 - memberProgressionValue
   513-1025 - memberProgressionPosition
   1026 - uint8_t door delay
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
    Serial.println("Rollback to AP");
    setupAP();
  }
}



void setup_ota() {

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    inOTA = true;
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    EEPROM.write(0, 0);
    EEPROM.commit();
    inOTA = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });

  ArduinoOTA.setHostname("maglock-OTA");

  ArduinoOTA.begin();
  Serial.println("Ready for OTA!!");
  inOTA = true;
  ArduinoOTA.handle();

  for (; inOTA == true;) {
    ArduinoOTA.handle();
    delay(1);
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
        serialNextIndex(databuf);
        if (strncmp(pwd, databuf, 32) == 0) {
          Serial.print("char * members[512] = {");
          serverClient[0].print("char * members[512] = {");
          for (uint16_t x = 0; x <= 511; x++) {
            Serial.print(members[x]);
            serverClient[0].print(members[x]);
            Serial.print(", ");
            serverClient[0].print(", ");
          }
          Serial.println("}");
          serverClient[0].println("}");
          Serial.print("char memberProgressionPosition[512] = {");
          serverClient[0].print("char memberProgressionPosition[512] = {");
          for (uint16_t x = 0; x <= 511; x++) {
            Serial.print(EEPROM.read(x + 513), DEC);
            Serial.print(", ");
            serverClient[0].print(EEPROM.read(x + 513), DEC);
            serverClient[0].print(", ");
          }
          Serial.println("}");
          serverClient[0].println("}");
          Serial.print("char memberProgressionValue[512] = {");
          serverClient[0].print("char memberProgressionValue[512] = {");
          for (uint16_t x = 0; x <= 511; x++) {
            Serial.print(EEPROM.read(x + 1), DEC);
            Serial.print(", ");
            serverClient[0].print(EEPROM.read(x + 1), DEC);
            serverClient[0].print(", ");
          }
          Serial.println("}");
          serverClient[0].println("}");
          Serial.print("uint8_t byteOrder[16] = {");
          serverClient[0].print("uint8_t byteOrder[16] = {");
          for (uint16_t i = 0; i <= 15; i++) {
            Serial.print(byteOrder[i], DEC);
            Serial.print(", ");
            serverClient[0].print(byteOrder[i], DEC);
            serverClient[0].print(", ");
          }
          Serial.println("}");
          serverClient[0].println("}");
          Serial.print("uint8_t valueOrder[255] = {");
          serverClient[0].print("uint8_t valueOrder[255] = {");
          for (uint8_t i = 0; i <= 254; i++) {
            Serial.print(valueOrder[i], DEC);
            Serial.print(", ");
            serverClient[0].print(valueOrder[i], DEC);
            serverClient[0].print(", ");
          }
          Serial.println("}");
          serverClient[0].println("}");
        }
        break;

      case 'd':
        serialNextIndex(databuf);
        if (strncmp(pwd, databuf, 32) == 0) {
          openDoor(true);
        }
        break;

      case 'o':
        serialNextIndex(databuf);
        if (strncmp(pwd, databuf, 32) == 0) {
          setup_ota();
        }
        break;

      case 'f':
        serialNextIndex(databuf);
        if (strncmp(pwd, databuf, 32) == 0) {
          readUID(true);//true == fix broken card
        }
        break;

      default:
        Serial.println("Accepted commands are #b(backup) [pwd], #o(OTA) [pwd], #d(doorOpen) [pwd], #f(fixBr0ken card) [pwd]");
        serverClient[0].println("Accepted commands are #b(backup) [pwd], #o(OTA) [pwd], #d(doorOpen) [pwd], #f(fixBr0ken card) [pwd]");
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
    memberProgressionValue[x] = EEPROM.read(x + 1);
    memberProgressionPosition[x] = EEPROM.read(x + 513);
  }
  Serial.println("Data succesfully retrieved");
}

bool saveToEEPROM() {

  for (int16_t i = 0; i <= 511; i++) {
    EEPROM.write(i + 1, memberProgressionValue[i]);
    EEPROM.write((i + 513), memberProgressionPosition[i]);
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
  for (uint16_t i = 0; i <= 511; i++) {
    if (cardUID[i] == uid) {
      Serial.print("  Found: ");
      Serial.println(members[i]);
      serverClient[0].print("  Found: ");
      serverClient[0].println(members[i]);
      return i;
    }
  }
  return 0;
}

void alarm() {
  digitalWrite(doorOpenAlarm, HIGH);
}

bool checkDoorStatus() {
  if (digitalRead(inductiveSensorPin) != inductiveSensorDefault) {
    Serial.println("Unauthorized door access");
    return true;
  }
  return false;
}

void openDoor(bool open) {

  if (open) {
    doorStatusOpen = true;
    previousOpenMillis = millis();
    Serial.println("Door open");
    serverClient[0].println("Door open");
    digitalWrite(maglockControlPin, !maglockDefaultOn);
    digitalWrite(doorOpenButtonlLED, HIGH);
    delay(20);
    if (digitalRead(magLockReadbackPin) != maglockReadbackDefault) {
      Serial.println("Unable read to state of maglock");
      serverClient[0].println("Unable read to state of maglock");
    }
  }
  else {
    Serial.println("Door close");
    serverClient[0].println("Door close");
    doorStatusOpen = false;
    digitalWrite(maglockControlPin, maglockDefaultOn);
    digitalWrite(doorOpenButtonlLED, LOW);
  }
}


//Read block from card, check if stored informations are valid and then write new info
void cardRW(uint16_t a, uint8_t uid[], uint8_t uidLength, bool fix) {
  uint8_t blockData[16] = {0};
  uint8_t newBlockData[16] = {0};
  uint8_t currentblock = 4; // Use 4th block

  //for reading authenticate with key A and read 5th block
  if (nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyA)) {
    //Read block
    nfc.mifareclassic_ReadDataBlock(currentblock, blockData);

    Serial.print("Blockdata: ");
    nfc.PrintHex(blockData, 16);
    Serial.println();

    //Check data in block to determine position
    uint8_t dataPosition = 0;
    uint8_t valueProgression = 0;
    //Get position of data
    for (uint8_t i = 0; i <= 15; i++) {
      if (blockData[i] != 0) {
        dataPosition = i;
        break;
      }
    }
    //Get value at position
    for (uint8_t i = 0; i <= 254; i++) {
      if (valueOrder[i] == blockData[dataPosition]) {
        valueProgression = i;
        break;
      }
    }

    //Check if data at position and value match progression
    if (valueProgression == memberProgressionValue[a] && dataPosition == memberProgressionPosition[a]) {

      if (valueProgression == 254) {
        dataPosition++;
        valueProgression = 0;
      }
      else {
        valueProgression++;
      }
      if (dataPosition == 16) {
        dataPosition = 0;
      }


      //Write new data to card
      if (nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyA)) {
        newBlockData[dataPosition] = valueOrder[valueProgression];
        memberProgressionValue[a] = valueProgression;
        memberProgressionPosition[a] = dataPosition;
        nfc.mifareclassic_WriteDataBlock(currentblock, newBlockData);

        //Save data to EEPROM
        saveToEEPROM();
        openDoor(true);
      }
      else {
        Serial.println("Unable to authenticate for writing");
        serverClient[0].println("Unable to authenticate for writing");
      }
    }
    else if (fix) {
      if (nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyA)) {
        newBlockData[dataPosition] = valueOrder[valueProgression];
        memberProgressionValue[a] = valueProgression;
        memberProgressionPosition[a] = dataPosition;
        nfc.mifareclassic_WriteDataBlock(currentblock, newBlockData);

        //Save data to EEPROM
        saveToEEPROM();
        Serial.println("Card fixed");
      }
    }
    else {
      Serial.println("Invalid card data");
      serverClient[0].println("Invalid card data");
    }

  }
  else {
    Serial.println("Unable to authenticate for reading");
    serverClient[0].println("Unable to authenticate for reading");
  }

}

void readUID(bool fix) {
  bool success = false;
  uint8_t UID[7] = {0};
  uint8_t UIDLength = 0;
  uint64_t UIDPass = 0;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, UID, &UIDLength, 5000);

  if (success) {
    Serial.println("Card found");
    Serial.print("  UID Length: "); Serial.print(UIDLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(UID, UIDLength);

    if (UIDLength == 4) {
      //Convert UID array to single value
      UIDPass = UID[0];
      UIDPass = (UIDPass << 8) | UID[1];
      UIDPass = (UIDPass << 8) | UID[2];
      UIDPass = (UIDPass << 8) | UID[3];

      //Serial.println(UIDPass, HEX);
      uint16_t number = compareUID(UIDPass);
      if (number != 0) {
        cardRW(number, UID, UIDLength, fix);
      }
      else {
        Serial.println("UID invalid");
        serverClient[0].println("UID invalid");
      }

    } else if (UIDLength == 7) {
      //Convert UID array to single value
      UIDPass = UID[0];
      UIDPass = (UIDPass << 8) | UID[1];
      UIDPass = (UIDPass << 8) | UID[3];
      UIDPass = (UIDPass << 8) | UID[4];
      UIDPass = (UIDPass << 8) | UID[5];
      UIDPass = (UIDPass << 8) | UID[6];

      //Serial.println(UIDPass, HEX);
      int number = compareUID(UIDPass);
      if (number != 0) {
        cardRW(number, UID, UIDLength, fix);
      }
      else {
        Serial.println("UID invalid");
        serverClient[0].println("UID invalid");
      }
    }
  }
}

unsigned long last_interrupt_time = 0;

void IRAM_ATTR isr() {
  //Debouncing
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) {
    Serial.print("Manual ");
    openDoor(true);
  }
  last_interrupt_time = interrupt_time;
}

uint32_t versiondata;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  nfc.begin();

  versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
  }
  else {
    Serial.println("PN532 found");
  }

  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();

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
  pinMode(doorOpenButtonlLED, OUTPUT);

  attachInterrupt(manualOpenPin, isr, FALLING);

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Unable to initialize EEPROM");
  }

  //If this is first run copy arrays to EEPROM
  if (EEPROM.read(0) == 0) {
    EEPROM.write(0, 1);
    saveToEEPROM();
  }
  else {
    //After start restore data from EEPROM
    readFromEEPROM();
  }
}


void loop() {

  telnet();
  serialCore();

  unsigned long currentMillis = millis();

  if (currentMillis - cardReadInterval >= previousReadMillis && doorStatusOpen == false) {
    previousReadMillis = currentMillis;
    if (! versiondata) {
      Serial.println("Didn't find PN53x board");
    }
    else {
      readUID(false);//false == do not fix broken card
    }
  }

  if (doorStatusOpen == true && digitalRead(manualOpenPin) == HIGH) {
    if (currentMillis - doorOpenCountdown >= previousOpenMillis) {
      openDoor(false);
      previousOpenMillis = millis();
    }
  }

}
