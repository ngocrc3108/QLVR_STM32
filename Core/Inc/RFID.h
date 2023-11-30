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

#define BLOCK_SIZE 16
#define ID_BLOCK 6
#define ID_SECTOR_BLOCK 7

typedef enum {
	RFID_OK = 0,
	RFID_WRITE_ERR = 1,
	RFID_READ_ERR = 2
} RFID_Status;

extern void RFID_Init();
extern void signExtend(uint8_t* id, uint8_t* buf);
extern uint8_t writeID(uint8_t* id);
extern uint8_t readID(uint8_t* id);

#endif /* INC_RFID_H_ */
