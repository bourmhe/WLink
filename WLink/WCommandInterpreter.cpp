/* ******************************************************************************** */
/*                                                                                  */
/* WCommandIntepreter.cpp															*/
/*                                                                                  */
/* Description :                                                                    */
/*		Describes the state machine to manage the commands processing               */
/*                                                                                  */
/* History :  	16/02/2015  (RW)	Creation of this file                           */
/*				14/05/2016	(RW)	Re-mastered version								*/
/*				08/10/2016	(RW)	Remove SendNack state and functions				*/
/*									Manage only SendResp and add status byte		*/
/*				01/01/2016	(RW)	Fix bug : number of bytes in response was not	*/
/*									sent											*/
/*                                                                                  */
/* ******************************************************************************** */

#define MODULE_NAME		"WCommandInterpreter"

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */

#include "WCommand.h"
#include "WCommandMedium.h"
#include "WCommandInterpreter.h"

#include "Debug.h"

/* ******************************************************************************** */
/* Local Variables
/* ******************************************************************************** */
enum WCMD_INTERPRETER_STATE {
	WCMD_INTERPRETER_STATE_IDLE,
    WCMD_INTERPRETER_STATE_WAIT_PACKET,
	WCMD_INTERPRETER_STATE_CHECK_CMD,
	WCMD_INTERPRETER_STATE_PROCESS_CMD,
	WCMD_INTERPRETER_STATE_SEND_RESP
};

static WCMD_INTERPRETER_STATE GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_IDLE;

static const WCMD_FCT_DESCR * GL_pWCmdFctDescr_X;
static unsigned long GL_WCmdFctNb_UL;

typedef struct {
	unsigned char CmdId_UB;
	boolean HasParam_B;
	unsigned long ParamNb_UL;
	unsigned long AnsNb_UL;
	unsigned char pParamBuffer_UB[WCMD_MAX_PARAM_NB];
	unsigned char pAnsBuffer_UB[WCMD_MAX_ANS_NB];
	WCMD_FCT_STS FctSts_E;
} WCMD_PARAM_STRUCT;

static WCMD_PARAM_STRUCT GL_WCmdParam_X;

/* ******************************************************************************** */
/* Prototypes for Internal Functions
/* ******************************************************************************** */
static void ProcessIdle(void);
static void ProcessWaitPacket(void);
static void ProcessCheckCmd(void);
static void ProcessProcessCmd(void);
static void ProcessSendResp(void);

static void TransitionToIdle(void);
static void TransitionToWaitPacket(void);
static void TransitionToCheckCmd(void);
static void TransitionToProcessCmd(void);
static void TransitionToSendResp(void);


/* ******************************************************************************** */
/* Functions
/* ******************************************************************************** */

void WCommandInterpreter_Init(const WCMD_FCT_DESCR *pFctDescr_X, unsigned long NbFct_UL) {
	GL_pWCmdFctDescr_X = pFctDescr_X;
	GL_WCmdFctNb_UL = NbFct_UL;
	DBG_PRINTLN(DEBUG_SEVERITY_INFO, "W-Command Interpreter Initialized");
}

void WCommandInterpreter_Process() {
    
    /* Reset Condition */
    if (!WCmdMedium_IsRunning() && (GL_WCommandInterpreter_CurrentState_E != WCMD_INTERPRETER_STATE_IDLE))
        TransitionToIdle();

    /* State Machine */ 
	switch (GL_WCommandInterpreter_CurrentState_E) {
		case WCMD_INTERPRETER_STATE_IDLE :
			ProcessIdle();
			break;

        case WCMD_INTERPRETER_STATE_WAIT_PACKET :
            ProcessWaitPacket();
            break;

		case WCMD_INTERPRETER_STATE_CHECK_CMD :
			ProcessCheckCmd();
			break;

		case WCMD_INTERPRETER_STATE_PROCESS_CMD :
			ProcessProcessCmd();
			break;

		case WCMD_INTERPRETER_STATE_SEND_RESP :
			ProcessSendResp();
			break;

	}
}

/* ******************************************************************************** */
/* Internal Functions
/* ******************************************************************************** */

void ProcessIdle(void) {
	if (WCmdMedium_IsRunning())
		TransitionToWaitPacket();
}

void ProcessWaitPacket(void) {
    if (WCmdMedium_IsPacketReceived())
        TransitionToCheckCmd();
}

void ProcessCheckCmd(void) {
	boolean FoundStx_B = false;
	unsigned char Temp_UB = 0x00;

	while (WCmdMedium_DataAvailable()) {
		// Look for the Start Of Transmit byte
		if (WCmdMedium_ReadByte() == WCMD_STX) {
			FoundStx_B = true;
			DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Found STX");
			break;
		}
	}

	if (FoundStx_B && (WCmdMedium_DataAvailable() > 0)) {

		// Get Command ID and Parameters bit
		Temp_UB = WCmdMedium_ReadByte();
		GL_WCmdParam_X.CmdId_UB = Temp_UB & WCMD_CMD_ID_MASK;
		GL_WCmdParam_X.HasParam_B = ((Temp_UB & WCMD_PARAM_BIT_MASK) == WCMD_PARAM_BIT_MASK) ? true : false;

		DBG_PRINT(DEBUG_SEVERITY_INFO, "Command ID = 0x");
		DBG_PRINTDATABASE(GL_WCmdParam_X.CmdId_UB, HEX);
		DBG_ENDSTR();
		DBG_PRINT(DEBUG_SEVERITY_INFO, "Has Param ? = ");
		DBG_PRINTDATABASE(GL_WCmdParam_X.HasParam_B, BIN);
		DBG_ENDSTR();

		// Get Parameters if any
		if (GL_WCmdParam_X.HasParam_B) {
			GL_WCmdParam_X.ParamNb_UL = WCmdMedium_ReadByte();

			DBG_PRINT(DEBUG_SEVERITY_INFO, "Param Number = ");
			DBG_PRINTDATA(GL_WCmdParam_X.ParamNb_UL);
			DBG_ENDSTR();

            //DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Param List = ");
            for (int i = 0; i < GL_WCmdParam_X.ParamNb_UL; i++) {
                //DBG_PRINTDATA(i);
                //DBG_PRINTDATA(" - ");
                GL_WCmdParam_X.pParamBuffer_UB[i] = WCmdMedium_ReadByte();
                //DBG_PRINTDATABASE(GL_WCmdParam_X.pParamBuffer_UB[i], HEX);
                //DBG_ENDSTR();
            }
		}

		// Check for End Of Transmit byte
		if (WCmdMedium_ReadByte() == WCMD_ETX) {
			DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Found ETX");
			TransitionToProcessCmd();
		}
		else {
			DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Do NOT Found ETX");
			GL_WCmdParam_X.FctSts_E = WCMD_FCT_STS_BAD_PACKET;
			TransitionToSendResp();
		}

	}

}

void ProcessProcessCmd(void) {

	int i = 0;
	GL_WCmdParam_X.FctSts_E = WCMD_FCT_STS_ERROR;

	// Look for the command ID in the table of fonction handler
	for (i = 0; i < GL_WCmdFctNb_UL; i++) {
		if (GL_pWCmdFctDescr_X[i].CmdID_UB == GL_WCmdParam_X.CmdId_UB) {
			DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Found Command ID. Call Function...");
			GL_WCmdParam_X.FctSts_E = GL_pWCmdFctDescr_X[i].FctHandler(GL_WCmdParam_X.pParamBuffer_UB, GL_WCmdParam_X.ParamNb_UL, GL_WCmdParam_X.pAnsBuffer_UB, &(GL_WCmdParam_X.AnsNb_UL));
			DBG_PRINT(DEBUG_SEVERITY_INFO, "Status = ");
			DBG_PRINTDATA(GL_WCmdParam_X.FctSts_E);
			DBG_PRINTDATA(" - Number of Bytes in Answer = ");
			DBG_PRINTDATA(GL_WCmdParam_X.AnsNb_UL);
			DBG_ENDSTR();
			break;
		}
	}

	// Check if the ID has been found and call the appropriate function
	if (i == GL_WCmdFctNb_UL) {
		GL_WCmdParam_X.FctSts_E = WCMD_FCT_STS_UNKNOWN;
		DBG_PRINT(DEBUG_SEVERITY_WARNING, "Command ID Unknown [0x");
		DBG_PRINTDATABASE(GL_WCmdParam_X.CmdId_UB, HEX);
		DBG_PRINTDATA("]");
		DBG_ENDSTR();
	}

	// Go to Send Response state
	TransitionToSendResp();
}

void ProcessSendResp(void) {
	unsigned char pBuffer_UB[WCMD_MAX_ANS_NB];
	unsigned long Offset_UL = 0;

	// Start Of Transmit
	pBuffer_UB[Offset_UL++] = WCMD_STX;

	if (GL_WCmdParam_X.FctSts_E == WCMD_FCT_STS_OK) {

		if (GL_WCmdParam_X.AnsNb_UL == 0) {
			// Command ID
			pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.CmdId_UB;
			// Status Byte
			pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.FctSts_E;
		}
		else {
			// Command ID
			pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.CmdId_UB | WCMD_PARAM_BIT_MASK;
			// Status Byte
			pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.FctSts_E;
			// Data
			pBuffer_UB[Offset_UL++] = (unsigned char)GL_WCmdParam_X.AnsNb_UL;
			for (int i = 0; i < GL_WCmdParam_X.AnsNb_UL; i++)
				pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.pAnsBuffer_UB[i];
		}

		// Acknowledge
		pBuffer_UB[Offset_UL++] = WCMD_ACK;
	}
	else {

		// Command ID
		pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.CmdId_UB;

		// Status Byte
		pBuffer_UB[Offset_UL++] = GL_WCmdParam_X.FctSts_E;

		// Not Acknowledge
		pBuffer_UB[Offset_UL++] = WCMD_NACK;
	}

	// End Of Transmit
	pBuffer_UB[Offset_UL++] = WCMD_ETX;

	// Send Response
	WCmdMedium_BeginPacket();
	WCmdMedium_Write(pBuffer_UB, Offset_UL);
	WCmdMedium_EndPacket();
	delay(1);

    // Manage Transition
    if (WCmdMedium_IsMonoClient())
        TransitionToWaitPacket();       // Wait for next command
    else
        TransitionToIdle();             // Restart medium to close current connection
}


void TransitionToIdle(void) {
	DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To IDLE");
    WCmdMedium_Flush();
    WCmdMedium_Stop();
	GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_IDLE;
}

void TransitionToWaitPacket(void) {
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To WAIT PACKET");
    GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_WAIT_PACKET;
}

void TransitionToCheckCmd(void) {
	DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To CHECK CMD");
	GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_CHECK_CMD;
}

void TransitionToProcessCmd(void) {
	DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To PROCESS CMD");
	GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_PROCESS_CMD;
}

void TransitionToSendResp(void) {
	DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To SEND RESP");
	GL_WCommandInterpreter_CurrentState_E = WCMD_INTERPRETER_STATE::WCMD_INTERPRETER_STATE_SEND_RESP;
}
