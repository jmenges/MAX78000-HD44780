#ifndef HD44780_H_INCLUDED
#define HD44780_H_INCLUDED
/*
||
||  Filename:	 		HD44780.h
||  Title:			HD44780 Driver for MAX78000
||  Author:			Jonas Menges (dev.menges.jonas@gmail.com)
||  Description: 
||    This library can drive HD44780 based LCD. 
||    The LCD is driven exclusively in 4-bit mode. 
||
||
||  Original Author for AVR-Version:
||  Efthymios Koktsidis (efthymios.ks@gmail.com)
||  https://github.com/efthymios-ks/AVR-HD44780
||		
||
*/

//----- Headers ------------//
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_delay.h"

//--------------------------//
//General use bit manipulating commands
#define BitSet(		x, y)			(	x |=	 (1UL<<y)			)
#define BitClear(	x, y)			(	x &=	(~(1UL<<y))			)
#define BitToggle(	x, y)			(	x ^=	 (1UL<<y)			)
#define BitCheck(	x, y)			(	x &		 (1UL<<y)	? 1 : 0	)



//----- Configuration --------------------------//
//LCD size
#define LCD_Size	2004

//LCD pins			PIN
#define LCD_PORT    MXC_GPIO0
#define LCD_D4		MXC_GPIO_PIN_19
#define LCD_D5		MXC_GPIO_PIN_11
#define LCD_D6		MXC_GPIO_PIN_8
#define LCD_D7		MXC_GPIO_PIN_9
#define LCD_EN		MXC_GPIO_PIN_6
#define LCD_RW		MXC_GPIO_PIN_5
#define LCD_RS		MXC_GPIO_PIN_7



//----- Auxiliary data ---------------------------//
#define __LCD_CMD_ClearDisplay			0x01
#define __LCD_CMD_ReturnHome			0x02
#define __LCD_CMD_EntryModeSet			0x04
#define __LCD_CMD_DisplayControl		0x08
#define __LCD_CMD_CursorShift			0x10
#define __LCD_CMD_FunctionSet			0x20
#define __LCD_CMD_SetCGRAMAddress		0x40
#define __LCD_CMD_SetDDRAMAddress		0x80

#define __LCD_CMD_EntryIncrement		0x02
#define __LCD_CMD_EntryDecrement		0x00
#define __LCD_CMD_EntryShift			0x01
#define __LCD_CMD_EntryNoShift			0x00

#define __LCD_CMD_DisplayOn				0x04
#define __LCD_CMD_DisplayOff			0x00
#define __LCD_CMD_CursonOn				0x02
#define __LCD_CMD_CursorOff				0x00
#define __LCD_CMD_BlinkOn				0x01
#define __LCD_CMD_BlinkOff				0x00

#define __LCD_CMD_DisplayMove			0x08
#define __LCD_CMD_CursorMove			0x00
#define __LCD_CMD_MoveRight				0x04
#define __LCD_CMD_MoveLeft				0x00

#define __LCD_CMD_8BitMode				0x10
#define __LCD_CMD_4BitMode				0x00
#define __LCD_CMD_2Line					0x08
#define __LCD_CMD_1Line					0x00
#define __LCD_CMD_5x10Dots				0x04
#define __LCD_CMD_5x8Dots				0x00


#define __LCD_BusyFlag					7


typedef struct
{
	uint8_t X,Y;
}Point_t;
//------------------------------------------------//

//----- Prototypes ------------------------------------------------------------//
void LCD_Setup();
void LCD_SendCommand(uint8_t Command);
void LCD_SendData(char Character);
void LCD_WaitBusy();
void LCD_BuildChar(char *Data, uint8_t Position);
void LCD_BuildChar_P(const char *Data, uint8_t Position);

void LCD_Clear();
void LCD_ClearLine(uint8_t Line);
void LCD_GotoXY(uint8_t X, uint8_t Y);
Point_t LCD_GetP();
uint8_t LCD_GetX();
uint8_t LCD_GetY();

void LCD_PrintChar(char Character);
void LCD_PrintString(char *Text);
void LCD_PrintString_P(const char *Text);
void LCD_PrintInteger(int32_t Value);
void LCD_PrintDouble(double Value, uint32_t Tens);
//-----------------------------------------------------------------------------//
#endif
