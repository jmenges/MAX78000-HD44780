#include "HD44780.h"

//----- Auxiliary data --------------------------//
#define _Byte2Ascii(Value)			(Value = Value + '0')

#if (LCD_Size == 801)
#define __LCD_Rows					1
#define __LCD_Columns				8
#define __LCD_LineStart_1			0x00
#elif (LCD_Size == 802)
#define __LCD_Rows					2
#define __LCD_Columns				8
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#elif (LCD_Size == 1601)
#define __LCD_Rows					1
#define __LCD_Columns				16
#define __LCD_LineStart_1			0x00
#if (LCD_Type == B)
#define __LCD_LINESTART_1B		0x40
#endif
#elif (LCD_Size == 1602)
#define __LCD_Rows					2
#define __LCD_Columns				16
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#elif (LCD_Size == 1604)
#define __LCD_Rows					4
#define __LCD_Columns				16
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#define __LCD_LineStart_3			0x10
#define __LCD_LineStart_4			0x50
#elif (LCD_Size == 2001)
#define __LCD_Rows					1
#define __LCD_Columns				20
#define __LCD_LineStart_1			0x00
#elif (LCD_Size == 2002)
#define __LCD_Rows					2
#define __LCD_Columns				20
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#elif (LCD_Size == 2004)
#define __LCD_Rows					4
#define __LCD_Columns				20
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#define __LCD_LineStart_3			0x14
#define __LCD_LineStart_4			0x54
#elif (LCD_Size == 4001)
#define __LCD_Rows					1
#define __LCD_Columns				40
#define __LCD_LineStart_1			0x00
#elif (LCD_Size == 4002)
#define __LCD_Rows					2
#define __LCD_Columns				40
#define __LCD_LineStart_1			0x00
#define __LCD_LineStart_2			0x40
#endif

//-----------------------------------------------//
int lcdData[] = {LCD_D4, LCD_D5, LCD_D6, LCD_D7};

//----- Prototypes ----------------------------//
static void LCD_SendCommandHigh(uint8_t Command);
static void LCD_Send(uint8_t Data);
static inline void Pulse_En();
static void Int2bcd(int32_t Value, char *BCD);
//---------------------------------------------//

//----- Functions -------------//
//Setup LCD.
void LCD_Setup()
{
	//LCD pins = Outputs
	mxc_gpio_cfg_t gpio_out;
	gpio_out.port = LCD_PORT;
    gpio_out.mask = LCD_D4| LCD_D5 | LCD_D6 | LCD_D7 | LCD_RS | LCD_RW | LCD_EN;
    gpio_out.pad = MXC_GPIO_PAD_NONE;
    gpio_out.func = MXC_GPIO_FUNC_OUT;
	gpio_out.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_out);
	
	//LCD pins = 0
	MXC_GPIO_OutClr(LCD_PORT, LCD_D4);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D5);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D6);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D7);
	MXC_GPIO_OutClr(LCD_PORT, LCD_EN);
	MXC_GPIO_OutClr(LCD_PORT, LCD_RW);
	MXC_GPIO_OutClr(LCD_PORT, LCD_RS);


	//----- Soft reset -----
	//1. Wait for more than 15ms
	MXC_Delay(MXC_DELAY_MSEC(15));
	//2. Command 32: LCD 8-bit mode
	LCD_SendCommandHigh(__LCD_CMD_FunctionSet | __LCD_CMD_8BitMode);
	//3. Wait for more than 4.1ms
	MXC_Delay(MXC_DELAY_MSEC(5));
	//4. Command 32: LCD 8-bit mode
	LCD_SendCommandHigh(__LCD_CMD_FunctionSet | __LCD_CMD_8BitMode);
	//5. Wait for more than 100us
	MXC_Delay(MXC_DELAY_USEC(100));
	//6. Command 32: LCD 8-bit mode, for the 3rd time
	LCD_SendCommandHigh(__LCD_CMD_FunctionSet | __LCD_CMD_8BitMode);
	//7. Wait for more than 100us
	MXC_Delay(MXC_DELAY_USEC(100));



	//----- Initialization -----
	LCD_SendCommandHigh(__LCD_CMD_FunctionSet | __LCD_CMD_4BitMode);
	//2. Command 32: LCD mode and size
	LCD_SendCommand(__LCD_CMD_FunctionSet | __LCD_CMD_4BitMode | __LCD_CMD_2Line | __LCD_CMD_5x8Dots);
	//3. Command 8: Display On, Cursor off, Blinking Off
	LCD_SendCommand(__LCD_CMD_DisplayControl | __LCD_CMD_DisplayOn | __LCD_CMD_CursorOff | __LCD_CMD_BlinkOff);
	//4. Command 4: Auto increment, No shifting
	LCD_SendCommand(__LCD_CMD_EntryModeSet | __LCD_CMD_EntryIncrement | __LCD_CMD_EntryNoShift);
	//5. Command 1: Clear display, cursor at home
	LCD_SendCommand(__LCD_CMD_ClearDisplay);
}

// used for debugging
void print_to_binary(uint8_t x, char *prefix){
    static char b[512];
    char *p = b;
    b[0] = '\0';

    for(int i=7; i>=0; i--){
      *p++ = (x & (1<<i)) ? '1' : '0';
      if(!(i%4)) *p++ = ' ';
    }

    printf("%s 0x%02X - %s\n", prefix, x, b);
}


//Wait until busy flag is cleared.
void LCD_WaitBusy()
{
	uint8_t busy = 0;
	
	//LCD pins = INPUT
	mxc_gpio_cfg_t gpio;
	gpio.port = LCD_PORT;
    gpio.mask = LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7;
    gpio.pad = MXC_GPIO_PAD_PULL_DOWN;
    gpio.func = MXC_GPIO_FUNC_IN;
    MXC_GPIO_Config(&gpio);

	MXC_GPIO_OutClr(LCD_PORT, LCD_RS);	//RS=0
	MXC_GPIO_OutSet(LCD_PORT, LCD_RW);	//RW=1

	do
	{
		//High nibble comes first
		MXC_GPIO_OutSet(LCD_PORT, LCD_EN);
		MXC_Delay(MXC_DELAY_USEC(1));
		busy &= ~(1<<__LCD_BusyFlag);
		busy |= (MXC_GPIO_InGet(LCD_PORT, LCD_D7)<<__LCD_BusyFlag);
		MXC_GPIO_OutClr(LCD_PORT, LCD_EN);

		//Low nibble follows
		Pulse_En();
	}
	while(BitCheck(busy, __LCD_BusyFlag));

	//LCD pins = Outputs
    gpio.pad = MXC_GPIO_PAD_NONE;
    gpio.func = MXC_GPIO_FUNC_OUT;
	gpio.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio);

	// Clear Pins
	MXC_GPIO_OutClr(LCD_PORT, LCD_D4);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D5);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D6);
	MXC_GPIO_OutClr(LCD_PORT, LCD_D7);

	// RW = 0 
	MXC_GPIO_OutClr(LCD_PORT, LCD_RW);
}


//Send command to LCD.
void LCD_SendCommand(uint8_t Command)
{
	LCD_WaitBusy();

	MXC_GPIO_OutClr(LCD_PORT, LCD_RS);
	LCD_Send(Command);
}

//Send data to LCD.
void LCD_SendData(char c)
{
	LCD_WaitBusy();

	MXC_GPIO_OutSet(LCD_PORT, LCD_RS);
	LCD_Send((uint8_t)(c));
}


//Build character in LCD CGRAM from data in SRAM.
void LCD_BuildChar(char *Data, uint8_t Position)
{
	if (Position < 0)
		return;
	if (Position >= 8)
		return;

	Point_t p = LCD_GetP();
	uint8_t i;

	//Every character in CGRAM needs 8bytes
	LCD_SendCommand(__LCD_CMD_SetCGRAMAddress | (Position<<3));

	//Save the character byte-by-byte
	for (i = 0 ; i < 8 ; i++)
		LCD_SendData(Data[i]);

	//Return to the DDRAM position
	LCD_GotoXY(p.X, p.Y);
}

//Build character in LCD CGRAM from data in Flash memory.
void LCD_BuildChar_P(const char *Data, uint8_t Position)
{
	if (Position < 0)
		return;
	if (Position >= 8)
		return;

	Point_t p = LCD_GetP();
	uint8_t i;

	//Every character in CGRAM needs 8bytes
	LCD_SendCommand(__LCD_CMD_SetCGRAMAddress | (Position<<3));

	//Save the character byte-by-byte
	for (i = 0 ; i < 8 ; i++)
		LCD_SendData(pgm_read_byte(Data[i]));

	//Return to the DDRAM position
	LCD_GotoXY(p.X, p.Y);
}

//Clear display.
void LCD_Clear()
{
	LCD_SendCommand(__LCD_CMD_ClearDisplay);
}

//Clear line.
void LCD_ClearLine(uint8_t Line)
{
	uint8_t i = 0;
	
	LCD_GotoXY(0, Line);
	while(i <= __LCD_Columns)
	{
		LCD_SendData(' ');
		i++;
	}
}

//Go to specified position.
void LCD_GotoXY(uint8_t X, uint8_t Y)
{
	if ((X < __LCD_Columns) && (Y < __LCD_Rows))
	{
		uint8_t addr = 0;
		switch (Y)
		{
			#if ((defined(__LCD_LineStart_4)) || (defined(__LCD_LineStart_3)) || (defined(__LCD_LineStart_2)) || (defined(__LCD_LineStart_1)))
				case (0):
					addr = __LCD_LineStart_1;
					#if ((LCD_Size == 1601) && (LCD_Type == B))
					if (X >= (__LCD_Columns>>1))
					{
						X -= __LCD_Columns>>1;
						addr = __LCD_LINESTART_1B;
					}
					#endif
				break;
			#endif
			#if ((defined(__LCD_LineStart_4)) || (defined(__LCD_LineStart_3)) || (defined(__LCD_LineStart_2)))
				case (1):
					addr = __LCD_LineStart_2;
					break;
			#endif
			#if ((defined(__LCD_LineStart_4)) || (defined(__LCD_LineStart_3)))
				case (2):
					addr = __LCD_LineStart_3;
					break;
			#endif
			#if (defined(__LCD_LineStart_4))
				case (3):
					addr = __LCD_LineStart_4;
					break;
			#endif
		}
		addr = __LCD_CMD_SetDDRAMAddress | (addr | X);
		LCD_SendCommand(addr);
	}
}


//Print character.
void LCD_PrintChar(char Character)
{
	LCD_SendData(Character);
}

//Print string from SRAM.
void LCD_PrintString(char *Text)
{
	while(*Text)
		LCD_SendData(*Text++);
}

//Print string from Flash memory.
void LCD_PrintString_P(const char *Text)
{
	char r = pgm_read_byte(Text++);
	while(r)
	{
		LCD_SendData(r);
		r = pgm_read_byte(Text++);
	}
}

//Print integer.
void LCD_PrintInteger(int32_t Value)
{
	if (Value == 0 )
	{
		LCD_PrintChar('0');
	}
	else if ((Value > INT32_MIN ) && (Value <= INT32_MAX))
	{
		//int32_max + sign + null = 12 bytes
		char arr[12] = { '\0' };
		
		//Convert integer to array (returns in reversed order)
		Int2bcd(Value, arr);
		
		//Print
		LCD_PrintString(arr);
	}
}

//Print double.
void LCD_PrintDouble(double Value, uint32_t Tens)
{
	if (Value == 0)
	{
		//Print characters individually so no string is stored into RAM.
		LCD_PrintChar('0');
		LCD_PrintChar('.');
		LCD_PrintChar('0');
	}
	else if ((Value >= (-2147483647)) && (Value < 2147483648))
	{
		//Print sign
		if (Value < 0)
		{
			Value = -Value;
			LCD_PrintChar('-');
		}
		
		//Print integer part
		LCD_PrintInteger(Value);
		
		//Print dot
		LCD_PrintChar('.');
		
		//Print decimal part
		LCD_PrintInteger((Value - (uint32_t)(Value)) * Tens);
	}
}

//Send only high nibble to LCD.
static void LCD_SendCommandHigh(uint8_t Data)
{

	// print_to_binary(Data, "LCD_SendCommandHigh");

	MXC_GPIO_OutClr(LCD_PORT, LCD_RS);

	//Send the high nibble
	int i;
    for(i = 4; i < 8; i++){
		if(BitCheck(Data, i)){
			MXC_GPIO_OutSet(LCD_PORT, lcdData[i-4]);
		} else {
			MXC_GPIO_OutClr(LCD_PORT, lcdData[i-4]);
		}
	}
	Pulse_En();
}

//Send data to LCD.
static void LCD_Send(uint8_t Data)
{

	// print_to_binary(Data, "LCD_Send");

	//Send the high nibble
	int i;
    for(i = 4; i < 8; i++){
		if(BitCheck(Data, i)){
			MXC_GPIO_OutSet(LCD_PORT, lcdData[i-4]);
		} else {
			MXC_GPIO_OutClr(LCD_PORT, lcdData[i-4]);
		}
	}
	Pulse_En();

	//Low nibble comes after
    for(i = 0; i < 4; i++){
		if(BitCheck(Data, i)){
			MXC_GPIO_OutSet(LCD_PORT, lcdData[i]);
		} else {
			MXC_GPIO_OutClr(LCD_PORT, lcdData[i]);
		}
	}
	Pulse_En();
}

//Sends pulse to PIN_EN of LCD.
static inline void Pulse_En()
{
	MXC_GPIO_OutSet(LCD_PORT, LCD_EN);
	MXC_Delay(1000);
	MXC_GPIO_OutClr(LCD_PORT, LCD_EN);
	MXC_Delay(1000);
}

//Converts integer value to BCD.
static void Int2bcd(int32_t Value, char BCD[])
{
	uint8_t isNegative = 0;
	
	BCD[0] = BCD[1] = BCD[2] =
	BCD[3] = BCD[4] = BCD[5] =
	BCD[6] = BCD[7] = BCD[8] =
	BCD[9] = BCD[10] = '0';
	
	if (Value < 0)
	{
		isNegative = 1;
		Value = -Value;
	}
	
	while (Value > 1000000000)
	{
		Value -= 1000000000;
		BCD[1]++;
	}
	
	while (Value >= 100000000)
	{
		Value -= 100000000;
		BCD[2]++;
	}
		
	while (Value >= 10000000)
	{
		Value -= 10000000;
		BCD[3]++;
	}
	
	while (Value >= 1000000)
	{
		Value -= 1000000;
		BCD[4]++;
	}
	
	while (Value >= 100000)
	{
		Value -= 100000;
		BCD[5]++;
	}

	while (Value >= 10000)
	{
		Value -= 10000;
		BCD[6]++;
	}

	while (Value >= 1000)
	{
		Value -= 1000;
		BCD[7]++;
	}
	
	while (Value >= 100)
	{
		Value -= 100;
		BCD[8]++;
	}
	
	while (Value >= 10)
	{
		Value -= 10;
		BCD[9]++;
	}

	while (Value >= 1)
	{
		Value -= 1;
		BCD[10]++;
	}

	uint8_t i = 0;
	//Find first non zero digit
	while (BCD[i] == '0')
		i++;

	//Add sign 
	if (isNegative)
	{
		i--;
		BCD[i] = '-';
	}

	//Shift array
	uint8_t end = 10 - i;
	uint8_t offset = i;
	i = 0;
	while (i <= end)
	{
		BCD[i] = BCD[i + offset];
		i++;
	}
	BCD[i] = '\0';
}
//-----------------------------//
