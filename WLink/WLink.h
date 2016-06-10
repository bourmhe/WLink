/* ******************************************************************************** */
/*                                                                                  */
/* WLink.h																			*/
/*                                                                                  */
/* Description :                                                                    */
/*		Header file for W-Link project												*/
/*		Gathers global variables and definitions for the application				*/
/*                                                                                  */
/* History :	14/05/2016	(RW)	Creation of this file                           */
/*                                                                                  */
/* ******************************************************************************** */

#ifndef __WLINK_H__
#define __WLINK_H__

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */

#include <Arduino.h>

#include "Hardware.h"

#include "TCPServer.h"
#include "TCPServerManager.h"

#include "WCommand.h"
#include "WCommandMedium.h"
#include "WCommandInterpreter.h"

#include "Indicator.h"
#include "IndicatorInterface.h"
#include "IndicatorManager.h"

#include "BadgeReader.h"

#include "Debug.h"

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */
#define NOP	(void (*)());

/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */
typedef struct {
	void(*FctHandler)(void);
} COM_EVENT_FCT_STRUCT;

typedef struct {
	//UDPServer UdpServer_H;
	TCPServer TcpServer_H;
} NETWORK_INTERFACE_STRUCT;

typedef struct {
	unsigned char pRevisionId_UB[8];
	unsigned char LedPin_UB = PIN_BLINK_LED;
	const unsigned char pGpioInputIndex_UB[4] = { PIN_GPIO_INPUT0, PIN_GPIO_INPUT1, PIN_GPIO_INPUT2, PIN_GPIO_INPUT3 };
	const unsigned char pGpioOutputIndex_UB[4] = { PIN_GPIO_OUTPUT0, PIN_GPIO_OUTPUT1, PIN_GPIO_OUTPUT2, PIN_GPIO_OUTPUT3 };
	NETWORK_INTERFACE_STRUCT NetworkIf_X;
	Indicator Indicator_H;
	BadgeReader BadgeReader_H;
} GLOBAL_PARAM_STRUCT;


#endif // __WLINK_H__