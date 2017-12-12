//******************************************************************************
/// @file 01847-temp.c
/// @brief Applicativo di test per l'articolo 01847
/// $Author$
/// $Date$
/// $Revision$
///
///
/// @note La storia dello sviluppo del modulo è descritta in @ref revnote01847-test
//******************************************************************************

//
// -----
// $Id$
// -----
//


/**
*@page revnote01847-test Revision History del modulo 01847-test
*@section revnote01847-test_rev0 giursino 23:40:27 23 mar 2017
* - Creazione del modulo.
*/

//-START--------------------------- Definitions ------------------------------//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <hidapi/hidapi.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "config.h"
#include "libknxusb.h"
#include "01847-temp.h"

#ifdef LOCAL
	#undef LOCAL
#endif
#define LOCAL static
#ifdef GLOBAL
	#undef GLOBAL
#endif
#define GLOBAL


//-END----------------------------- Definitions ------------------------------//


//-START----------------------- Functions Declaration ------------------------//
LOCAL void LogPrintMsg(const char* strprefix, const uint8_t* pMsg, uint8_t u8Len);
//-END------------------------- Functions Declaration ------------------------//


//-START------------------------------ Const ---------------------------------//
//-END-------------------------------- Const ---------------------------------//


//-START----------------------------- Variables ------------------------------//
//-PUBLIC-
//-PRIVATE-
LOCAL char *socket_path = SOCKET_FILE;
//-END------------------------------ Variables -------------------------------//


//-START------------------------------- ISR ----------------------------------//
//-END--------------------------------- ISR ----------------------------------//


//-START--------------------------- Functions --------------------------------//

/// Print hex message on log
LOCAL void LogPrintMsg(const char* strprefix, const uint8_t* pMsg, uint8_t u8Len) {
	uint8_t i=0;
	fprintf(stdout, "%s: ", strprefix);
	for (i=0; i<u8Len; i++){
		fprintf(stdout, "%.2X ", pMsg[i]);
	}
	fprintf(stdout, ".\n");
}

/// Print hex message on recevie buffer
LOCAL void PrintReceivedMsg(const char* strprefix, const uint8_t* pMsg, uint8_t u8Len) {
	uint8_t i=0;
	fprintf(stdout, "%s: ", strprefix);
	for (i=0; i<u8Len; i++){
		switch (i) {
			case 1:
			case 3:
			case 5:
			case 6:
			case 7:
			case 8:
				fprintf(stdout, " ");
				break;

			default:
				break;
		}
		fprintf(stdout, "%.2X", pMsg[i]);
	}
	fprintf(stdout, ".\n");
}


// Remove space from string
LOCAL void RemoveSpace (char* str) {
	char *write = str, *read = str;
	do {
	   if (*read != ' ')
	       *write++ = *read;
	} while (*read++);
}

// Send message
LOCAL int SendStringMsg (hid_device* pDevice, char* strmsg) {
	int len, i, res;
	uint8_t msg[LKU_KNX_MSG_LENGTH];

	RemoveSpace(strmsg);

	if (strmsg == NULL) {
		return -1;
	}
	len = strlen(strmsg);
	if (len <= 0) {
		return -1;
	}
	if (len % 2) {
		return -1;
	}
	if (len > (LKU_KNX_MSG_LENGTH*2)) {
		return -1;
	}

	for (i=0; i<len; i+=2) {
		char strbyte[3];
		char *err;
		strbyte[0]=strmsg[i];
		strbyte[1]=strmsg[i+1];
		strbyte[2]='\0';
		msg[i/2] = (uint8_t) strtoul(strbyte, &err, 16);
		if (*err != '\0') {
			return -1;
		}
	}
	len/=2;

	res = LKU_SendRawMessage(pDevice, msg, len);
	if (res < 0) {
		return -1;
	}
	LogPrintMsg("SendStringMsg", msg, len);

	return len;
}

LOCAL float DptValueTemp2Float (uint8_t dpt[2]) {
	uint16_t t_raw = (dpt[0] << 8) + dpt[1];
	int16_t m = t_raw & 0x7FF;
	if ((t_raw & 0x8000) == 0x8000) {
		m = -m;
	}
	uint8_t e = (uint8_t) ((t_raw & 0x7800) >> 11);
	float t = (0.01*m) * powf(2,e);

#if DEBUG
	fprintf(stdout, "* t_raw=0x%.4X\n", t_raw);
	fprintf(stdout, "* m=%i (0x%.4X)\n", m, (uint16_t) m);
	fprintf(stdout, "* e=%i (0x%.4X)\n", e, e);
	fprintf(stdout, "* t=%f\n", t);
	fprintf(stdout, "* 2^e=%f\n", powf(2,e));
	fprintf(stdout, "* 0.01*m=%f\n", 0.01*m);
#endif

	return t;
}

/// Thread function to handle the receiving of messages
LOCAL void* ThreadKnxRx(void *arg) {
	int res;
	uint8_t buf[65];
	hid_device* pDevice = (hid_device*) arg;

	while(1) {
		res = LKU_ReceiveRawMessage(pDevice, buf, 65);
		if (res < 0) {
			perror("Error receiving data.");
			exit(1);
		}

		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char sTime[64];
		strftime(sTime, sizeof(sTime), "%c", tm);
		PrintReceivedMsg(sTime, buf, res);


		if ((buf[3]==0x0C) && (buf[4]==0x72)) {
			float t = DptValueTemp2Float(&buf[8]);
			fprintf(stdout, "*** Zona giorno, Ta=%.1f\n", t);
		}

		if ((buf[3]==0x0C) && (buf[4]==0x99)) {
			float t = DptValueTemp2Float(&buf[8]);
			fprintf(stdout, "*** Zona notte, Ta=%.1f\n", t);
		}

	}
	return NULL;
}

/// Main function
///
int main(int argc, char* argv[]) {

	printf("Welcome to %s.\n", PACKAGE_STRING);

	hid_device* pDevice;
	int res;
	int ch;
	bool toexit = false;


	// Init Lib Knx Usb
	res = LKU_Init(&pDevice);
	if (res < 0) {
		perror("LKU_Init");
		exit(1);
	}


	// Create socket to comunicate with other modules
	int fd;
#ifdef CONNECTION_ORIENTED
	if ( (fd = socket(AF_UNIX, SOCK_STREAM , 0)) == -1) {
		perror("Socket error");
		exit(1);
	}
#else
	if ( (fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("Socket error");
		exit(1);
	}
#endif

	// Binding address to socket
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
		unlink(socket_path);
	}
	if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		perror("Bind error");
		exit(1);
	}

#ifdef CONNECTION_ORIENTED
	// Listening to new connections
	if (listen(fd, 5) == -1) {
		perror("listen error");
		exit(1);
	}

	// Accepting new connections
	int cl;
	if ((cl = accept(fd, NULL, NULL)) == -1) {
		perror("accept error");
		exit(1);
	}
#else
	int cl = fd;
#endif

	// Creating thread to handle the received message
	pthread_t threadRx;
	if (pthread_create(&threadRx, NULL, ThreadKnxRx, pDevice)) {
		perror("Error creating thread");
		exit(1);
	}

	printf("Press 'q' to quit...\n");
	while(!toexit) {

		ch = getchar();
		if (ch == 'q') {
			toexit = true;
		}

#if DEBUG
	{
		uint8_t tfix[2] = {0x0C, 0x1A};
		float t = DptValueTemp2Float(tfix);
		printf("t=%.1f \n\n", t);
	}
	{
		uint8_t tfix[2] = {0x8C, 0x1A};
		float t = DptValueTemp2Float(tfix);
		printf("t=%.1f \n\n", t);
	}
	{
		uint8_t tfix[2] = {0x14, 0x1A};
		float t = DptValueTemp2Float(tfix);
		printf("t=%.1f \n\n", t);
	}
	{
		uint8_t tfix[2] = {0x0C, 0x1B};
		float t = DptValueTemp2Float(tfix);
		printf("t=%.1f \n\n", t);
	}

	{
		char* buf = "pippo\n";
		int rc = strlen(buf);
		printf("send data [%s, %i bytes] to socket [%i] \n", buf, rc, cl);

		if (write(cl, buf, rc) != rc) {
			if (rc > 0)
				fprintf(stderr, "partial write");
			else {
				perror("write error");
				exit(-1);
			}
		}
	}
#endif

	{
		SocketData_Type buf;
		int rc = sizeof(buf);
		buf.time=12;
		buf.temperature=12.5;
		printf("send data [%p, %i bytes] to socket [%i] \n", &buf, rc, cl);

		if (write(cl, &buf, rc) != rc) {
			if (rc > 0)
				fprintf(stderr, "partial write");
			else {
				perror("write error");
				exit(-1);
			}
		}
	}


	}

	// End thread
	if (pthread_cancel(threadRx)) {
		perror("Error ending thread");
		exit(1);
	}
	if (pthread_join(threadRx, NULL)) {
		perror("Error waiting thread");
		exit(1);
	}

	// Destroy socket
	close(cl);
	close(fd);
	unlink(socket_path);
	// TODO remove file

	// Deinit Lib Knx Usb
	res = LKU_Deinit(pDevice);
	if (res < 0) {
		perror("LKU_Deinit");
		exit(1);
	}

	printf("Done.\n");

	return 0;
}

//-END----------------------------- Functions --------------------------------//










