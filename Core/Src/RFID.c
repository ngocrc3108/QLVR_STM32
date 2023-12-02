/*
 * RFID.c
 *
 *  Created on: Nov 30, 2023
 *      Author: ngocrc
 */

#include "RFID.h"
#include "stdio.h"

uint8_t buff[MFRC522_MAX_LEN];
uint8_t Sectorkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
	if(TM_MFRC522_Request(PICC_REQIDL, buff) != MI_OK || TM_MFRC522_Anticoll(buff) != MI_OK)
		return RFID_WRITE_ERR;

	TM_MFRC522_SelectTag(buff);

	if(TM_MFRC522_Auth(PICC_AUTHENT1A, ID_SECTOR_BLOCK, Sectorkey, buff) != MI_OK)
		return RFID_WRITE_ERR;

	uint8_t signExtendedID[BLOCK_SIZE];
	uint8_t dataAfter[BLOCK_SIZE];
	uint8_t tryCount = 0;

	memcpy(signExtendedID, id, ID_SIZE);

	do {
	tryCount++;
	TM_MFRC522_Write(ID_BLOCK, signExtendedID);
	TM_MFRC522_Read(ID_BLOCK, dataAfter);
	} while(tryCount < 5 && memcmp(signExtendedID, dataAfter, ID_SIZE) != 0);

	TM_MFRC522_Halt();
	TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);

	if(tryCount >= 5)
		return RFID_WRITE_ERR;

	return RFID_OK;
}

RFID_Status readID(uint8_t* id) {
	//TM_MFRC522_Init();
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
