/*
 * lcd.c
 *
 * Created: 2022-06-18 오전 7:46:35
 * Author : taroz
 */ 

#include "lcd.h"

void LCD_Data(uint8_t ch)
{
	LCD_CTRL |= (1 << LCD_RS);
	LCD_CTRL &= ~(1 << LCD_RW);
	LCD_CTRL |= (1 << LCD_EN);
	_delay_us(50);
	LCD_WINST = ch;
	_delay_us(50);
	LCD_CTRL &= ~(1 << LCD_EN);
}

void LCD_Comm(uint8_t ch)
{
	LCD_CTRL &= ~(1 << LCD_RS);
	LCD_CTRL &= ~(1 << LCD_RW);
	LCD_CTRL |= (1 << LCD_EN);
	_delay_us(50);
	LCD_WINST = ch;
	_delay_us(50);
	LCD_CTRL &= ~(1 << LCD_EN);
}

void LCD_Char(uint8_t ch)
{
	LCD_Data(ch);
	_delay_us(50);
}

void LCD_Str(uint8_t *str)
{
	while(*str != 0)
	{
		LCD_Char(*str);
		str++;
	}
}

void LCD_Set_DDRAM(uint8_t row, uint8_t col)
{
	LCD_Comm(0x80 | ((row * 0x40) + col));
}

void LCD_Function_set(uint8_t dl, uint8_t n, uint8_t f)
{
	LCD_Comm(0x20 | ((dl * 0x10) + (n * 0x08) + (f * 0x04)));
	_delay_us(1000);
}

void LCD_Cursor_Display_Shift(uint8_t sc, uint8_t rl)
{
	LCD_Comm(0x10 | ((sc * 0x08) + (rl * 0x04)));
	_delay_us(50);
}


void LCD_Display_on_off(uint8_t d, uint8_t c, uint8_t b)
{
	LCD_Comm(0x08 | ((d * 0x04) + (c * 0x02) + (b * 0x01)));
	_delay_us(50);
}

void LCD_Entry_mode_set(uint8_t id, uint8_t s)
{
	LCD_Comm(0x04 | ((id * 0x02) + (s * 0x01)));
	_delay_us(50);
}

void LCD_Return_home(void)
{
	LCD_Comm(0x02);
	_delay_us(2000);
}

void LCD_Clear(void)
{
	LCD_Comm(0x01);
	_delay_us(2000);
}

void LCD_Init(void)
{
	LCD_Comm(0x30);
	_delay_us(4100);
	
	LCD_Comm(0x30);
	_delay_us(100);
	
	LCD_Comm(0x30);
	_delay_us(100);
}