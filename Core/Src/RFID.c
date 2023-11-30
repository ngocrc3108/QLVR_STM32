/*
 * RFID.c
 *
 *  Created on: Nov 30, 2023
 *      Author: ngocrc
 */

#include "RFID.h"

uint8_t buff[MFRC522_MAX_LEN];
uint8_t Sectorkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void RFID_Init() {
	TM_MFRC522_Init();
}

void signExtend(uint8_t* id, uint8_t* buf) {
	for(uint8_t i = 0; i < 12; i++)
		buf[i] = id[i];
	for(uint8_t i = 12; i < BLOCK_SIZE; i++)
		buf[i] = 0;
}

uint8_t writeID(uint8_t* id) {
	TM_MFRC522_Init();
	if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
		return RFID_WRITE_ERR;

	TM_MFRC522_SelectTag(buff);

	if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, Sectorkey, buff) != MI_OK)
		return RFID_WRITE_ERR;

	uint8_t signExtendedID[BLOCK_SIZE];
	uint8_t dataAfter[BLOCK_SIZE];
	uint8_t tryCount = 0;

	signExtend(id, signExtendedID);

	do {
	tryCount++;
	TM_MFRC522_Write(ID_BLOCK, signExtendedID);
	TM_MFRC522_Read(ID_BLOCK, dataAfter);
	} while(tryCount < 5 && memcmp(signExtendedID, dataAfter, BLOCK_SIZE) != 0);

	TM_MFRC522_Halt();
	TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);

	if(tryCount >= 5)
		return RFID_WRITE_ERR;

	return RFID_OK;
}

uint8_t readID(uint8_t* id) {
	TM_MFRC522_Init();
	if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
		return RFID_WRITE_ERR;

	TM_MFRC522_SelectTag(buff);

	if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, Sectorkey, buff) != MI_OK)
		return RFID_WRITE_ERR;

	uint8_t data[BLOCK_SIZE];

	TM_MFRC522_Read(ID_BLOCK, data);

	memcpy(id, data, 12);

	TM_MFRC522_Halt();
	TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);

	return RFID_OK;
}
