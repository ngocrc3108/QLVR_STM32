#ifndef INC_RFID_H_
#define INC_RFID_H_

#include "rc522.h"
#include "string.h"

#define ID_SIZE 12
#define BLOCK_SIZE 16
#define ID_BLOCK 6
#define ID_SECTOR_BLOCK 7

typedef enum {
	RFID_OK = 0,
	RFID_WRITE_DATA_ERR,
	RFID_AUTH_ERR,
	RFID_WRITE_SECTOR_ERR,
	RFID_READ_ERR,
	RFID_NO_TARGET,
	RFID_AUTH_BY_DEFAULT,
	RFID_AUTH_BY_SYSTEM
} RFID_Status;

extern void RFID_Init();
extern void convertToString(const char* id, char* str);
extern void convertStringToHexId(const char* str, char* id);
extern RFID_Status writeID(const char* id);
extern RFID_Status readID(char* id);

#endif /* INC_RFID_H_ */
