#define EEPROM_SIZE 2048

#define maglockControlPin 12
#define magLockReadbackPin 11
#define inductiveSensorPin 10
#define manualOpenPin 13
#define doorOpenAlarm 14

#define doorOpenCountdown 15000 // 15 seconds until warning

//Should ESP run in AP or client mode?
#define AP 0 
#define CLIENT 1

uint64_t pwd = 776013698255;

uint64_t cardUID[512] = {0, 0xA4004D0E, 0x4DD00D02};
char * members[512] = {0, "trimen", "trimen2"};
char memberProgressionValue[512] = {0};
char memberProgressionPosition[512] = {0};
uint8_t byteOrder[16] = {16, 5, 12, 6, 13, 7, 10, 15, 4, 3, 9, 2, 14, 8, 11, 1};
uint8_t valueOrder[255] = {255 , 227, 69, 27, 65, 43, 100, 226, 146, 76, 198, 138, 114, 62, 249, 48, 188, 230, 172, 87, 42, 223, 8, 104, 211, 246, 180, 9, 162, 109, 134, 88, 19, 195, 112, 143, 17, 196, 120, 136, 40, 73, 248, 133, 118, 244, 164, 63, 222, 93, 125, 242, 68, 234, 34, 233, 1, 247, 79, 148, 122, 18, 10, 61, 60, 3, 78, 103, 216, 151, 189, 217, 26, 184, 12, 245, 176, 71, 82, 150, 81, 149, 141, 98, 30, 193, 252, 113, 16, 23, 190, 51, 201, 144, 47, 251, 156, 41, 124, 126, 254, 7, 250, 192, 168, 84, 186, 253, 107, 191, 167, 6, 20, 218, 50, 80, 89, 99, 173, 86, 66, 123, 241, 232, 116, 175, 229, 219, 54, 119, 91, 57, 117, 239, 13, 161, 49, 145, 28, 137, 213, 210, 147, 209, 182, 159, 237, 225, 169, 74, 224, 31, 179, 215, 94, 153, 96, 111, 70, 102, 129, 181, 45, 55, 154, 203, 108, 72, 24, 185, 2, 135, 238, 194, 121, 187, 207, 220, 25, 58, 240, 152, 95, 170, 105, 44, 174, 92, 37, 158, 200, 197, 206, 33, 178, 140, 21, 59, 171, 160, 202, 46, 67, 131, 199, 75, 157, 183, 142, 14, 39, 127, 106, 139, 208, 130, 52, 166, 85, 97, 77, 15, 36, 228, 101, 4, 110, 155, 90, 214, 243, 231, 165, 29, 204, 163, 11, 53, 177, 22, 38, 115, 56, 205, 5, 212, 132, 64, 35, 221, 235, 83, 32, 128, 236};

//uint8_t keyA[6] = {0xFA, 0xEE, 0xDD, 0xFF, 0xA0, 0xAA};
//uint8_t keyB[6] = {0x22, 0x7F, 0x69, 0x42, 0xDE, 0xF2};
uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t keyB[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if AP
const char ssid[] = "maglock";
const char password[] = "supersecret";
#elif CLIENT
const char* ssid = "trimen";
const char* password = "ahojahoj";
#endif
