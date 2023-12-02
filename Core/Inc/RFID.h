/*
 * RFID.h
 *
 *  Created on: Nov 30, 2023
 *      Author: ngocrc
 */

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
	RFID_WRITE_ERR = 1,
	RFID_READ_ERR = 2,
	RFID_WAIT_ERR = 3
} RFID_Status;

extern void RFID_Init();
extern void convertToString(uint8_t* id, uint8_t* str);
extern RFID_Status writeID(uint8_t* id);
extern RFID_Status readID(uint8_t* id);

#endif /* INC_RFID_H_ */
