#ifndef __I2C_LCD_H__
#define __I2C_LCD_H__

#include "stm32f1xx_hal.h"
#include "string.h"

#define LCD_LENGTH 16

typedef enum {
	DM_HOME_SCREEN,
	DM_INFO
} Display_mode;

typedef struct {
	Display_mode mode;
	uint32_t time;
} Display;

void displayInfo(Display* display, char* str); // printf info in the second row, mark the time to clear this row after some time

void lcd_init (void);   // initialize lcd

void lcd_send_cmd (char cmd);  // send command to the lcd

void lcd_send_data (char data);  // send data to the lcd

void lcd_send_string (char *str);  // send string to the lcd

void lcd_clear_display (void);	//clear display lcd

void lcd_goto_XY (int row, int col); //set proper location on screen

#endif
