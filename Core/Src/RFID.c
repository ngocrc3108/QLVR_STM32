/*
 * RFID.c
 *
 *  Created on: Nov 30, 2023
 *      Author: ngocrc
 */

#include "RFID.h"
#include "stdio.h"

#define SECTOR_SIZE 6

uint8_t buff[MFRC522_MAX_LEN];
uint8_t sectorKeyA[SECTOR_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t defaultSectorKeyA[SECTOR_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t defaultAccessBits[4] = {0xFF, 0x07, 0x80, 0x69};
void RFID_Init() {
	TM_MFRC522_Init();
}

void convertToString(uint8_t* id, uint8_t* str) {
	for(uint8_t i = 0; i < ID_SIZE; i++)
		sprintf((char*)(str + i*2), "%02x", id[i]);
	str[ID_SIZE*2] = '\0';
}

void convertStringToHexId(uint8_t* str, uint8_t* id) {
	for(uint8_t i = 0, j = 0; i < ID_SIZE; i++, j+=2) {
		// first character
		if(str[j] >= '0' && str[j] <= '9')
			id[i] = ((str[j] - '0') << 4);
		else
			id[i] = ((str[j] - 'a' + 10) << 4);

		// second character
		if(str[j+1] >= '0' && str[j+1] <= '9')
			id[i] += str[j+1] - '0';
		else
			id[i] += str[j+1] - 'a' + 10;
	}
}

RFID_Status writeID(uint8_t* id) {
	//TM_MFRC522_Init();
	RFID_Status status = RFID_AUTH_BY_DEFAULT;
	if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
		return RFID_NO_TARGET;

	TM_MFRC522_SelectTag(buff);

	if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, defaultSectorKeyA, buff) != MI_OK) {
		// can not authenticate using defaultSectorKeyA, try again with system key (sectorKeyA).

		if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
			return RFID_NO_TARGET;

		TM_MFRC522_SelectTag(buff);

		if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, sectorKeyA, buff) != MI_OK)
			return RFID_WRITE_DATA_ERR; // give up :))

		status = RFID_AUTH_BY_SYSTEM;
	}

	uint8_t writeData[BLOCK_SIZE];
	uint8_t dataAfter[BLOCK_SIZE];
	uint8_t tryCount = 0;

	memcpy(writeData, id, ID_SIZE);

	do {
	tryCount++;
	TM_MFRC522_Write(ID_BLOCK, writeData);
	TM_MFRC522_Read(ID_BLOCK, dataAfter);
	} while(tryCount < 5 && memcmp(writeData, dataAfter, ID_SIZE) != 0);

	if(tryCount >= 5) {
		TM_MFRC522_Halt();
		TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);
		return RFID_WRITE_DATA_ERR;
	}

	if(status == RFID_AUTH_BY_DEFAULT) {
		// write new key A
		memcpy(writeData, sectorKeyA, SECTOR_SIZE);
		memcpy(writeData + 6, defaultAccessBits, SECTOR_SIZE);
		// write new key B
		memcpy(writeData + 10, sectorKeyA, SECTOR_SIZE);

		tryCount = 0;
		do {
		tryCount++;
		TM_MFRC522_Write(ID_SECTOR_BLOCK, writeData);
		TM_MFRC522_Read(ID_SECTOR_BLOCK, dataAfter);
		} while(tryCount < 10 && memcmp(writeData + 6, dataAfter + 6, ID_SIZE - 6) != 0); // key A always be masked.
		if(tryCount >= 10) {
			TM_MFRC522_Halt();
			TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);
			return RFID_WRITE_SECTOR_ERR;
		}

	}

	TM_MFRC522_Halt();
	TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);

	return RFID_OK;
}

RFID_Status readID(uint8_t* id) {
	//TM_MFRC522_Init();
	if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
		return RFID_NO_TARGET;

	TM_MFRC522_SelectTag(buff);

	if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, sectorKeyA, buff) != MI_OK)
		return RFID_WRITE_DATA_ERR;

	uint8_t data[BLOCK_SIZE];

	TM_MFRC522_Read(ID_BLOCK, data);

	memcpy(id, data, 12);

	TM_MFRC522_Halt();
	TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);

	return RFID_OK;
}
