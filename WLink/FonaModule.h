/* ******************************************************************************** */
/*                                                                                  */
/* FonaModule.h																		*/
/*                                                                                  */
/* Description :                                                                    */
/*		Header file for FonaModule.cpp												*/
/*		This class creates and manages a FonaModule object to dialog with an 		*/
/*		external GSM module through Serial communication.       					*/
/*                                                                                  */
/* History :	25/05/2017	(RW)	Creation of this file                           */
/*                                                                                  */
/* ******************************************************************************** */

#ifndef __FONA_MODULE_H__
#define __FONA_MODULE_H__

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */

#include <Arduino.h>

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */

#define FONA_MODULE_DEFAULT_BAUDRATE    4800UL

/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */

typedef struct {
    boolean IsInitialized_B;
    unsigned char PinRst_UB;
    unsigned char PinKey_UB;
    unsigned char PinPower_UB;
    unsigned long ApnIndex_UL;
    char pPinCode_UB[4];
} FONA_MODULE_PARAM;

const String GL_pFonaModuleApn_Str[] = { "mworld.be", "internet.proximus.be", "publicip.m2mmobi.be", "standard.m2mmobi.be" };
const String GL_FonaModuleUserAgent_Str = "WLINK";


typedef enum {
    FONA_MODULE_HTTP_PARAM_CID = 0, 
    FONA_MODULE_HTTP_PARAM_URL,
    FONA_MODULE_HTTP_PARAM_UA,
    FONA_MODULE_HTTP_PARAM_CONTENT,
    FONA_MODULE_HTTP_PARAM_USERDATA,
    FONA_MODULE_HTTP_PARAM_REDIR
} FONA_MODULE_HTTP_PARAM_ENUM;

const String GL_pFonaModuleHttpParam_Str[] = {  "CID",
                                                "URL",
                                                "UA",
                                                "CONTENT",
                                                "USERDATA",
                                                "REDIR"
                                                };

typedef enum {
    FONA_MODULE_HTTP_ACTION_METHOD_GET = 0,
    FONA_MODULE_HTTP_ACTION_METHOD_POST,
    FONA_MODULE_HTTP_ACTION_METHOD_HEAD
} FONA_MODULE_HTTP_ACTION_ENUM;

const String GL_pFonaModuleHttpAction_Str[] = { "GET",
                                                "POST",
                                                "HEAD"
                                                };


typedef enum {
	FONA_MODULE_NETWORK_STATUS_NOT_REGISTERED = 0,
	FONA_MODULE_NETWORK_STATUS_REGISTERED,
	FONA_MODULE_NETWORK_STATUS_SEARCHING,
	FONA_MODULE_NETWORK_STATUS_DENIED,
	FONA_MODULE_NETWORK_STATUS_UNKNOWN,
	FONA_MODULE_NETWORK_STATUS_ROAMING
} FONA_MODULE_NETWORK_STATUS_ENUM;

const String GL_pFonaModuleNetworkStatus_Str[] = {  "Not registered",				// = 0
                                                    "Registered (home)",			// = 1
                                                    "Not registered (searching)",	// = 2
                                                    "Denied",						// = 3
                                                    "Unknown",						// = 4
                                                    "Registered roaming"			// = 5
                                                    };


/* ******************************************************************************** */
/* Class
/* ******************************************************************************** */
class FonaModule {
public:
    // Constructor
    FonaModule();

    // Functions
    void init(HardwareSerial * pSerial_H, unsigned long BaudRate_UL, boolean Begin_B, unsigned char PinRst_UB, unsigned char PinKey_UB, unsigned char PinPower_UB, char * pPinCode_UB, unsigned long ApnIdx_UL = 0);
    boolean isInitialized();


    void setPinRst(void);
    void setPinKey(void);
    void clearPinRst(void);
    void clearPinKey(void);
    boolean isPowered(void);


    void flush(void);
    unsigned long readLine(boolean DebugPrint_B = false, unsigned long TimeoutMs_UL = 1000);
    void sendAtCommand(char * pData_UB, boolean Completed_B = true);
    void sendAtCommand(String Data_Str, boolean Completed_B = true);

    void sendAtCommand(char * pData_UB, char * pSuffix_UB, boolean Quoted_B = false, boolean Completed_B = true);
    void sendAtCommand(char * pData_UB, String Suffix_Str, boolean Quoted_B = false, boolean Completed_B = true);
    void sendAtCommand(String Data_Str, String Suffix_Str, boolean Quoted_B = false, boolean Completed_B = true);
    void sendAtCommand(String Data_Str, char * pSuffix_UB, boolean Quoted_B = false, boolean Completed_B = true);
    void addAtData(int Data_SI, boolean Quoted_B = false, boolean Completed_B = true);
	void addAtData(char Data_UB, boolean Quoted_B = false, boolean Completed_B = true);
	void addAtData(unsigned long Data_UL, boolean Quoted_B = false, boolean Completed_B = true);
    void addAtData(char * pData_UB, boolean Quoted_B = false, boolean Completed_B = true);
    void addAtData(String Data_Str, boolean Quoted_B = false, boolean Completed_B = true);
	void endAtData(void);

    boolean checkAtResponse(char * pData_UB);
    boolean checkAtResponse(String Data_Str);
    boolean parseResponse(char * pBuffer_UB, char * pPrefix_UB, int * pValue_SI, char Token_UB = ',', unsigned int Index_UI = 0);
    boolean parseResponse(char * pBuffer_UB, String Prefix_Str, int * pValue_SI, char Token_UB = ',', unsigned int Index_UI = 0);
    boolean begin(void);


    boolean enterPinCode(char * pPinCode_UB);
    boolean enterPinCode(String PinCode_Str);
    boolean enterPinCode(void);

    signed int getSignalStrength(void);
    unsigned int getBatteryLevel(void);

    boolean enableGprs(void);
    boolean disabeGprs(void);
    boolean isGprsEnabled(void);
    boolean getNetworkStatus(int * pStatus_SI);

    boolean httpInit(void);
    boolean httpTerm(void);
    boolean httpParam(FONA_MODULE_HTTP_PARAM_ENUM Param_E, int Value_SI);
	boolean httpParam(FONA_MODULE_HTTP_PARAM_ENUM Param_E, char * pParamValue_UB);
    boolean httpParam(FONA_MODULE_HTTP_PARAM_ENUM Param_E, String ParamValue_Str);
	boolean httpParamStart(FONA_MODULE_HTTP_PARAM_ENUM Param_E);
	boolean httpParamAdd(char ParamValue_UB);
	boolean httpParamAdd(int ParamValue_SI);
	boolean httpParamAdd(unsigned long ParamValue_UL);
	boolean httpParamAdd(char * pParamValue_UB);
	boolean httpParamAdd(String ParamValue_Str);
	boolean httpParamEnd(void);
    boolean httpAction(FONA_MODULE_HTTP_ACTION_ENUM Action_E, int * pServerResponse_SI, int * pDataSize_SI);
    boolean httpRead(char * pData_UB);
    boolean httpRead(char * pData_UB, unsigned long StartAddr_UL, unsigned long DataLength_UL);


    FONA_MODULE_PARAM GL_FonaModuleParam_X;
};

#endif // __FONA_MODULE_H__

