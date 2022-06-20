/*
 * MCU.c
 *
 * Created: 2022-06-15 오후 3:07:47
 * Author : YouBeen Kim(t.a.rozemyne@gmail.com)
 */


#define F_CPU	14745600UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "Include/lcd.h"

#define CLCD_POS			0x2b
#define CLCD_NEG			0x2d

#define CLCD_Star			0x2a
#define CLCD_SHARP			0x22

#define SERVO_SPEED_FLOAT	128

struct FLAGSYSTEM {
	union {	
		struct {
			uint8_t fPOWERON : 1;
			uint8_t fSETUP : 1;
			uint8_t fREADY : 1;
			uint8_t fBOOK : 1;
			uint8_t fRUN : 1;
			uint8_t fPASSWD : 1;
			uint8_t fLOCK : 1;
			uint8_t fSTOP : 1;
		};
		uint8_t state;
	};
};

// flag CLCD DRAW
struct FLAGLCD {
	union {
		struct {
			uint8_t fDISPLAY_REDRAW : 1;
			uint8_t fRUN : 1;
			uint8_t fFAN : 1;
			uint8_t fSERVO : 1;
			uint8_t fLOCK : 1;
			uint8_t fUNLOCK: 1;
			uint8_t fPASSWD: 1;
			uint8_t fBOOK : 1;
		};
		uint8_t lcd;
	};
};

struct FANCTRL{
	uint8_t FAN_MODE;
	uint8_t FAN_PWM[4];
};
	
struct SERVOCTRL{
	uint16_t SERVO_INT;
	uint16_t SERVO_FLOAT;
	
	uint16_t SERVO_H_INT[2];
	uint16_t SERVO_H_FLOAT[2];
	uint16_t SERVO_L_INT[2];
	uint16_t SERVO_L_FLOAT[2];
	
	uint8_t SERVO_MODE;
	uint8_t SERVO_DIR;
};

struct BUTTON{
	uint8_t BUTTON_ENABLE;
	uint8_t BUTTON_CURRENT;
	uint8_t BUTTON_PAST;
};

struct KEYPAD{
	uint8_t KEYPAD_ENALBE;
	
	uint8_t KEYPAD_CURRENT;
	uint8_t KEYPAD_PAST;
	uint8_t KEYPAD_NUM;
	
	uint8_t BUFFER_IDX;
	uint8_t KEYPAD_BUFFER[4];
	uint8_t PASSWD[4];
	
	uint8_t KEYPAD_LINE[4];
};

struct TIMER {
	union {
		struct {
			uint8_t BOOK_10ms;
			uint8_t BOOK_1m;
			uint8_t BOOK_1s;
			uint8_t SW_20ms;
			uint8_t SW_1s;
			uint8_t RUN_10ms;
			uint8_t LOCK_10ms;
			uint8_t LOCK_PAST;
		};
			
	};
	uint64_t ALL;			
};


struct CLCD_VAR{
	uint8_t sRUN;
	uint8_t sFAN[8];
	uint8_t sSERVO[7];
	uint8_t sLOCK[7];
	uint8_t sUNLOCK[9];
	uint8_t sPASSWD[9];
	uint8_t sBOOK[6];
};

struct FLAGSYSTEM	fSYSTEM		=	{ .state = 0x00 };
struct FLAGLCD		fLCD		=	{ .lcd = 0x00	};
struct FLAGLCD		tmpfLCD		=	{ .lcd = 0x00	};
struct FANCTRL		fanCTRL		=	{ .FAN_MODE = 0	, .FAN_PWM = {1, 63, 127, 255} };
struct SERVOCTRL	servoCTRL	=	{ .SERVO_MODE = 3 , .SERVO_DIR = 0 , .SERVO_INT = 85 , .SERVO_FLOAT = 400 , .SERVO_H_INT   = {143, 115} , .SERVO_H_FLOAT = {0, 200} , .SERVO_L_INT = {27, 56} , .SERVO_L_FLOAT = {800, 600} };
struct BUTTON		cBUTTON		=	{ .BUTTON_ENABLE = 0x00 , .BUTTON_CURRENT = 0x00 , .BUTTON_PAST = 0x00 };
struct KEYPAD		cKEYPAD		=	{ .KEYPAD_ENALBE = 0x00 , .KEYPAD_NUM = 0 , .BUFFER_IDX = 0 , .KEYPAD_CURRENT = 0x00 , .KEYPAD_PAST = 0x00 , .KEYPAD_LINE = {0x0e, 0x0d, 0x0b, 0x07} , .KEYPAD_BUFFER = {CLCD_Star, CLCD_Star, CLCD_Star, CLCD_Star} , .PASSWD = {0x32, 0x30, 0x30, 0x35} };
struct TIMER		nTIMER		=	{ .ALL = 0x00000000 };
struct CLCD_VAR		sCLCD		=	{ .sRUN = CLCD_POS, .sFAN = "SPEED:@", .sSERVO = "MODE:$", .sLOCK = "LOCKED", .sUNLOCK = "UNLOCKED", .sPASSWD = "KEY:****", .sBOOK = "%%sec"};	

void myDelay_us(uint16_t delay)
{
	int i;
	for (i = 0; i < delay; i++)
	{
		_delay_us(1);
	}
}

void SSound(int time)
{
	
	int i, tim;
	tim = 10000 / time;
	for(i = 0; i < tim; i++)
	{
		PORTG |= (1 << PG4);
		myDelay_us(time);
		PORTG &= ~(1 << PG4);
		myDelay_us(time);
	}
	
	PORTG |= (1 << PG4);
}

void servo_set(uint8_t idx)
{
	if(servoCTRL.SERVO_DIR == 0)
	{
		servoCTRL.SERVO_FLOAT = servoCTRL.SERVO_FLOAT + SERVO_SPEED_FLOAT;
		servoCTRL.SERVO_INT = servoCTRL.SERVO_INT + (servoCTRL.SERVO_FLOAT / 1000);
		servoCTRL.SERVO_FLOAT = servoCTRL.SERVO_FLOAT % 1000;
		
		if((servoCTRL.SERVO_INT <= servoCTRL.SERVO_H_INT[idx]) || (servoCTRL.SERVO_FLOAT <= servoCTRL.SERVO_H_FLOAT[idx]))
		{
			OCR3B = servoCTRL.SERVO_INT;
		}
		else 
		{
			servoCTRL.SERVO_DIR = 1;
		}
		
	}
	
	if(servoCTRL.SERVO_DIR == 1)
	{
		if(servoCTRL.SERVO_FLOAT < SERVO_SPEED_FLOAT)
		{
			servoCTRL.SERVO_INT -= 1;
			servoCTRL.SERVO_FLOAT += 1000;
		}
		
		servoCTRL.SERVO_FLOAT = servoCTRL.SERVO_FLOAT - SERVO_SPEED_FLOAT;
		servoCTRL.SERVO_INT = servoCTRL.SERVO_INT + (servoCTRL.SERVO_FLOAT / 1000);
		servoCTRL.SERVO_FLOAT = servoCTRL.SERVO_FLOAT % 1000;
		
		if((servoCTRL.SERVO_INT >= servoCTRL.SERVO_L_INT[idx]) || (servoCTRL.SERVO_FLOAT >= servoCTRL.SERVO_L_FLOAT[idx]))
		{
			OCR3B = servoCTRL.SERVO_INT;
		}
		else
		{
			servoCTRL.SERVO_DIR = 0;
		}
		
	}
}

void servo_deg(void)
{
	if(servoCTRL.SERVO_MODE == 3)
	{
		OCR3B = servoCTRL.SERVO_INT;
	}
	else if(servoCTRL.SERVO_MODE == 2)
	{
		servo_set(1);	
	}
	else if(servoCTRL.SERVO_MODE == 1)
	{
		servo_set(0);
	}
}

void servo_mode(void)
{
	if(servoCTRL.SERVO_MODE == 3)
	{
		servoCTRL.SERVO_MODE = 1;
	}
	else if(servoCTRL.SERVO_MODE == 1)
	{
		servoCTRL.SERVO_MODE = 2;
	}
	else if(servoCTRL.SERVO_MODE == 2)
	{
		servoCTRL.SERVO_MODE = 3;
	}
}

void fan_pwm(void)
{
	sprintf(sCLCD.sFAN, "SPEED:%01d", fanCTRL.FAN_MODE);
	switch(fanCTRL.FAN_MODE)
	{
		case 0:
			TCCR2 &= ~(1 << COM21);
			break;
		case 1:
			OCR2 = fanCTRL.FAN_PWM[1];
			break;
		case 2:
			OCR2 = fanCTRL.FAN_PWM[2];
			break;
		case 3:
			OCR2 = fanCTRL.FAN_PWM[3];
			break;
	}
}

void fan_mode(void)
{
	switch(fanCTRL.FAN_MODE)
	{
		case 0:
		{
			TCCR2 |= (1 << COM21);
			fanCTRL.FAN_MODE = 1;
			break;
		}
		
		case 1:
		{
			fanCTRL.FAN_MODE = 2;
			break;
		}
		
		case 2:
		{
			fanCTRL.FAN_MODE = 3;
			break;
		}
		
		case 3:
		{
			fanCTRL.FAN_MODE = 1;
			break;
		}
	}	
}

void FanStop(void)
{
	nTIMER.ALL = 0x00000000;

	fanCTRL.FAN_MODE = 0;
	TCCR2 &= ~(1 << COM21);
	servoCTRL.SERVO_MODE = 3;
	servoCTRL.SERVO_DIR = 0;
	servo_deg();
	
	sprintf(sCLCD.sSERVO, "MODE:%01d", servoCTRL.SERVO_MODE);
	sprintf(sCLCD.sFAN, "SPEED:%01d", fanCTRL.FAN_MODE);
	sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
	
	cKEYPAD.KEYPAD_ENALBE = 0x00;
	cBUTTON.BUTTON_ENABLE = 0xf8;
	
	fLCD.lcd = 0x00;
	
	fLCD.fRUN = 1;
	fLCD.fSERVO = 1;
	
	fSYSTEM.fBOOK = 0;
	fSYSTEM.fSTOP = 1;
	fSYSTEM.fRUN = 0;
	
	fLCD.fDISPLAY_REDRAW = 1;
}

uint8_t KeyScan(void)
{
	uint16_t Key_Scan_Line_Sel = 0xfe;
	uint8_t Key_Scan_Sel = 0;
	uint8_t key_scan_num = 0;
	uint8_t Get_Key_Data = 0;
	uint8_t Temp_Get_Key_Data = 0;
	
	for(Key_Scan_Sel = 0; Key_Scan_Sel < 4; Key_Scan_Sel++)
	{
		PORTC = Key_Scan_Line_Sel;
		_delay_us(10);
		
		Temp_Get_Key_Data = (PINC & 0xf0);
		
		if(Temp_Get_Key_Data != 0x00)
		{
			_delay_ms(1);
			Get_Key_Data = (PINC & 0xf0);
			
			switch(Get_Key_Data)
			{
				case 0x10:
					key_scan_num = Key_Scan_Sel * 4 + 1;
					break;
				case 0x20:
					key_scan_num = Key_Scan_Sel * 4 + 2;
					break;
				case 0x40:
					key_scan_num = Key_Scan_Sel * 4 + 3;
					break;
				case 0x80:
					key_scan_num = Key_Scan_Sel * 4 + 4;
					break;
				default:
					key_scan_num = 17;
					break;
			}
			return key_scan_num;
		}
		Key_Scan_Line_Sel = (Key_Scan_Line_Sel << 1) | 0x01;
	}
	return key_scan_num;
}

void keypad_in()
{
	if(cKEYPAD.BUFFER_IDX < 4)
	{
		SSound(130);
		cKEYPAD.KEYPAD_BUFFER[cKEYPAD.BUFFER_IDX] = cKEYPAD.KEYPAD_NUM;
		LCD_Char(cKEYPAD.KEYPAD_BUFFER[cKEYPAD.BUFFER_IDX]);
		cKEYPAD.BUFFER_IDX += 1;
	}
	
	if(cKEYPAD.BUFFER_IDX == 4)
	{
		
		if((cKEYPAD.PASSWD[0] == cKEYPAD.KEYPAD_BUFFER[0]) && (cKEYPAD.PASSWD[1] == cKEYPAD.KEYPAD_BUFFER[1]) && (cKEYPAD.PASSWD[2] ==	cKEYPAD.KEYPAD_BUFFER[2]) && (cKEYPAD.PASSWD[3] ==	cKEYPAD.KEYPAD_BUFFER[3]))
		{
			cBUTTON.BUTTON_ENABLE = 0xf8;
			
			fSYSTEM.fLOCK = 0;
			
			nTIMER.LOCK_10ms = 0;
			fLCD.fUNLOCK = 1;
		}
		else 
		{	
			cBUTTON.BUTTON_ENABLE = 0x10;
		
			fSYSTEM.fLOCK = 1;
			fSYSTEM.fPASSWD = 0;
		}
		
		LCD_Display_on_off(1, 0, 0);
		LCD_Clear();
		
		cKEYPAD.KEYPAD_ENALBE = 0x00;
		
		cKEYPAD.KEYPAD_BUFFER[0] = CLCD_Star;
		cKEYPAD.KEYPAD_BUFFER[1] = CLCD_Star;
		cKEYPAD.KEYPAD_BUFFER[2] = CLCD_Star;
		cKEYPAD.KEYPAD_BUFFER[3] = CLCD_Star;
		cKEYPAD.BUFFER_IDX = 0;
	}
}

uint8_t Key_decode(uint16_t New_key_data)
{
	switch(New_key_data)
	{
		case 1 : cKEYPAD.KEYPAD_NUM = 0x31; break;
		case 2 : cKEYPAD.KEYPAD_NUM = 0x32; break;
		case 3 : cKEYPAD.KEYPAD_NUM = 0x33; break;
		case 4 : cKEYPAD.KEYPAD_NUM = 10; break;
		case 5 : cKEYPAD.KEYPAD_NUM = 0x34; break;
		case 6 : cKEYPAD.KEYPAD_NUM = 0x35; break;
		case 7 : cKEYPAD.KEYPAD_NUM = 0x36; break;
		case 8 : cKEYPAD.KEYPAD_NUM = 11; break;
		case 9 : cKEYPAD.KEYPAD_NUM = 0x37; break;
		case 10 : cKEYPAD.KEYPAD_NUM = 0x38; break;
		case 11 : cKEYPAD.KEYPAD_NUM = 0x39; break;
		case 12 : cKEYPAD.KEYPAD_NUM = 12; break;
		case 13 : 
		{
			cKEYPAD.KEYPAD_NUM = CLCD_Star;
			cKEYPAD.KEYPAD_BUFFER[0] = CLCD_Star;
			cKEYPAD.KEYPAD_BUFFER[1] = CLCD_Star;
			cKEYPAD.KEYPAD_BUFFER[2] = CLCD_Star;
			cKEYPAD.KEYPAD_BUFFER[3] = CLCD_Star;
			cKEYPAD.BUFFER_IDX = 0;
			fLCD.fPASSWD = 1;
			break;
		}
		case 14 : cKEYPAD.KEYPAD_NUM = 0x30; break;
		case 15 : cKEYPAD.KEYPAD_NUM = CLCD_SHARP; break;
		case 16 : cKEYPAD.KEYPAD_NUM = 13; break;
	}
	return cKEYPAD.KEYPAD_NUM;
}

ISR(TIMER0_COMP_vect)
{
	nTIMER.BOOK_10ms += 1;
	
	if(fSYSTEM.fRUN == 1)
	{
		nTIMER.RUN_10ms += 1;
		
		if(nTIMER.RUN_10ms == 200)
		{
			if(sCLCD.sRUN == CLCD_POS)
			{
				sCLCD.sRUN = CLCD_NEG;
			}else {
				sCLCD.sRUN = CLCD_POS;
			}
			nTIMER.RUN_10ms = 0;
		}
	}
	
	if(fLCD.fLOCK == 1)
	{
		nTIMER.LOCK_10ms += 1;
		
		if(nTIMER.LOCK_10ms == 100)
		{
			nTIMER.LOCK_10ms = 0;
			fLCD.fLOCK = 0;
			fLCD.fDISPLAY_REDRAW = 1;
		}
	}
	
	if(fLCD.fUNLOCK == 1)
	{
		nTIMER.LOCK_10ms += 1;
		
		if(nTIMER.LOCK_10ms == 100)
		{
			nTIMER.LOCK_10ms = 0;
			fLCD.fUNLOCK = 0;
			fSYSTEM.fPASSWD = 0;
			fLCD.fDISPLAY_REDRAW = 1;
		}
	}
	
	if(nTIMER.BOOK_10ms == 100)
	{
		nTIMER.BOOK_10ms = 0;
		
		if(fLCD.fBOOK == 1)
		{
			if(nTIMER.BOOK_1s == 0)
			{
				//if(nTIMER.BOOK_1m > 0)
				//{
					//nTIMER.BOOK_1s = 59;
					//nTIMER.BOOK_1m -= 1;
					//sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
					//fLCD.fDISPLAY_REDRAW = 1;
				//}
				//else if(nTIMER.BOOK_1m == 0)
				//{
					
					LCD_Clear();
					fSYSTEM.fSTOP = 0;
					fSYSTEM.fRUN = 0;
					FanStop();		
				//}
			}
			else 
			{
				nTIMER.BOOK_1s -= 1;
				sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
			}
		}
	}
}

ISR(TIMER3_COMPA_vect)
{
	cBUTTON.BUTTON_CURRENT = ((~(PIND & 0xf8)) & cBUTTON.BUTTON_ENABLE);
		
	if((cBUTTON.BUTTON_PAST == 0x00) && (cBUTTON.BUTTON_CURRENT != 0x00))
	{
		SSound(130);
		
		if(fSYSTEM.fSETUP == 1)
		{
			fSYSTEM.fSETUP = 0;
			fSYSTEM.fREADY = 1;
		}
		else
		{	
			switch(cBUTTON.BUTTON_CURRENT)
			{
				case 0x80 :
				{
					if(fSYSTEM.fSTOP == 1)
					{
						fSYSTEM.fSTOP = 0;
						fSYSTEM.fRUN = 1;
					}
					servo_mode();
					
					sprintf(sCLCD.sSERVO, "MODE:%01d", servoCTRL.SERVO_MODE);
					
					break;
				}
				
				case 0x40 :
				{
					LCD_Clear();
					
					fSYSTEM.fSTOP = 0;
					fSYSTEM.fRUN = 0;
					
					FanStop();
					break;
				}
				
				case 0x20 :
				{
					if(fSYSTEM.fSTOP == 1)
					{
						fSYSTEM.fSTOP = 0;
						fSYSTEM.fRUN = 1;
					}
	
					fan_mode();
					fan_pwm();
					break;
				}
				
				case 0x10 :
				{	
					
					
					nTIMER.SW_20ms = 0;
					nTIMER.LOCK_PAST = 1;
					break;
				}
				
				case 0x08 :
				{
					
					nTIMER.SW_20ms = 0;
					nTIMER.LOCK_PAST = 1;
					
					if(fSYSTEM.fBOOK == 0)
					{
						fSYSTEM.fBOOK = 1;
						fLCD.fBOOK = 1;
						
						nTIMER.BOOK_10ms = 0;
						
					}
					
					nTIMER.BOOK_1s += 5;

					sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
					
					if (nTIMER.BOOK_1s >= 90)// && (nTIMER.BOOK_1s > 0))
					{
						nTIMER.BOOK_1s = 0;
						//nTIMER.BOOK_1m = 0;
						
						sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
						
						fSYSTEM.fBOOK = 0;
						fLCD.fBOOK = 0;
						fLCD.fDISPLAY_REDRAW = 1;
					}
					break;
				}
				
				default:
					break;
			}

			cBUTTON.BUTTON_PAST = cBUTTON.BUTTON_CURRENT;
		}
	}
	else if ((cBUTTON.BUTTON_PAST == cBUTTON.BUTTON_CURRENT) && (cBUTTON.BUTTON_CURRENT != 0x00)) 
	{		
		if(cBUTTON.BUTTON_CURRENT == 0x10)
		{	
			if(nTIMER.LOCK_PAST == 1)		
			{
				nTIMER.SW_20ms += 1;
			}
			
			if(nTIMER.SW_20ms == 50)
			{
				nTIMER.SW_20ms = 0;
				nTIMER.SW_1s += 1;
			}
				
			if((nTIMER.SW_1s >= 3) && (fSYSTEM.fLOCK == 0))
			{
				cBUTTON.BUTTON_ENABLE = 0x10;
				
				nTIMER.SW_20ms = 0;
				nTIMER.SW_1s = 0;
				nTIMER.LOCK_PAST = 0;
				
				fSYSTEM.fLOCK = 1;
				fSYSTEM.fPASSWD = 0;
				
				fLCD.fLOCK = 1;
				fLCD.fPASSWD = 0;
				fLCD.fUNLOCK = 0;
		
				fLCD.fDISPLAY_REDRAW = 1;					
			}
				
			if((nTIMER.SW_1s >= 10) && (fSYSTEM.fLOCK == 1))
			{
				cBUTTON.BUTTON_ENABLE = 0x00;
				cKEYPAD.KEYPAD_ENALBE = 0xf0;
				
				nTIMER.SW_20ms = 0;
				nTIMER.SW_1s = 0;
				nTIMER.LOCK_PAST = 0;
				
				fSYSTEM.fLOCK = 0;
				fSYSTEM.fPASSWD = 1;
				
				fLCD.fLOCK = 0;
				fLCD.fPASSWD = 1;
				fLCD.fUNLOCK = 0;
			}
		}
			
		if(cBUTTON.BUTTON_CURRENT == 0x08)
		{			
			if(nTIMER.LOCK_PAST == 1)
			{
				nTIMER.SW_20ms += 1;
			}
			
			if(nTIMER.SW_20ms == 50)
			{
				nTIMER.SW_20ms = 0;
				nTIMER.SW_1s += 1;
			}
			
			if((nTIMER.SW_1s >= 3))
			{
				fLCD.fBOOK = 0;
				nTIMER.SW_20ms = 0;
				nTIMER.SW_1s = 0;
				nTIMER.LOCK_PAST = 0;
					
				nTIMER.BOOK_1s = 0;
				nTIMER.BOOK_1m = 0;
				
				sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
				
				fSYSTEM.fBOOK = 0;
				fLCD.fBOOK = 0;
				
				fLCD.fDISPLAY_REDRAW = 1;	
			}
		}
	}
	else if((cBUTTON.BUTTON_PAST != 0x00) && (cBUTTON.BUTTON_CURRENT == 0x00))
	{
		nTIMER.SW_20ms = 0;
		nTIMER.LOCK_PAST = 0;
		cBUTTON.BUTTON_PAST = 0x00;
	}
}

ISR(TIMER3_COMPB_vect)
{
	servo_deg();
}

ISR(TIMER3_COMPC_vect)
{
	if(cKEYPAD.KEYPAD_ENALBE == 0xf0)
	{
		cKEYPAD.KEYPAD_CURRENT = KeyScan();
		
		if((cKEYPAD.KEYPAD_CURRENT != 0) && (cKEYPAD.KEYPAD_PAST == 0))
		{
			cKEYPAD.KEYPAD_NUM = Key_decode(cKEYPAD.KEYPAD_CURRENT);
			keypad_in();
		}
		
		cKEYPAD.KEYPAD_PAST = cKEYPAD.KEYPAD_CURRENT;
	}
}

// 8bit timer prescaler 1/1024 CTC 14 10ms
void Init_Timer0(void)
{
	TCCR0 = (1 << WGM01) | (0 << WGM00) | (1 << CS02) | (1 << CS01) | (1 << CS00);
	OCR0 = 143;
	TIMSK |= (1 << OCIE0);
}

// 8bit timer prescaler 1/8 fast PWM
void Init_Timer2(void)
{
	TCCR2 = (1 << WGM21) | (1 << WGM20) | (1 << CS21);
	OCR2 = 72;
}

// 16bit timer prescaler 1/256 fast PWM 1us = 0.0576 1ms = 57.6 10ms = 576 1s = 57600
void Init_Timer3(void)
{
	TCCR3A = (1 << WGM31) | (1 << WGM30);
	TCCR3B = (1 << WGM33) | (1 << WGM32) | (1 << CS32);
	OCR3A = 1151; // 20ms 
	OCR3B = 85;
	OCR3C = 977;
	ETIMSK |= (1 << OCIE3A) | (1 << OCIE3B) | (1 << OCIE3C);
}

void Init_Timer(void)
{
	Init_Timer0();
	Init_Timer2();
	Init_Timer3();
	
	sei();
}

void Init_Port(void)
{
	DDRA = 0xff;
	DDRB = (1 << PB7);
	DDRC = 0x0f;
	DDRD = 0x00;
	DDRE = (1 << PE4);
	DDRG = 0x17;
	
	PORTG = 0x10;
}

void CLCD0_DISPLAY(void)
{
	LCD_Set_DDRAM(0, 1);
	LCD_Str(sCLCD.sFAN);
	
	LCD_Set_DDRAM(0, 9);
	LCD_Str(sCLCD.sSERVO);
}

void CLCD1_DISPLAY(void)
{
	if(fSYSTEM.fLOCK == 1)
	{
		LCD_Set_DDRAM(1, 1);
		LCD_Str(sCLCD.sLOCK);
	}
	
	if(fLCD.fBOOK == 1)
	{
		LCD_Set_DDRAM(1, 9);
		LCD_Str(sCLCD.sBOOK);
	}
	
	if(fLCD.fRUN == 1)
	{
		LCD_Set_DDRAM(1, 15);
		LCD_Char(sCLCD.sRUN);
	}
}

int main(void)
{
	if (fSYSTEM.state == 0x00)
	{
		fSYSTEM.fPOWERON = 1;
		cBUTTON.BUTTON_ENABLE = 0x00;
		cKEYPAD.KEYPAD_ENALBE = 0x00;
	}
	
	if (fSYSTEM.fPOWERON == 1)
	{
		Init_Port();
		
		_delay_ms(15);
		LCD_Init();
		
		Init_Timer();
		
		fSYSTEM.fPOWERON = 0;
		fSYSTEM.fSETUP = 1;	
	}
	
	if (fSYSTEM.fSETUP == 1)
	{
		LCD_Function_set(1, 1, 0);
		LCD_Display_on_off(0, 0, 0);
		LCD_Clear();
		LCD_Entry_mode_set(1, 0);
		
		// LCD 및 서보모터 위치 초기화.
		TCCR3A |= (1 << COM3B1);
		
		fanCTRL.FAN_MODE = 0;
		servoCTRL.SERVO_MODE = 3;
		servoCTRL.SERVO_DIR = 0;
		
		sprintf(sCLCD.sLOCK, "LOCKED");
		sprintf(sCLCD.sUNLOCK, "UNLOCKED");
		sprintf(sCLCD.sSERVO, "MODE:%01d", servoCTRL.SERVO_MODE);
		sprintf(sCLCD.sFAN, "SPEED:%01d", fanCTRL.FAN_MODE);
		sprintf(sCLCD.sBOOK, "%02dsec", nTIMER.BOOK_1s);
		
		// 선풍기 작동 버튼 = fSETUP이 1 인 상태에서 STOP을 제외한 첫 버튼입력은 선풍기 작동을 알리고 기능은 동작 안하도록 작성.
		cBUTTON.BUTTON_ENABLE = 0xb8;
	}
	
	while (1)
	{	
		if(fLCD.fDISPLAY_REDRAW == 1)
		{
			LCD_Clear();
			fLCD.fDISPLAY_REDRAW = 0;
		}
		
		if (fSYSTEM.fREADY == 1)
		{
			fLCD.fFAN = 1;
			fLCD.fSERVO = 1;
			
			LCD_Display_on_off(1, 0, 0);
			
			fSYSTEM.fREADY = 0;
			fSYSTEM.fSTOP = 1;
			
			cBUTTON.BUTTON_ENABLE = 0xf8;
		}
		
		if ((fSYSTEM.fRUN == 1) || (fSYSTEM.fSTOP == 1))
		{
			if((fSYSTEM.fRUN == 1) && (fLCD.fRUN == 0))
			{
				fLCD.fRUN = 1;
			}
			
			if(fSYSTEM.fSTOP == 1)
			{
				fLCD.fRUN = 0;
				
			}
			
			if ((fLCD.fLOCK == 0) && (fSYSTEM.fPASSWD == 0) && (fLCD.fUNLOCK == 0))
			{	
				CLCD0_DISPLAY();
				CLCD1_DISPLAY();
			}
			
			if (fSYSTEM.fLOCK == 1)
			{
				if (fLCD.fLOCK == 1)
				{	
					LCD_Set_DDRAM(1, 5);
					LCD_Str(sCLCD.sLOCK);
				}
			}
			
			if (fSYSTEM.fPASSWD == 1)
			{
				if (fLCD.fPASSWD == 1)
				{
					LCD_Display_on_off(1, 1, 0);
					LCD_Clear();
					LCD_Set_DDRAM(1, 4);
					LCD_Str(sCLCD.sPASSWD);
					LCD_Cursor_Display_Shift(0, 0);
					LCD_Cursor_Display_Shift(0, 0);
					LCD_Cursor_Display_Shift(0, 0);
					LCD_Cursor_Display_Shift(0, 0);
					fLCD.fPASSWD = 0;
				}
				
				if (fLCD.fUNLOCK == 1)
				{
					LCD_Set_DDRAM(1, 4);
					LCD_Str(sCLCD.sUNLOCK);
				}
			}
		}
	}
}