/* ******************************************************************************** */
/*                                                                                  */
/* WConfigManager.cpp													            */
/*                                                                                  */
/* Description :                                                                    */
/*		Describes the state machine to manage the W-Link configuration    			*/
/*                                                                                  */
/* History :  	25/02/2017  (RW)	Creation of this file                           */
/*                                                                                  */
/* ******************************************************************************** */

#define MODULE_NAME		"WConfigManager"

/* ******************************************************************************** */
/* Include
/* ******************************************************************************** */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LiquidCrystal.h>
#include "Keypad.h"
#include <Wire.h>
#include <SD.h>

#include "Utilz.h"
#include "CommEvent.h"

#include "WConfigManager.h"
#include "SerialHandler.h"

#include "WCommand.h"
#include "WCommandMedium.h"
#include "WCommandInterpreter.h"

#include "Debug.h"


/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */
#define WCONFIG_PARAMETER_BUFFER_SIZE   48

#define WCONFIG_CONFIG_TAG0             0xAA
#define WCONFIG_CONFIG_TAG1             0x55

    
#define WCONFIG_ADDR_CONFIG_TAG         0x0000
#define WCONFIG_ADDR_BOARD_REV          0x0002
#define WCONFIG_ADDR_LANGUAGE           0x0004
#define WCONFIG_ADDR_APP				0x0005
#define WCONFIG_ADDR_GEN                0x0006
#define WCONFIG_ADDR_WCMD_MEDIUM        0x0007
#define WCONFIG_ADDR_IO                 0x0008
#define WCONFIG_ADDR_COM                0x0010
#define WCONFIG_ADDR_ETH                0x001C
#define WCONFIG_ADDR_TCP_SERVER         0x0034
#define WCONFIG_ADDR_UDP_SERVER         0x0038
#define WCONFIG_ADDR_TCP_CLIENT         0x003C
#define WCONFIG_ADDR_FONA_MODULE        0x0040
#define WCONFIG_ADDR_INDICATOR          0x0050


/* ******************************************************************************** */
/* LUT
/* ******************************************************************************** */
unsigned long GL_pPortComSpeedLut_UL[] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 };
static const String GL_pLanguageLut_str[] = { "EN", "FR", "NL" };
static const String GL_pWCmdMediumLut_str[] = { "None", "COM0", "COM1", "COM2", "COM3", "UDP Server", "TCP Server", "GSM Server" };
static const int GL_pInputLut_SI[] = { PIN_GPIO_INPUT0, PIN_GPIO_INPUT1, PIN_GPIO_INPUT2, PIN_GPIO_INPUT3 };
static const int GL_pOutputLut_SI[] = { PIN_GPIO_OUTPUT0, PIN_GPIO_OUTPUT1,PIN_GPIO_OUTPUT2,PIN_GPIO_OUTPUT3 };
static const String GL_pWAppLut_str[] = { "Default", "KipControl", "CowWeight" };


/* ******************************************************************************** */
/* LCD Special Char
/* ******************************************************************************** */
byte GL_pLcdWLinkLogo[8] = {
    B11111,
    B10001,
    B01010,
    B10001,
    B10001,
    B10001,
    B11111,
    B00000
};

byte GL_pLcdSignalStrengthLogo0[8] = {
	B00000,
	B00000,
	B00000,
	B00000,
	B00000,
	B00000,
	B10000,
	B00000
};
byte GL_pLcdSignalStrengthLogo1[8] = {
	B00000,
	B00000,
	B00000,
	B00000,
	B11000,
	B00000,
	B10000,
	B00000
};
byte GL_pLcdSignalStrengthLogo2[8] = {
	B00000,
	B00000,
	B11100,
	B00000,
	B11000,
	B00000,
	B10000,
	B00000
};
byte GL_pLcdSignalStrengthLogo3[8] = {
	B11110,
	B00000,
	B11100,
	B00000,
	B11000,
	B00000,
	B10000,
	B00000
};
byte GL_pLcdNoSignalStrengthLogo[8] = {
	B00000,
	B00000,
	B01010,
	B00100,
	B01010,
	B00000,
	B00000,
	B00000
};

/* ******************************************************************************** */
/* General Configuration
/* ******************************************************************************** */
LiquidCrystal GL_LcdObject_X(PIN_LCD_RS, PIN_LCD_RW, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

char GL_ppFlatPanel_KeyConfig_UB[4][4] = {	{ 'A','1','2','3' },        // A = Dedicated function (F1)
											{ 'B','4','5','6' },        // B = Dedicated function (F2)
											{ 'C','7','8','9' },        // C = Dedicated function (F3)
											{ 'X','V','0','.' }         // X = Cancel - V = Validate
											};

byte GL_pFlatPanel_RowPin_UB[4] = { PIN_FP7, PIN_FP6, PIN_FP5, PIN_FP4 };
byte GL_pFlatPanel_ColPin_UB[4] = { PIN_FP0, PIN_FP1, PIN_FP2, PIN_FP3 };
Keypad GL_Keypad_X = Keypad(makeKeymap(GL_ppFlatPanel_KeyConfig_UB), GL_pFlatPanel_RowPin_UB, GL_pFlatPanel_ColPin_UB, sizeof(GL_pFlatPanel_RowPin_UB), sizeof(GL_pFlatPanel_ColPin_UB));


/* ******************************************************************************** */
/* Extenral Variables
/* ******************************************************************************** */
extern GLOBAL_PARAM_STRUCT GL_GlobalData_X;
extern GLOBAL_CONFIG_STRUCT GL_GlobalConfig_X;


/* ******************************************************************************** */
/* Local Variables
/* ******************************************************************************** */
enum WCFG_STATE {
    WCFG_IDLE,
    WCFG_INIT_EEPROM,
    WCFG_INIT_RTC,
    WCFG_CHECK_PIN,
    WCFG_CHECK_TAG,
    WCFG_GET_BOARD_REVISION,
    WCFG_GET_LANGUAGE,
    WCFG_GET_GEN_CONFIG,
    WCFG_GET_WCMD_MEDIUM,
    WCFG_GET_IO_CONFIG,
    WCFG_GET_COM_CONFIG,
    WCFG_GET_ETH_CONFIG,
    WCFG_GET_TCP_SERVER_CONFIG,
    WCFG_GET_UDP_SERVER_CONFIG,
    WCFG_GET_TCP_CLIENT_CONFIG,
    WCFG_GET_FONA_MODULE_CONFIG,
    WCFG_GET_INDICATOR_CONFIG,
	WCFG_GET_APP_CONFIG,
    WCFG_CONFIG_DONE,
    WCFG_ERROR_READING,
    WCFG_BAD_PARAM,
    WCFG_ERROR_INIT
};

static WCFG_STATE GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_IDLE;
static boolean GL_WConfigManagerEnabled_B = false;

static WCFG_STATUS GL_WConfigStatus_E = WCFG_STS_ERROR;

static unsigned char GL_pWConfigBuffer_UB[WCONFIG_PARAMETER_BUFFER_SIZE];


/* ******************************************************************************** */
/* Prototypes for Internal Functions
/* ******************************************************************************** */
static void TransitionToConfigDone(void);
static void TransitionToErrorReading(void);
static void TransitionToBadParam(void);
static void TransitionToErrorInit(void);


/* ******************************************************************************** */
/* Functions
/* ******************************************************************************** */

void WConfigManager_Init() {
    GL_WConfigManagerEnabled_B = false;
    GL_WConfigStatus_E = WCFG_STS_NOT_ENABLED;
    for (int i = 0; i < WCONFIG_PARAMETER_BUFFER_SIZE; i++)
        GL_pWConfigBuffer_UB[i] = 0x00;
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "W-Link Configuration Manager Initialized");
}

void WConfigManager_Enable() {
    GL_WConfigManagerEnabled_B = true;
}

void WConfigManager_Disable() {
    GL_WConfigStatus_E = WCFG_STS_NOT_ENABLED;
    GL_WConfigManagerEnabled_B = false;
}

WCFG_STATUS WConfigManager_Process() { 

    boolean ErrorInit_B = false;
    boolean BadParam_B = false;

    /* Reset Condition */
    if (!GL_WConfigManagerEnabled_B) {
        GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_IDLE;
    }

    /* W-Link Configuration State Machine */
    switch (GL_WConfigManager_CurrentState_E) {

    /* IDLE */
    /* > Wait for enabling command. */
    case WCFG_IDLE:

        GL_WConfigStatus_E = WCFG_STS_NOT_ENABLED;

        if (GL_WConfigManagerEnabled_B) {
            GL_WConfigStatus_E = WCFG_STS_BUSY;
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize Forcing Input");
            pinMode(PIN_ANALOG_IN0, INPUT_PULLUP);
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To INIT EEPROM");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_INIT_EEPROM;
        }

        break;


    /* INIT EEPROM */
    /* > Initialize EEPROM. It is the configuration storage device. */
    case WCFG_INIT_EEPROM:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize EEPROM Modules");
        GL_GlobalData_X.Eeprom_H.init(&Wire1, 0x50);

        if (GL_GlobalData_X.Eeprom_H.isInitialized()) {
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To INIT RTC");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_INIT_RTC;
        }
        else {
            DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "EEPROM cannot be initialized");
            TransitionToErrorInit();
        }


        break;


    /* INIT RTC */
    /* > Initialize RTC. */
    case WCFG_INIT_RTC:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize RTC Modules");
        GL_GlobalData_X.Rtc_H.init(&Wire, PIN_RTC_SQUARE_OUT);

        if (GL_GlobalData_X.Rtc_H.isInitialized()) {
			// Debug print actual time
			DBG_PRINT(DEBUG_SEVERITY_INFO, "Actual time : ");
			DBG_PRINTDATA(GL_GlobalData_X.Rtc_H.getDateTimeString());
			DBG_ENDSTR();

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To CHECK TAG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_CHECK_PIN;
        }
        else {
            DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "RTC cannot be initialized");
            TransitionToErrorInit();
        }

        break;

    /* CHECK PIN */
    /* > Check if pin is low to enable or not configuration with EEPROM. */
    case WCFG_CHECK_PIN:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Get Forcing input status");
        static int ForcingInput_SI = analogRead(PIN_ANALOG_IN0);
        for (int i = 0; i < 5; i++) {
            ForcingInput_SI = analogRead(PIN_ANALOG_IN0);
            delay(500);
        }

        DBG_PRINT(DEBUG_SEVERITY_INFO, "> Value = ");
        DBG_PRINTDATA(ForcingInput_SI);
        DBG_ENDSTR();

        if (ForcingInput_SI > 100) {
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configuration enabled by pin");
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To CHECK TAG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_CHECK_TAG;
        }
        else {
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configuration not enabled by pin");
            TransitionToErrorInit();
        }

        break;


    /* CHECK TAG */
    /* > Check if EEPROM has been configured or not thanks to the configuration tag. */
    case WCFG_CHECK_TAG:
        
        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive configuration tag");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_CONFIG_TAG, GL_pWConfigBuffer_UB, 2) == 2) {
            if ((GL_pWConfigBuffer_UB[0] == WCONFIG_CONFIG_TAG0) && (GL_pWConfigBuffer_UB[1] == WCONFIG_CONFIG_TAG1)) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configuration tag ok!");
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET BOARD REVISION");
                GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_BOARD_REVISION;
            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Error in configuration tag (no tag or bad tag)");
                TransitionToBadParam();
            }
        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET BOARD REVISION */
    /* > Retreive board revision and language settings. */
    case WCFG_GET_BOARD_REVISION:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive board revision");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_BOARD_REV, GL_pWConfigBuffer_UB, 2) == 2) {

            GL_GlobalConfig_X.MajorRev_UB = GL_pWConfigBuffer_UB[0];
            GL_GlobalConfig_X.MinorRev_UB = GL_pWConfigBuffer_UB[1];
            DBG_PRINT(DEBUG_SEVERITY_INFO, "Board Revision = ");
            DBG_PRINTDATA(GL_GlobalConfig_X.MajorRev_UB);
            DBG_PRINTDATA(".");
            DBG_PRINTDATA(GL_GlobalConfig_X.MinorRev_UB);
            DBG_ENDSTR();

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET LANGUAGE");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_LANGUAGE;
        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET LANGUAGE */
    /* > Retreive language. */
    case WCFG_GET_LANGUAGE:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive language settings");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_LANGUAGE, GL_pWConfigBuffer_UB, 1) == 1) {

            if ((GL_pWConfigBuffer_UB[0] & 0x0F) < 3) {
                GL_GlobalConfig_X.Language_E = (WLINK_LANGUAGE_ENUM)GL_pWConfigBuffer_UB[0];
                DBG_PRINT(DEBUG_SEVERITY_INFO, "Language sets to  ");
                DBG_PRINTDATA(GL_pLanguageLut_str[GL_pWConfigBuffer_UB[0]]);
                DBG_ENDSTR();

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET GEN CONFIG");
                GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_GEN_CONFIG;
            }
            else {
                GL_GlobalConfig_X.Language_E = WLINK_LANGUAGE_EN;   // Default language
                DBG_PRINT(DEBUG_SEVERITY_ERROR, "Bad parameter for language settings (0x");
                DBG_PRINTDATABASE((GL_pWConfigBuffer_UB[0] & 0x0F), HEX);
                DBG_PRINTDATA(")");
                DBG_ENDSTR();
                DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Language sets to " + GL_pLanguageLut_str[GL_GlobalConfig_X.Language_E] + " (default)");
                TransitionToBadParam();
            }
        }
        else {
            TransitionToErrorReading();
        }


        break;


    /* GET GEN CONFIG */
    /* > Retreive general configuration. */
    case WCFG_GET_GEN_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive general configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_GEN, GL_pWConfigBuffer_UB, 1) == 1) {

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "General configuration :");

            // Check if WLink has LCD
            if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {
                GL_GlobalConfig_X.HasLcd_B = true;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- LCD");
            }
            else {
                GL_GlobalConfig_X.HasLcd_B = false;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- No LCD");
            }


            // Check if WLink has Flat-Panel
            if ((GL_pWConfigBuffer_UB[0] & 0x02) == 0x02) {
                GL_GlobalConfig_X.HasFlatPanel_B = true;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- Flat-Panel");
            }
            else {
                GL_GlobalConfig_X.HasFlatPanel_B = false;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- No Flat-Panel");
            }


            // Check if WLink has MemoryCard
            if ((GL_pWConfigBuffer_UB[0] & 0x04) == 0x04) {
                GL_GlobalConfig_X.HasMemoryCard_B = true;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- Memory Card");
            }
            else {
                GL_GlobalConfig_X.HasMemoryCard_B = false;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- No Memory Card");
            }


            /* Initialize LCD Modules */
            if (GL_GlobalConfig_X.HasLcd_B) {                
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize LCD Modules");
                GL_GlobalData_X.Lcd_H.init(&GL_LcdObject_X, PIN_LCD_BACKLIGHT);
                GL_GlobalData_X.Lcd_H.clearDisplay();
                GL_GlobalData_X.Lcd_H.setBacklight(255);	// Max value for Backlight by default
                GL_GlobalData_X.Lcd_H.createChar(0, GL_pLcdWLinkLogo);
				GL_GlobalData_X.Lcd_H.createChar(1, GL_pLcdSignalStrengthLogo0);
				GL_GlobalData_X.Lcd_H.createChar(2, GL_pLcdSignalStrengthLogo1);
				GL_GlobalData_X.Lcd_H.createChar(3, GL_pLcdSignalStrengthLogo2);
				GL_GlobalData_X.Lcd_H.createChar(4, GL_pLcdSignalStrengthLogo3);
				GL_GlobalData_X.Lcd_H.createChar(5, GL_pLcdNoSignalStrengthLogo);

                if (!GL_GlobalData_X.Lcd_H.isInitialized()) {
                    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "LCD cannot be initialized");
                    ErrorInit_B = true;
                }                    
            }

            /* Initialize FlatPanel Modules */
            if (GL_GlobalConfig_X.HasFlatPanel_B) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize Flat-Panel Modules");
                GL_GlobalData_X.FlatPanel_H.init(&GL_Keypad_X);

                if (!GL_GlobalData_X.FlatPanel_H.isInitialized()) {
                    FlatPanelManager_Disable();
                    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Flat-Panel cannot be initialized");
                    ErrorInit_B = true;
                }
                else {
                    /* Initialize & Enable Interface Manager */
                    FlatPanelManager_Init(&(GL_GlobalData_X.FlatPanel_H));
                    FlatPanelManager_Enable();

                    /* Initialize default callbacks for each key */
                    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Assign default callback for each key");
                    for (int i = 0; i < 16; i++)
                        GL_GlobalData_X.FlatPanel_H.assignOnKeyPressedEvent((FLAT_PANEL_KEY_ENUM)i, DefaultKeyEvents);
                }
            }

            /* Initialize Memory Card Modules */
            if (GL_GlobalConfig_X.HasMemoryCard_B) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize Memory Card Modules");
                GL_GlobalData_X.MemCard_H.init(PIN_SD_CS, PIN_SD_CD, PIN_SD_WP);

                if (!GL_GlobalData_X.MemCard_H.isInitialized()) {
                    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Memory Card cannot be initialized");
                    ErrorInit_B = true;
                }
            }

            if (ErrorInit_B) {
                TransitionToErrorInit();
            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of general configuration");

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET WCMD MEDIUM");
                GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_WCMD_MEDIUM;
            }
        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET WCMD MEDIUM */
    /* Retreive WCommand Medium. */
    case WCFG_GET_WCMD_MEDIUM:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive WCommand Medium");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_WCMD_MEDIUM, GL_pWConfigBuffer_UB, 1) == 1) {

            if ((GL_pWConfigBuffer_UB[0] & 0x0F) < 8) {
                GL_GlobalConfig_X.WCmdConfig_X.Medium_E = (WLINK_WCMD_MEDIUM_ENUM)(GL_pWConfigBuffer_UB[0] & 0x0F);
                DBG_PRINT(DEBUG_SEVERITY_INFO, "WCommand Medium sets to ");
                DBG_PRINTDATA(GL_pWCmdMediumLut_str[(GL_pWConfigBuffer_UB[0] & 0x0F)]);
                DBG_ENDSTR();

                if ((GL_pWConfigBuffer_UB[0] & 0x10) == 0x10) {
                    GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B = true;
                    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "WCommand Medium is mono-client");
                }
                else { 
                    GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B = false;
                    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "WCommand Medium is not mono-client -> close connection after each command");
                }

                /* Initialize W-Link Command Medium Modules */
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Initialize W-Link Command Management Modules");

                switch (GL_GlobalConfig_X.WCmdConfig_X.Medium_E) {
                case WLINK_WCMD_MEDIUM_COM0:        WCmdMedium_Init(WCMD_MEDIUM_SERIAL, GetSerialHandle(0), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);                    break;
                case WLINK_WCMD_MEDIUM_COM1:        WCmdMedium_Init(WCMD_MEDIUM_SERIAL, GetSerialHandle(1), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);                    break;
                case WLINK_WCMD_MEDIUM_COM2:        WCmdMedium_Init(WCMD_MEDIUM_SERIAL, GetSerialHandle(2), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);                    break;
                case WLINK_WCMD_MEDIUM_COM3:        WCmdMedium_Init(WCMD_MEDIUM_SERIAL, GetSerialHandle(3), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);                    break;
                case WLINK_WCMD_MEDIUM_UDP_SERVER:  WCmdMedium_Init(WCMD_MEDIUM_UDP, &(GL_GlobalData_X.EthAP_X.UdpServer_H), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);   break;
                case WLINK_WCMD_MEDIUM_TCP_SERVER:  WCmdMedium_Init(WCMD_MEDIUM_TCP, &(GL_GlobalData_X.EthAP_X.TcpServer_H), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);   break;
                case WLINK_WCMD_MEDIUM_GSM_SERVER:  WCmdMedium_Init(WCMD_MEDIUM_GSM, &(GL_GlobalData_X.EthAP_X.GsmServer_H), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);   break;      // TODO : to modify once the GSMServer Object will be created
                }       

                WCommandInterpreter_Init(GL_GlobalConfig_X.WCmdConfig_X.pFctDescr_X, GL_GlobalConfig_X.WCmdConfig_X.NbFct_UL);

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of WCommand Medium configuration");

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET IO CONFIG");
                GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_IO_CONFIG;
            }
            else {

                DBG_PRINT(DEBUG_SEVERITY_ERROR, "Bad parameter for WCommand Medium settings (0x");
                DBG_PRINTDATABASE((GL_pWConfigBuffer_UB[0] & 0x0F), HEX);
                DBG_PRINTDATA(")");
                DBG_ENDSTR();

                WConfigManager_BuildSerialGateway();
                TransitionToBadParam();
            }           

        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET IO CONFIG */
    /* > Retreive digital I/O's configuration. */
    case WCFG_GET_IO_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive digital I/O's configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_IO, GL_pWConfigBuffer_UB, 8) == 8) {

            int i = 0;

            /* Configure Inputs */
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configure Inputs:");
            for (i = 0; i < 4; i++) {
                if ((GL_pWConfigBuffer_UB[i] & 0x01) == 0x01) {

                    pinMode(GL_GlobalData_X.pGpioInputIndex_UB[i], INPUT);

                    DBG_PRINT(DEBUG_SEVERITY_INFO, "- Configure IN");
                    DBG_PRINTDATA(i);
                    DBG_PRINTDATA(": Enabled");
                    DBG_ENDSTR();

                    // TODO : IRQ-based configuration sould be added here
                }
            }

            /* Configure Outputs */
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configure Outputs:");
            for (i = 4; i < 8; i++) {
                if ((GL_pWConfigBuffer_UB[i] & 0x01) == 0x01) {

                    pinMode(GL_GlobalData_X.pGpioOutputIndex_UB[i-4], OUTPUT);
                    digitalWrite(GL_GlobalData_X.pGpioOutputIndex_UB[i-4], LOW);

                    DBG_PRINT(DEBUG_SEVERITY_INFO, "- Configure OUT");
                    DBG_PRINTDATA(i-4);
                    DBG_PRINTDATA(": Enabled");
                    DBG_ENDSTR();
                }
            }

            /* Configure Blinking LED */  
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configure Blinking LED");
            pinMode(GL_GlobalData_X.LedPin_UB, OUTPUT);
            digitalWrite(GL_GlobalData_X.LedPin_UB, HIGH);	// Turn-on by default


            /* Add Output Management for Bug in SPI �*/
            pinMode(PIN_SPI_CS, OUTPUT);


            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of I/O's configuration");

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET COM CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_COM_CONFIG;
        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET COM CONFIG */
    /* > Retreive serial COM port configuration. */
    case WCFG_GET_COM_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive serial COM port configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_COM, GL_pWConfigBuffer_UB, 12) == 12) {
        
            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configure COM Ports:");
            for (int i = 0; i < 4; i++) {

                GL_GlobalConfig_X.pComPortConfig_X[i].Index_UL = i;
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- Configure COM");
                DBG_PRINTDATA(i);
                DBG_PRINTDATA(": ");

                if ((GL_pWConfigBuffer_UB[i * 3] & 0x01) == 0x01) {                    

                    // Check configuration
                    if ((GL_pWConfigBuffer_UB[i * 3 + 1] != 0x06)) {
                        DBG_PRINTDATA("Bad parameter in Config Mode (0x");
                        DBG_PRINTDATABASE(GL_pWConfigBuffer_UB[i * 3 + 1], HEX);
                        DBG_PRINTDATA(")");
                        DBG_ENDSTR();
                        BadParam_B = true;
                    }
                    else
                        GL_GlobalConfig_X.pComPortConfig_X[i].Config_UB = SERIAL_8N1;

                    // Check baudrate
                    if ((GL_pWConfigBuffer_UB[i * 3 + 2] > 0x07)) {
                        DBG_PRINTDATA("Bad parameter in Baudrate (0x");
                        DBG_PRINTDATABASE(GL_pWConfigBuffer_UB[i * 3 + 2], HEX);
                        DBG_PRINTDATA(")");
                        DBG_ENDSTR();
                        BadParam_B = true;
                    }
                    else
                        GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL = GL_pPortComSpeedLut_UL[GL_pWConfigBuffer_UB[i * 3 + 2]];

                    // Check if debug
                    GL_GlobalConfig_X.pComPortConfig_X[i].isDebug_B = ((GL_pWConfigBuffer_UB[i * 3] & 0x02) == 0x02) ? true : false;

                    // TODO : IRQ-based configuration sould be added here
					// NOTE : IRQ management is done further in the Application Configuration   
					// Assign empty function as CommEvent
					GL_GlobalConfig_X.pComPortConfig_X[i].pFctCommEvent = Nop;

                    // Configure actual COM port (if not debug port)
                    if (!GL_GlobalConfig_X.pComPortConfig_X[i].isDebug_B)
                        GetSerialHandle(i)->begin(GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL);


                    // Print out configuration
                    GL_GlobalConfig_X.pComPortConfig_X[i].isEnabled_B = true;
                    DBG_PRINTDATA(": Enabled (8N1, ");
                    DBG_PRINTDATA(GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL);
                    DBG_PRINTDATA("[baud])");
                    if (GL_GlobalConfig_X.pComPortConfig_X[i].isDebug_B)
                        DBG_PRINTDATA(" -> Configured as Debug COM port");
                    DBG_ENDSTR();

                }
                else {
                    GL_GlobalConfig_X.pComPortConfig_X[i].isEnabled_B = false;
                    DBG_PRINTDATA(": Not enabled");
                    DBG_ENDSTR();
                }

            }

            // Configure Debug COM port in last
            for (int i = 0; i < 4; i++) {
                if ((GL_GlobalConfig_X.pComPortConfig_X[i].isEnabled_B) && (GL_GlobalConfig_X.pComPortConfig_X[i].isDebug_B)) {
                    if (i == WLINK_DEBUG_DEFAULT_COM_PORT) {
                        if (GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL != WLINK_DEBUG_DEFAULT_SPEED) {
                            DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Change Debug output speed");
                            delay(10);
                            Debug_Init(GetSerialHandle(i), GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL);
                        }
                        else {
                            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "No change in Debug COM port default configuration");
                        }
                    }
                    else {
                        DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Change Debug COM port");
                        delay(10);
                        Debug_Init(GetSerialHandle(i), GL_GlobalConfig_X.pComPortConfig_X[i].Baudrate_UL);
                    }
                }
            }


            // Init SerialManager
            SerialManager_Init();


            if (BadParam_B) {
                TransitionToBadParam();
            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of COM Ports configuration");

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET ETH CONFIG");
                GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_ETH_CONFIG;
            }
        }
        else {
            TransitionToErrorReading();
        }
        
        break;


    /* GET ETH CONFIG */
    /* > Retreive Ethernet configuration. */
    case WCFG_GET_ETH_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive Ethernet configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_ETH, GL_pWConfigBuffer_UB, 24) == 24) {

            if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {
                GL_GlobalConfig_X.EthConfig_X.isEnabled_B = true;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Ethernet enabled");

                // Get MAC Address
                for (int i = 0; i < sizeof(GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB); i++) {
                    GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB[i] = GL_pWConfigBuffer_UB[i + 2];
                }
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- MAC Address = [");
                DBG_PRINTDATA(HexArrayToString(GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB, sizeof(GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB), ":"));
                DBG_PRINTDATA("]");
                DBG_ENDSTR();

                // Check if DHCP
                if ((GL_pWConfigBuffer_UB[0] & 0x02) == 0x02) {
                    GL_GlobalConfig_X.EthConfig_X.isDhcp_B = true;
                    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- DHCP enabled");
                }
                else {
                    GL_GlobalConfig_X.EthConfig_X.isDhcp_B = false;
                    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- DHCP not enabled");

                    GL_GlobalConfig_X.EthConfig_X.IpAddr_X = IPAddress(GL_pWConfigBuffer_UB[8], GL_pWConfigBuffer_UB[9], GL_pWConfigBuffer_UB[10], GL_pWConfigBuffer_UB[11]);
                    DBG_PRINT(DEBUG_SEVERITY_INFO, "- IP Address = ");
                    DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.IpAddr_X);
                    DBG_ENDSTR();

                    // Check if Advanced Configuration
                    if ((GL_pWConfigBuffer_UB[0] & 0x05) == 0x05) {
                        GL_GlobalConfig_X.EthConfig_X.isAdvancedConfig_B = true;
                        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- Advanced configuration");

                        GL_GlobalConfig_X.EthConfig_X.SubnetMaskAddr_X = IPAddress(GL_pWConfigBuffer_UB[12], GL_pWConfigBuffer_UB[13], GL_pWConfigBuffer_UB[14], GL_pWConfigBuffer_UB[15]);
                        DBG_PRINT(DEBUG_SEVERITY_INFO, "- Subnet Mask = ");
                        DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.SubnetMaskAddr_X);
                        DBG_ENDSTR();

                        GL_GlobalConfig_X.EthConfig_X.GatewayAddr_X = IPAddress(GL_pWConfigBuffer_UB[16], GL_pWConfigBuffer_UB[17], GL_pWConfigBuffer_UB[18], GL_pWConfigBuffer_UB[19]);
                        DBG_PRINT(DEBUG_SEVERITY_INFO, "- Gateway Address = ");
                        DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.GatewayAddr_X);
                        DBG_ENDSTR();

                        GL_GlobalConfig_X.EthConfig_X.DnsIpAddr_X = IPAddress(GL_pWConfigBuffer_UB[20], GL_pWConfigBuffer_UB[21], GL_pWConfigBuffer_UB[22], GL_pWConfigBuffer_UB[23]);
                        DBG_PRINT(DEBUG_SEVERITY_INFO, "- DNS Address = ");
                        DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.DnsIpAddr_X);
                        DBG_ENDSTR();
                    }
                    else {
                        GL_GlobalConfig_X.EthConfig_X.isAdvancedConfig_B = false;
                        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "- No advanced configuration");
                    }
                }
                
                // Initialize Network Adapter
                if (GL_GlobalConfig_X.EthConfig_X.isDhcp_B) {
                    GL_GlobalData_X.Network_H.init(PIN_ETH_LINKED, GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB);
                }
                else {
                    if (GL_GlobalConfig_X.EthConfig_X.isAdvancedConfig_B) {
                        GL_GlobalData_X.Network_H.init(PIN_ETH_LINKED, GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB, GL_GlobalConfig_X.EthConfig_X.IpAddr_X, GL_GlobalConfig_X.EthConfig_X.SubnetMaskAddr_X, GL_GlobalConfig_X.EthConfig_X.GatewayAddr_X, GL_GlobalConfig_X.EthConfig_X.DnsIpAddr_X);
                    }
                    else {
                        GL_GlobalData_X.Network_H.init(PIN_ETH_LINKED, GL_GlobalConfig_X.EthConfig_X.pMacAddr_UB, GL_GlobalConfig_X.EthConfig_X.IpAddr_X);
                    }
                }

                // Enable Network Adapter Manager
                NetworkAdapterManager_Init(&(GL_GlobalData_X.Network_H));
                NetworkAdapterManager_Enable(); // GL_GlobalData_X.Network_H.begin() is called in NetworkAdapterManager_Process() when cable is conneted

                
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of Ethernet configuration");
            }
            else {
                // Disable Network Adapter Manager
                NetworkAdapterManager_Disable();

                GL_GlobalConfig_X.EthConfig_X.isEnabled_B = false;
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Ethernet not enabled");
            }


            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET TCP SERVER CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_TCP_SERVER_CONFIG;

        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET TCP SERVER CONFIG */
    /* > Retreive TCP Server configuration. */
    case WCFG_GET_TCP_SERVER_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive TCP Server configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_TCP_SERVER, GL_pWConfigBuffer_UB, 4) == 4) {
        
            if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "TCP Server enabled");

                GL_GlobalConfig_X.EthConfig_X.TcpServerConfig_X.LocalPort_UI = (unsigned int)((GL_pWConfigBuffer_UB[2] << 8) + GL_pWConfigBuffer_UB[1]);
                DBG_PRINT(DEBUG_SEVERITY_INFO, "Local port = ");
                DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.TcpServerConfig_X.LocalPort_UI);
                DBG_ENDSTR();

                /* Initialize TCP Server Modules */
                GL_GlobalData_X.EthAP_X.TcpServer_H.init(&(GL_GlobalData_X.Network_H), GL_GlobalConfig_X.EthConfig_X.TcpServerConfig_X.LocalPort_UI);
                if (GL_GlobalData_X.EthAP_X.TcpServer_H.isInitialized()) {
                    TCPServerManager_Init(&(GL_GlobalData_X.Network_H), &(GL_GlobalData_X.EthAP_X.TcpServer_H));
                    TCPServerManager_Enable();
                    GL_GlobalConfig_X.EthConfig_X.TcpServerConfig_X.isEnabled_B = true;
                }

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of TCP Server configuration");

            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "TCP Server not enabled");
                GL_GlobalConfig_X.EthConfig_X.TcpServerConfig_X.isEnabled_B = false;
            }

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET UDP SERVER CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_UDP_SERVER_CONFIG;        
        }
        else {
            TransitionToErrorReading();        
        }

        break;



    /* GET UDP SERVER CONFIG */
    /* > Retreive UDP Server configuration. */
    case WCFG_GET_UDP_SERVER_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive UDP Server configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_UDP_SERVER, GL_pWConfigBuffer_UB, 4) == 4) {

            if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "UDP Server enabled");
                GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.isEnabled_B = true;

                GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.LocalPort_UI = (unsigned int)((GL_pWConfigBuffer_UB[2] << 8) + GL_pWConfigBuffer_UB[1]);
                DBG_PRINT(DEBUG_SEVERITY_INFO, "Local port = ");
                DBG_PRINTDATA(GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.LocalPort_UI);
                DBG_ENDSTR();

                /* Initialize UDP Server Modules */
                GL_GlobalData_X.EthAP_X.UdpServer_H.init(&(GL_GlobalData_X.Network_H), GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.LocalPort_UI);
                if (GL_GlobalData_X.EthAP_X.UdpServer_H.isInitialized()) {
                    UDPServerManager_Init(&(GL_GlobalData_X.Network_H), &(GL_GlobalData_X.EthAP_X.UdpServer_H));
                    UDPServerManager_Enable();
                    GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.isEnabled_B = true;
                }

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of UDP Server configuration");
            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "UDP Server not enabled");
                GL_GlobalConfig_X.EthConfig_X.UdpServerConfig_X.isEnabled_B = false;
            }

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET TCP CLIENT CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_TCP_CLIENT_CONFIG;
        }
        else {
            TransitionToErrorReading();
        }

        break;



    /* GET TCP CLIENT CONFIG */
    /* > Retreive TCP Client configuration. */
    case WCFG_GET_TCP_CLIENT_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive TCP Client configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_TCP_CLIENT, GL_pWConfigBuffer_UB, 4) == 4) {

            DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "NOT YET IMPLEMENTED..");


            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET FONA MODULE CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_FONA_MODULE_CONFIG;
        }
        else {
            TransitionToErrorReading();
        }

        break;


    /* GET FONA MODULE CONFIG */
    /* > Retreive FONA Module configuration. */
    case WCFG_GET_FONA_MODULE_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive FONA Module configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_FONA_MODULE, GL_pWConfigBuffer_UB, 16) == 16) {

            if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "FONA Module enabled");
                GL_GlobalConfig_X.GsmConfig_X.isEnabled_B = true;

                // Get COM Index
                GL_GlobalConfig_X.GsmConfig_X.ComIndex_UB = (GL_pWConfigBuffer_UB[0] & 0x30) >> 4;
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- Port COM Index = ");
                DBG_PRINTDATA((int)(GL_GlobalConfig_X.GsmConfig_X.ComIndex_UB));
                DBG_ENDSTR();

                // Get Pinout
                GL_GlobalConfig_X.GsmConfig_X.PinRst_UB = (GL_pWConfigBuffer_UB[1] & 0x03) >> 0;
                GL_GlobalConfig_X.GsmConfig_X.PinKey_UB = (GL_pWConfigBuffer_UB[1] & 0x0C) >> 2;
                GL_GlobalConfig_X.GsmConfig_X.PinPower_UB = (GL_pWConfigBuffer_UB[1] & 0x30) >> 4;
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- Pin Reset = OUT");
                DBG_PRINTDATA((int)(GL_GlobalConfig_X.GsmConfig_X.PinRst_UB));
                DBG_ENDSTR();
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- Pin Key = OUT");
                DBG_PRINTDATA((int)(GL_GlobalConfig_X.GsmConfig_X.PinKey_UB));
                DBG_ENDSTR();
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- Pin Power = IN");
                DBG_PRINTDATA((int)(GL_GlobalConfig_X.GsmConfig_X.PinPower_UB));
                DBG_ENDSTR();

                // Get PIN Code
                GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB[0] = (char)(GL_pWConfigBuffer_UB[2]);
                GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB[1] = (char)(GL_pWConfigBuffer_UB[3]);
                GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB[2] = (char)(GL_pWConfigBuffer_UB[4]);
                GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB[3] = (char)(GL_pWConfigBuffer_UB[5]);
                GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB[4] = 0;
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- PIN Code = ");
                DBG_PRINTDATA(GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB);
                DBG_ENDSTR();

                // Get APN Index
                GL_GlobalConfig_X.GsmConfig_X.ApnIndex_UL = (unsigned long)(GL_pWConfigBuffer_UB[6] & 0x0F);
                DBG_PRINT(DEBUG_SEVERITY_INFO, "- APN Index = ");
                DBG_PRINTDATA(GL_GlobalConfig_X.GsmConfig_X.ApnIndex_UL);
                DBG_ENDSTR();

                // Initialize FONA Module
                GL_GlobalData_X.Fona_H.init(GetSerialHandle(GL_GlobalConfig_X.GsmConfig_X.ComIndex_UB), 0, false, GL_pOutputLut_SI[GL_GlobalConfig_X.GsmConfig_X.PinRst_UB], GL_pOutputLut_SI[GL_GlobalConfig_X.GsmConfig_X.PinKey_UB], GL_pInputLut_SI[GL_GlobalConfig_X.GsmConfig_X.PinPower_UB], GL_GlobalConfig_X.GsmConfig_X.pPinCode_UB, GL_GlobalConfig_X.GsmConfig_X.ApnIndex_UL);
                FonaModuleManager_Init(&(GL_GlobalData_X.Fona_H));
                FonaModuleManager_Enable();

                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of FONA Module configuration");
            }
            else {
                DBG_PRINTLN(DEBUG_SEVERITY_INFO, "FONA Module not enabled");
                GL_GlobalConfig_X.GsmConfig_X.isEnabled_B = false;
            }

            DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET INDICATOR CONFIG");
            GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_INDICATOR_CONFIG;
        }
        else {
            TransitionToErrorReading();
        }

        break;



    /* GET INDICATOR CONFIG */
    /* > Retreive Indicator configuration */
    case WCFG_GET_INDICATOR_CONFIG:

        DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive Indicators configuration");
        if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_INDICATOR, GL_pWConfigBuffer_UB, 16) == 16) {

			// Initialize Interface if at least one Indicator is enabled
			for (int i = 0; i < 4; i++) {
				if ((GL_pWConfigBuffer_UB[i * 4] & 0x01) == 0x01) {
					IndicatorInterface_Init();
					break;
				}
			}

			// Configure the 4 possible Indicators
			for (int i = 0; i < 4; i++) {

				DBG_PRINT(DEBUG_SEVERITY_INFO, "- Configure Indicator ");
				DBG_PRINTDATA(i);
				DBG_PRINTDATA(": ");

				if ((GL_pWConfigBuffer_UB[i * 4] & 0x01) == 0x01) {
					DBG_PRINTDATA("Enabled");
					DBG_ENDSTR();

					// Get COM Port Index
					if ((GL_pWConfigBuffer_UB[i * 4 + 3] & 0x03) <= 0x03) {						
						DBG_PRINT(DEBUG_SEVERITY_INFO, "    > COM Port index = COM");
						DBG_PRINTDATA((GL_pWConfigBuffer_UB[i * 4 + 3] & 0x03));
						DBG_ENDSTR();
						GL_GlobalConfig_X.pIndicatorConfig_X[i].ComPortIdx_UB = (GL_pWConfigBuffer_UB[i * 4 + 3] & 0x03);
					}
					else {
						DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "    > Wrong COM Port index, use COM1 !");
						GL_GlobalConfig_X.pIndicatorConfig_X[i].ComPortIdx_UB = PORT_COM1;
					}

					// Get Interface Type
					if (GL_pWConfigBuffer_UB[i * 4 + 1] < INDICATOR_INTERFACE_DEVICES_NUM) {
						DBG_PRINT(DEBUG_SEVERITY_INFO, "    > Interface Type = ");
						DBG_PRINTDATA(pIndicatorInterfaceDeviceLut_Str[GL_pWConfigBuffer_UB[i * 4 + 1]]);
						DBG_ENDSTR();
						GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceType_E = (INDICATOR_INTERFACE_DEVICES_ENUM)(GL_pWConfigBuffer_UB[i * 4 + 1]);
					}
					else {
						DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "    > Wrong Interface Type, use ");
						DBG_PRINTDATA(pIndicatorInterfaceDeviceLut_Str[0]);
						DBG_PRINTDATA(" !");
						DBG_ENDSTR();
						GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceType_E = INDICATOR_INTERFACE_DEVICES_ENUM::INDICATOR_LD5218;
					}

                    // Get Frame Type
                    if (((GL_pWConfigBuffer_UB[i * 4 + 2]) & 0x0F) < INDICATOR_INTERFACE_FRAME_NUM) {
                        DBG_PRINT(DEBUG_SEVERITY_INFO, "    > Frame Type = ");
                        DBG_PRINTDATA(pIndicatorInterfaceFrameLut_Str[((GL_pWConfigBuffer_UB[i * 4 + 2]) & 0x0F)]);
                        DBG_ENDSTR();
                        GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceFrame_E = (INDICATOR_INTERFACE_FRAME_ENUM)((GL_pWConfigBuffer_UB[i * 4 + 2]) & 0x0F);
                    }
                    else {
                        DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "    > Wrong Frame Type, use ");
                        DBG_PRINTDATA(pIndicatorInterfaceFrameLut_Str[0]);
                        DBG_PRINTDATA(" !");
                        DBG_ENDSTR();
                        GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceFrame_E = INDICATOR_INTERFACE_FRAME_ENUM::INDICATOR_INTERFACE_FRAME_ASK_WEIGHT;
                    }

					// Check IRQ
					if ((GL_pWConfigBuffer_UB[i * 4] & 0x02) == 0x02) {
						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "    > Has IRQ");
						GL_GlobalConfig_X.pIndicatorConfig_X[i].HasIrq_B = true;
						GL_GlobalConfig_X.pComPortConfig_X[GL_GlobalConfig_X.pIndicatorConfig_X[i].ComPortIdx_UB].pFctCommEvent = CommEvent_Indicator;	// Assign Event on IRQ
					}
					else {
						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "    > No IRQ Handling");
						GL_GlobalConfig_X.pIndicatorConfig_X[i].HasIrq_B = false;
					}

					// Check Echo
					if ((GL_pWConfigBuffer_UB[i * 4] & 0x04) == 0x04) {
						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "    > Has Echo");
						GL_GlobalConfig_X.pIndicatorConfig_X[i].HasEcho_B = true;

						// Get Echo COM Port Index
						if (((GL_pWConfigBuffer_UB[i * 4 + 3] & 0x0C) >> 2) <= 0x03) {
							DBG_PRINT(DEBUG_SEVERITY_INFO, "    > Echo COM Port index = COM");
							DBG_PRINTDATA((GL_pWConfigBuffer_UB[i * 4 + 3] & 0x0C) >> 2);
							DBG_ENDSTR();
							GL_GlobalConfig_X.pIndicatorConfig_X[i].EchoComPortIx_UB = ((GL_pWConfigBuffer_UB[i * 4 + 3] & 0x0C) >> 2);
						}
						else {
							DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "    > Wrong Echo COM Port index, discard Echo function");
							GL_GlobalConfig_X.pIndicatorConfig_X[i].HasEcho_B = false;
						}

					}
					else {
						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "    > No Echo (ignore Echo COM Index)");
						GL_GlobalConfig_X.pIndicatorConfig_X[i].HasEcho_B = false;
					}

				}
				else {
					DBG_PRINTDATA("Not Enabled");
					DBG_ENDSTR();
				}
			}

			DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Call configuration functions for Indicators");
			DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Only one Indicator managed for now !");
			// Call Configuration Functions
			for (int i = 0; i < 1; i++) {																								// TODO : Limited to ONE Indicator

				// Call low-level function on Indicator object
				GL_GlobalData_X.Indicator_H.init(GetSerialHandle(GL_GlobalConfig_X.pIndicatorConfig_X[i].ComPortIdx_UB), false);
				GL_GlobalData_X.Indicator_H.setIndicatorDevice(GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceType_E);

				// Configure Echo if neeeded
				if (GL_GlobalConfig_X.pIndicatorConfig_X[i].HasEcho_B)
					GL_GlobalData_X.Indicator_H.attachEcho(GetSerialHandle(GL_GlobalConfig_X.pIndicatorConfig_X[i].EchoComPortIx_UB), false);

				// Configure Manager
				IndicatorManager_Init(&(GL_GlobalData_X.Indicator_H));
				IndicatorManager_Enable(GL_GlobalConfig_X.pIndicatorConfig_X[i].InterfaceFrame_E, GL_GlobalConfig_X.pIndicatorConfig_X[i].HasIrq_B);

			}

            
			DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To GET APP CONFIG");
			GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_GET_APP_CONFIG;
        }
        else {
            TransitionToErrorReading();
        }

        break;


	/* GET APP CONFIG */
	/* > Retreive Application configuration. */
	case WCFG_GET_APP_CONFIG:

		GL_GlobalConfig_X.App_X.hasApplication_B = false;

		DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive Application configuration");
		if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_APP, GL_pWConfigBuffer_UB, 1) == 1) {
			
			if ((GL_pWConfigBuffer_UB[0] & 0x01) == 0x01) {

				DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Application configuration needed");
				DBG_PRINT(DEBUG_SEVERITY_INFO, "Application : ");
				DBG_PRINTDATA(GL_pWAppLut_str[((GL_pWConfigBuffer_UB[0] & 0xF0) >> 4)]);
				DBG_ENDSTR();

				switch ((GL_pWConfigBuffer_UB[0] & 0xF0) >> 4)
				{

					/* Default Appplication */
					/* -------------------- */
					case WCFG_APP_DEFAULT:
						DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Nothing implemented yet !");
						break;


					/* Kip Control */
					/* ----------- */
					case WCFG_APP_KIP_CONTROL:

						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Assign generic functions for Application Process");
						GL_GlobalConfig_X.App_X.pFctInit = KipControlManager_Init;
						GL_GlobalConfig_X.App_X.pFctEnable = KipControlManager_Enable;
						GL_GlobalConfig_X.App_X.pFctDisable = KipControlManager_Disable;
						GL_GlobalConfig_X.App_X.pFctProcess = KipControlManager_Process;
						GL_GlobalConfig_X.App_X.pFctIsEnabled = KipControlManager_IsEnabled;
						GL_GlobalConfig_X.App_X.hasApplication_B = true;

						if ((GL_pWConfigBuffer_UB[0] & 0x02) == 0x02) {
							DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Application has Dediacted Menu - Assign functions");
							GL_GlobalConfig_X.App_X.pFctInitMenu = KipControlMenu_Init;
							GL_GlobalConfig_X.App_X.pFctGetFirstItem = KipControlMenu_GetFirstItem;
							GL_GlobalConfig_X.App_X.hasMenu_B = true;
						}
						else {
							GL_GlobalConfig_X.App_X.hasMenu_B = false;
						}

						DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Configure Communication Medium");
						if (GL_GlobalData_X.Eeprom_H.read(KC_WORKING_AREA_OFFSET, GL_pWConfigBuffer_UB, 1) == 1) {

							switch (GL_pWConfigBuffer_UB[0] & 0x0F) {

								// Ethernet -> Wired Communication
								case KC_MEDIUM_ETHERNET:
									KipControlMedium_Init(KC_MEDIUM_ETHERNET, &(GL_GlobalData_X.Network_H));
									KipControlMedium_SetServerParam(GL_cPortalServerName_str, 80);
									break;

								// GSM -> Wireless Communication
								case KC_MEDIUM_GSM:
									KipControlMedium_Init(KC_MEDIUM_GSM, &(GL_GlobalData_X.Fona_H));
									KipControlMedium_SetServerParam(GL_cPortalServerName_str, 80);
									break;

								// Nop
								default:
									DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Wrong Medium configuration -> use Ethernet");
									KipControlMedium_Init(KC_MEDIUM_ETHERNET, &(GL_GlobalData_X.Network_H));
									KipControlMedium_SetServerParam(GL_cPortalServerName_str, 80);
									break;
							}


							/* ------------------------- */
							/* Init & Enable Application */
							/* ------------------------- */
							GL_GlobalConfig_X.App_X.pFctInit(&(GL_GlobalData_X.KipControl_H));		// Call init with specific object
							GL_GlobalConfig_X.App_X.pFctEnable();

						}
						else {
							DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Error while retreiving Communication Medium configuration!");
							TransitionToErrorReading();
						}						

						break;


					/* Cow Weight */
					/* ---------- */
					case WCFG_APP_COW_WEIGHT:
						DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Nothing implemented yet !");
						break;


					default:
						break;
				}

			}
			else {
				DBG_PRINTLN(DEBUG_SEVERITY_INFO, "No application to be configured");
			}

			TransitionToConfigDone();
		}
		else {
			TransitionToErrorReading();
		}

	    break;

    ///* GET XXX CONFIG */
    ///* > Retreive XXX configuration. */
    //case WCFG_STATE:

        //DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Retreive XXX configuration");
        //if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_XXX, GL_pWConfigBuffer_UB, XXX) == XXX) {



        //      TransitionToConfigDone();


        //}
        //else {
        //    TransitionToErrorReading();
        //}

    //    break;


    /* CONFIG DONE */
    /* > End of configuration phase. State machine idling until renew of configuration. */
    case WCFG_CONFIG_DONE:

        GL_WConfigStatus_E = WCFG_STS_OK;

        break;


    /* ERROR READING */
    /* > Error while reading a configuration in a previous state. */
    case WCFG_ERROR_READING:

        GL_WConfigStatus_E = WCFG_STS_ERROR_READING;

        break;


    /* BAD PARAM */
    /* > Bad parameter(s) found from a configuration in a previous state. */
    case WCFG_BAD_PARAM:

        GL_WConfigStatus_E = WCFG_STS_BAD_PARAM;

        break;


    /* ERROR INIT */
    /* > Error while trying to initialize an object in a previous state. */
    case WCFG_ERROR_INIT:

        GL_WConfigStatus_E = WCFG_STS_ERROR_INIT;

        break;


    }

    return GL_WConfigStatus_E;
}


void WConfigManager_BuildSerialGateway(void) {

    DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "Build Serial Gateway");

    GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B = true;
    DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "WCommand Medium is mono-client");
    GL_GlobalConfig_X.WCmdConfig_X.Medium_E = WLINK_WCMD_MEDIUM_COM0;   // Default Medium = Default debug port
    DBG_PRINTLN(DEBUG_SEVERITY_WARNING, "WCommand Medium sets to " + GL_pWCmdMediumLut_str[GL_GlobalConfig_X.WCmdConfig_X.Medium_E] + " (default)");
	
	GL_GlobalConfig_X.pComPortConfig_X[0].pFctCommEvent = Nop;	// Assign empty function as CommEvent

    WCmdMedium_Init(WCMD_MEDIUM_SERIAL, GetSerialHandle(0), GL_GlobalConfig_X.WCmdConfig_X.isMonoClient_B);
    WCommandInterpreter_Init(GL_GlobalConfig_X.WCmdConfig_X.pFctDescr_X, GL_GlobalConfig_X.WCmdConfig_X.NbFct_UL);

}


/* ******************************************************************************** */
/* Internal Functions
/* ******************************************************************************** */

void TransitionToConfigDone(void) {
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "End of Configuration from EEPROM..");
    GL_WConfigStatus_E = WCFG_STS_OK;
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To CONFIG DONE");
    GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_CONFIG_DONE;
}

void TransitionToErrorReading(void) {
    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Error when reading from EEPROM");
    GL_WConfigStatus_E = WCFG_STS_ERROR_READING;
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To ERROR READING");
    GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_ERROR_READING;
}

void TransitionToBadParam(void) {
    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Bad parameter(s) found in configuration");
    GL_WConfigStatus_E = WCFG_STS_BAD_PARAM;
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To BAD PARAM");
    GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_BAD_PARAM;
}

void TransitionToErrorInit(void) {
    DBG_PRINTLN(DEBUG_SEVERITY_ERROR, "Error while trying to initialize an object");
    GL_WConfigStatus_E = WCFG_STS_ERROR_INIT;
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Transition To ERROR INIT");
    GL_WConfigManager_CurrentState_E = WCFG_STATE::WCFG_ERROR_INIT;
}


/* ******************************************************************************** */
/* Configuration Functions
/* ******************************************************************************** */
void WConfig_SetLanguage(unsigned char * pLanguage_UB) {

    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Set language");
    if (GL_GlobalData_X.Eeprom_H.read(WCONFIG_ADDR_LANGUAGE, GL_pWConfigBuffer_UB, 1) == 1) {

        // Get Byte and change it
        GL_pWConfigBuffer_UB[0] &= 0xF0;
        GL_pWConfigBuffer_UB[0] |= (*pLanguage_UB);

        // Write in EEPROM
        GL_GlobalData_X.Eeprom_H.write(WCONFIG_ADDR_LANGUAGE, GL_pWConfigBuffer_UB, 1);

        // Assign Global Config Data
        GL_GlobalConfig_X.Language_E = *((WLINK_LANGUAGE_ENUM *)(pLanguage_UB));
        DBG_PRINT(DEBUG_SEVERITY_INFO, "Language sets to  ");
        DBG_PRINTDATA(GL_pLanguageLut_str[*pLanguage_UB]);
        DBG_ENDSTR();

    }

}


void WConfig_SetDate(unsigned char * pDate_UB) {
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Set date");

    RTC_DATE_STRUCT Date_X;
    Date_X.Day_UB = pDate_UB[0] * 10 + pDate_UB[1];
    Date_X.Month_UB = pDate_UB[2] * 10 + pDate_UB[3];
    Date_X.Year_UB = pDate_UB[4] * 10 + pDate_UB[5];

    DBG_PRINT(DEBUG_SEVERITY_INFO, "Date sets to : ");
    DBG_PRINTDATA(Date_X.Day_UB);
    DBG_PRINTDATA("/");
    DBG_PRINTDATA(Date_X.Month_UB);
    DBG_PRINTDATA("/");
    DBG_PRINTDATA(Date_X.Year_UB);
    DBG_ENDSTR();

    // Call low-level function
    GL_GlobalData_X.Rtc_H.setDate(Date_X);
}


void WConfig_SetTime(unsigned char * pTime_UB) {
    DBG_PRINTLN(DEBUG_SEVERITY_INFO, "Set time");

    RTC_TIME_STRUCT Time_X;
    Time_X.Hour_UB = pTime_UB[0] * 10 + pTime_UB[1];
    Time_X.Min_UB = pTime_UB[2] * 10 + pTime_UB[3];
    Time_X.Sec_UB = pTime_UB[4] * 10 + pTime_UB[5];;

    DBG_PRINT(DEBUG_SEVERITY_INFO, "Time sets to : ");
    DBG_PRINTDATA(Time_X.Hour_UB);
    DBG_PRINTDATA(":");
    DBG_PRINTDATA(Time_X.Min_UB);
    DBG_PRINTDATA(":");
    DBG_PRINTDATA(Time_X.Sec_UB);
    DBG_ENDSTR();

    // Call low-level function
    GL_GlobalData_X.Rtc_H.setTime(Time_X);
}