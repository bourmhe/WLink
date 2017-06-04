/* ******************************************************************************** */
/*                                                                                  */
/* SerialManager.h																    */
/*                                                                                  */
/* Description :                                                                    */
/*		Header file for SerialManager.cpp										    */
/*		Process specific functions to manage the Serial communication               */
/*                                                                                  */
/* History :	04/06/2017	(RW)	Creation of this file                           */
/*                                                                                  */
/* ******************************************************************************** */

#ifndef __SERIAL_MANAGER_H__
#define __SERIAL_MANAGER_H__

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */

/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */

/* ******************************************************************************** */
/* Functions Prototypes
/* ******************************************************************************** */
void SerialManager_Init();
void SerialManager_SetTunnel(unsigned long ComPortA_UL, unsigned long ComPortB_UL);
void SerialManager_StopTunnel(void);
void SerialManager_Process();


#endif // __SERIAL_MANAGER_H__

