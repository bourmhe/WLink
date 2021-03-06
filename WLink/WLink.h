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
#include "Utilz.h"

#include "SerialHandler.h"
#include "SerialManager.h"

#include "EepromWire.h"
#include "RealTimeClock.h"
#include "LcdDisplay.h"
#include "FlatPanel.h"
#include "FlatPanelManager.h"
#include "MemoryCard.h"

#include "Indicator.h"
#include "IndicatorManager.h"
#include "BadgeReader.h"
#include "BadgeReaderManager.h"

#include "WCommand.h"
#include "WCommandMedium.h"
#include "WCommandInterpreter.h"

#include "NetworkAdapter.h"
#include "NetworkAdapterManager.h"

#include "FonaModule.h"
#include "FonaModuleManager.h"

#include "TCPServer.h"
#include "TCPServerManager.h"
#include "UDPServer.h"
#include "UDPServerManager.h"

#include "WLinkManager.h"
#include "WMenuManager.h"

#include "KipControl.h"
#include "KipControlMedium.h"
#include "KipControlManager.h"
#include "KipControlMenu.h"

#include "Debug.h"

/* ******************************************************************************** */
/* Define
/* ******************************************************************************** */

#define WLINK_DEBUG_DEFAULT_COM_PORT        PORT_COM0
#define WLINK_DEBUG_DEFAULT_SPEED           115200


/* ******************************************************************************** */
/* Structure & Enumeration
/* ******************************************************************************** */

// Language Enumeration
typedef enum {
    WLINK_LANGUAGE_EN = 0,
    WLINK_LANGUAGE_FR = 1,
    WLINK_LANGUAGE_NL = 2
} WLINK_LANGUAGE_ENUM;

// WCommand Medium
typedef enum {
    WLINK_WCMD_MEDIUM_NONE = 0,
    WLINK_WCMD_MEDIUM_COM0 = 1,
    WLINK_WCMD_MEDIUM_COM1 = 2,
    WLINK_WCMD_MEDIUM_COM2 = 3,
    WLINK_WCMD_MEDIUM_COM3 = 4,
    WLINK_WCMD_MEDIUM_UDP_SERVER = 5,
    WLINK_WCMD_MEDIUM_TCP_SERVER = 6,
    WLINK_WCMD_MEDIUM_GSM_SERVER = 7
} WLINK_WCMD_MEDIUM_ENUM;

// Dedicated Structure for Ethernet Access Points
typedef struct {
    TCPServer TcpServer_H;
    UDPServer UdpServer_H;
    unsigned long GsmServer_H;  // TODO : to modify when GSMServer Object will be created
} ETHERNET_ACCESS_POINT_STRUCT;


// Global Parameters Strucure
typedef struct {
	unsigned char pRevisionId_UB[8];
    String RevisionId_Str;
	unsigned char LedPin_UB;
	unsigned char pGpioInputIndex_UB[4];
	unsigned char pGpioOutputIndex_UB[4];
    EepromWire Eeprom_H;
    RealTimeClock Rtc_H;
    LcdDisplay Lcd_H;
    FlatPanel FlatPanel_H;
    MemoryCard MemCard_H;
    NetworkAdapter Network_H;
    FonaModule Fona_H;
    ETHERNET_ACCESS_POINT_STRUCT EthAP_X;
    Indicator Indicator_H;          // Not yet managed
    BadgeReader BadgeReader_H;      // Not yet managed
	KipControl KipControl_H;
} GLOBAL_PARAM_STRUCT;



// Dedicated Strucure for COM Port Configuration
typedef struct {
    unsigned long Index_UL;
    boolean isEnabled_B;
    boolean isDebug_B;
    unsigned char Config_UB;
    unsigned long Baudrate_UL;
    void(*pFctCommEvent)(void);
} COM_PORT_CONFIG_STRUCT;

// Dedicated Structure for TCP/UDP Server Configuration
typedef struct {
    boolean isEnabled_B;
    unsigned int LocalPort_UI;
} SERVER_CONFIG_STRUCT;

// Dedicated Structure for Ethernet Configuration
typedef struct {
    boolean isEnabled_B;
    boolean isDhcp_B;
    boolean isAdvancedConfig_B;
    unsigned char pMacAddr_UB[6];
    IPAddress IpAddr_X;
    IPAddress SubnetMaskAddr_X;
    IPAddress GatewayAddr_X;
    IPAddress DnsIpAddr_X;
    SERVER_CONFIG_STRUCT TcpServerConfig_X;
    SERVER_CONFIG_STRUCT UdpServerConfig_X;
} ETHERNET_CONFIG_STRUCT;

// Dedicated Structure for GSM Module Configuration
typedef struct {
    boolean isEnabled_B;
    char ComIndex_UB;
    char PinRst_UB;
    char PinKey_UB;
    char PinPower_UB;
    char pPinCode_UB[5];
    unsigned long ApnIndex_UL;
} GSM_CONFIG_STRUCT;

// Dedicated Structure for WCommand Configuration
typedef struct {
    WLINK_WCMD_MEDIUM_ENUM Medium_E;
    boolean isMonoClient_B;
    const WCMD_FCT_DESCR * pFctDescr_X;
    unsigned long NbFct_UL;
} WCMD_CONFIG_STRUCT;

// Dedicated Structure for WApplication Configuration

typedef struct {
    boolean hasApplication_B;
	boolean hasMenu_B;
    void (*pFctInit)(void *);
    void (*pFctEnable)(void);
    void (*pFctDisable)(void);
    boolean (*pFctIsEnabled)(void);
    void (*pFctProcess)(void);
	void (*pFctInitMenu)(void);
	WMENU_ITEM_STRUCT * (*pFctGetFirstItem)(void);
} WAPP_STRUCT;

// Dedicated Structure for Indicator Configuration
typedef struct {
	unsigned char ComPortIdx_UB;
	INDICATOR_INTERFACE_DEVICES_ENUM InterfaceType_E;
    INDICATOR_INTERFACE_FRAME_ENUM InterfaceFrame_E;
	boolean HasIrq_B;
	boolean HasEcho_B;
	unsigned char EchoComPortIx_UB;
} INDICATOR_CONFIG_STRUCT;

// Global Configuration Structure
typedef struct {
    unsigned char MajorRev_UB;
    unsigned char MinorRev_UB;
    WLINK_LANGUAGE_ENUM Language_E;
    WCMD_CONFIG_STRUCT WCmdConfig_X;
    boolean HasLcd_B;
    boolean HasFlatPanel_B;
    boolean HasMemoryCard_B;
    COM_PORT_CONFIG_STRUCT pComPortConfig_X[4];
    ETHERNET_CONFIG_STRUCT EthConfig_X;
    GSM_CONFIG_STRUCT GsmConfig_X;
    WAPP_STRUCT App_X;
	INDICATOR_CONFIG_STRUCT pIndicatorConfig_X[4];
} GLOBAL_CONFIG_STRUCT;

#endif // __WLINK_H__
