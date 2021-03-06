//******************************************************************************
/// @brief Example application to monitor temperature using KNX Interface VIMAR 01847
///
/// @note It create a socket server to send the temperature values gets from KNX bus
//******************************************************************************

#ifndef __01847_TEMP_H_
#define __01847_TEMP_H_

//-START--------------------------- Definitions ------------------------------//

#ifdef LOCAL
	#undef LOCAL
#endif
#define LOCAL static			///< Identifica tutti gli oggetti a carattere locale
#ifdef GLOBAL
	#undef GLOBAL
#endif
#define GLOBAL extern			///< Identifica tutti gli oggetti a carattere globale

#define SOCKET_FILE		"/tmp/01847-temp"

//#define CONNECTIONLESS
#define CONNECTION_ORIENTED

#define DAEMON
//-END----------------------------- Definitions ------------------------------//


//-START------------------------------ Types ---------------------------------//
typedef struct {
	char time[32];
	char track[32];
	float value;

} SocketData_Type;

typedef struct {
	hid_device* pDevice;
	int socket;

} ThreadKnxArgs_Type;
//-END-------------------------------- Types ---------------------------------//

//-START------------------------------ Macro ---------------------------------//
//-END-------------------------------- Macro ---------------------------------//

//-START------------------------------ Const ---------------------------------//
//-END-------------------------------- Const ---------------------------------//

//-START---------------------------- Variables -------------------------------//
//-END------------------------------ Variables -------------------------------//

//-START----------------------- Functions Declaration ------------------------//
//-END------------------------- Functions Declaration ------------------------//
#endif
