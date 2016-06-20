/* ******************************************************************************** */
/*                                                                                  */
/* LcdDisplay.h			                                                            */
/*                                                                                  */
/* Description :                                                                    */
/*		Header file for LcdDisplay.cpp												*/
/*                                                                                  */
/* History :  	13/06/2016  (RW)	Creation of this file							*/
/*                                                                                  */
/* ******************************************************************************** */

#ifndef __LCD_DISPLAY_H__
#define __LCD_DISPLAY_H__

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */
#include <Arduino.h>
#include <LiquidCrystal.h>

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */

/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */
typedef struct {
	boolean IsInitialized_B;
} LCD_DISPLAY_PARAM;

typedef enum {
	LCD_DISPLAY_BOTH_LINE,
	LCD_DISPLAY_LINE1,
	LCD_DISPLAY_LINE2
} LCD_DISPLAY_LINE_ENUM;

/* ******************************************************************************** */
/* Class
/* ******************************************************************************** */
class LcdDisplay {
public:
	// Constructor
	LcdDisplay();

	// Functions
	void init(LiquidCrystal * pLcd_H, unsigned char PinBacklight_UB);
	boolean isInitialized(void);

	void clearDisplay(void);
	void writeDisplay(LCD_DISPLAY_LINE_ENUM LineIndex_E, String TextStr_Str);
	void writeDisplay(LCD_DISPLAY_LINE_ENUM LineIndex_E, unsigned char * pTextStr_UB, unsigned long ArraySize_UL);
	void setBacklight(unsigned char Value_UB);

	LCD_DISPLAY_PARAM GL_LcdDisplayParam_X;
};

#endif // __LCD_DISPLAY_H__