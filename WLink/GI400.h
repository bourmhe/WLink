/* ******************************************************************************** */
/*                                                                                  */
/* GI400.h																			*/
/*                                                                                  */
/* Description :                                                                    */
/*		Header file for GI400.cpp													*/
/*                                                                                  */
/* History :  	29/04/2017  (RW)	Creation of this file							*/
/*                                                                                  */
/* ******************************************************************************** */

#ifndef __GI400_H__
#define __GI400_H__

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */
#include "IndicatorInterface.h"

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */

/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */
const INDICATOR_INTERFACE_FRAME_STRUCT GL_pGI400Frames_X[] = {  { 3, { 'S', 'B', 0x0D }, 9 },        // ASK_WEIGHT
                                                                { 0, { 0x00 }, 0 },		            // ASK_WEIGHT_ALIBI - not supported
                                                                { 0, { 0x00 }, 0 },		            // ASK_LAST_ALIBI - not supported
                                                                { 0, { 0x00 }, 0 },		            // ASK_WEIGHT_MSA - not supported
                                                                { 3, { 'S', 'C', 0x0D }, 0 }			// SET_TO_ZERO
                                                                };

/* ******************************************************************************** */
/* Functions
/* ******************************************************************************** */
void GI400_ProcessFrame(unsigned char * pBuffer_UB, INDICATOR_INTERFACE_FRAME_ENUM Frame_E, INDICATOR_WEIGHT_STRUCT * pWeight_X);

#endif // __GI400_H__
