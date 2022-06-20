/*
 * lcd.h
 *
 * Created: 2022-06-18 오전 7:47:28
 *  Author: taroz
 */ 

#ifndef LCD_H_
#define LCD_H_

#define LCD_WDATA	PORTA
#define	LCD_WINST	PORTA
#define	LCD_RDATA	PINA
#define LCD_CTRL	PORTG

#define LCD_EN		0
#define LCD_RW		1
#define LCD_RS		2

#define F_CPU	14745600UL

#include <avr/io.h>
#include <util/delay.h>

void LCD_Data(uint8_t ch);
void LCD_Comm(uint8_t ch);

void LCD_Char(uint8_t c);
void LCD_Str(uint8_t *str);

void LCD_Set_DDRAM(uint8_t row, uint8_t col);
void LCD_Function_set(uint8_t dl, uint8_t n, uint8_t f);
void LCD_Cursor_Display_Shift(uint8_t sc, uint8_t rl);
void LCD_Display_on_off(uint8_t d, uint8_t c, uint8_t b);
void LCD_Entry_mode_set(uint8_t id, uint8_t s);
void LCD_Return_home(void);
void LCD_Clear(void);
void LCD_Init(void);

#endif /* LCD_H_ */