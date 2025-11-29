/*
 * GSM.c
 *
 *  Created on: Nov 18, 2025
 *      Author: fervi
 */

#include "Serial_UART.h"
#include "GSM.h"
#include "Credentials.h"
#include "string.h"
#include "stdio.h"


char UartString[UartBufLen];
char payloadBuf[payBufLen];

extern GSMPar GSMVari;
extern TimeStampLog TimeFormat;
extern OTAPra OTAVar;
/**
 * STM32 version of Arduino atSend()
 *
 *  AT           : AT command string ("AT\r\n", "AT+CSQ\r\n", "0x1A", etc.)
 *  respons      : substring to look for in module response ("OK", "ERROR", "+CSQ:", etc.)
 *  responseTime : max wait per try (ms)
 *  tryFor       : how many times to retry the command
 */
uint8_t atSend(const char *AT, const char *respons, uint32_t responseTime,
		uint16_t tryFor) {
	// Same reset check logic as your original (optional)
	if (GSMVari.ResetStatus != NotInReset) {
		printf("\n\tEr: Module In reseting count %d\n", GSMVari.FaildCount);
		return 0;
	}

	uint32_t startTime = HAL_GetTick();
	memset(UartString, 0, sizeof(UartString));

	printf("Lg AT Send %s\n", AT);

	for (uint16_t id = 1; id <= tryFor; id++) {
		printf("Try = %hu\n", id);

		// ---------- Send AT command ----------
		if (strcmp(AT, "0x1A") == 0) {
			// Send Ctrl+Z
			uint8_t ctrlZ = 0x1A;
			Serial_Write(&ctrlZ, 1);
		} else {
			Serial_Print(AT);
		}

		startTime = HAL_GetTick();
		int bytesRead = 0;
		memset(UartString, 0, sizeof(UartString));

		// ---------- Wait for response ----------
		while ((HAL_GetTick() - startTime) < responseTime) {
			if (Serial_Available()) {

				// Read all bytes currently in buffer (up to UartString size)
				while (Serial_Available()
						&& (bytesRead < (int) sizeof(UartString) - 1)) {
					uint8_t c;
					if (Serial_ReadByte(&c)) {
						UartString[bytesRead++] = (char) c;
					} else {
						break;
					}
				}

				UartString[bytesRead] = '\0';

				// ---------- Your special handlers ----------

				// +CSQ:
				if (strstr(UartString, "+CSQ:") != NULL) {
					if (strstr(respons, "+CSQ:") != NULL) {
						handleReceivedData(UartString,0);
					}
				}
				// +QLTS:
				else if (strstr(UartString, "+QLTS:") != NULL) {
					if (strstr(respons, "+QLTS:") != NULL) {
						handleReceivedData(UartString,0);
					}
				}
				// +QMTRECV: (MQTT)
				else if (strstr(UartString, "+QMTRECV:") != NULL) {
					printf("\n\tLg: in the QMTRECV\n");

					if (strstr(UartString, "}") == NULL) {
						printf("\n\tLg: not have }\n");

						while ((HAL_GetTick() - startTime) < responseTime) {
							while (Serial_Available()
									&& bytesRead < (int) sizeof(UartString) - 1) {
								uint8_t prasentChar;
								if (Serial_ReadByte(&prasentChar)) {
									UartString[bytesRead++] =
											(char) prasentChar;
								} else {
									break;
								}
							}

							if (strstr(UartString, "}") != NULL) {
								printf("\n\tLg: found }\n");
								break;
							} else {
								printf("%s\n", UartString);
							}
						}
					}

					UartString[bytesRead] = '\0';
					printf("%s\n", UartString);
					handleReceivedData(UartString,0);
				}

				// Extra logging for MQTT ATs
				if ((strstr(AT, "AT+QMT") != NULL)
						|| (strstr(AT, "AT+CMGL") != NULL)
						|| (strstr(AT, "AT+CPMS") != NULL)) {
					printf("\t\tLg: GSM Respons\n");
					printf("%s\n", UartString);
				}

				// Check if expected response substring is present
				if (strstr(UartString, respons) != NULL) {
					printf("\n\tLg: AT respons Verfied\n");
					return 1;
				} else {
					printf("Receive:-%s\n", UartString);
				}

			}

			// If you are using FreeRTOS, replace with osDelay(1) or vTaskDelay(1)
			HAL_Delay(1);
		}
	}

	GSMVari.FaildCount++;
	printf("\n\tEr: Failed to get respons count %d\n", GSMVari.FaildCount);

	return 0;
}

// ---------------- GSMInit ----------------

void GSMInit(void) {
	printf("Lg: GSM Init\n");
	const uint16_t responseTime = 1000;
	const uint8_t tryFor = 1;

	atSend("AT\r\n", "OK", responseTime, 10);
	atSend("ATE0\r\n", "OK", responseTime, tryFor);
	atSend("AT+CPIN?\r\n", "+CPIN: READY", responseTime, tryFor);
	atSend("AT+CFUN=1\r\n", "OK", responseTime, tryFor);

	// UART Flow Control
	HAL_Delay(responseTime);

	// GPRS configuration
	atSend("AT+CREG?\r\n", "+CREG:", responseTime, tryFor);
	atSend("AT+CGREG?\r\n", "+CGREG:", responseTime, tryFor);
	atSend("AT+COPS?\r\n", "+COPS:", responseTime, tryFor);

//    atSend("AT+CGATT?\r\n",       "+CGATT:",        responseTime, tryFor);
//    atSend("AT+CGATT=0\r\n",      "OK",             responseTime, tryFor);
//    atSend("AT+CGATT=1\r\n",      "OK",             responseTime, tryFor);
//    atSend("AT+CGDCONT=1,\"IP\",\"internet\"\r\n",
//                                 "OK",             responseTime, tryFor);
//    atSend("AT+CGACT=1,1\r\n",    "OK",             responseTime, tryFor);

	HAL_Delay(responseTime);
	atSend("AT+QIACT\r\n", "OK", responseTime, tryFor);

	// SMS config
	atSend("AT+CMGF=1\r\n", "OK", 2000, 3);
	atSend("AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n", "OK", 2000, 3); // use Module memory (ME)
	atSend("AT+CNMI=1,1,0,0,0\r\n", "OK", 2000, 3);
	atSend("AT+CSCS=\"IRA\"\r\n", "OK", 2000, 2);
	atSend("AT+CPMS?\r\n", "OK", 2000, 3);

	printf("Lg: GSM Done\n");
}

// ---------------- generateRandomID ----------------

static char* generateRandomID(void) {
	static char randomID[20];

	// Use system tick as seed
//    srand((unsigned int)HAL_GetTick());
//
//    uint16_t randomValue = (uint16_t)(rand() & 0xFFFF);
//    snprintf(randomID, sizeof(randomID), "%05u", randomValue);

	return randomID;
}

// ---------------- GSMConnectMqtt ----------------

uint8_t GSMConnectMqtt(void) {
	const uint16_t responseTime = 6000;
	const uint8_t tryFor = 1;
	char atCommand[228] = { 0 };
	char WILLData[228] = { 0 };

	printf("Lg: Connecting Mqtt\n");

	atSend("AT+QMTCLOSE=1\r\n", "OK", 1000, tryFor);
	atSend("AT+QMTDISC=1\r\n", "OK", 1000, tryFor);

	// Build WILL payloadBuf
	snprintf(WILLData, sizeof(WILLData),
			"{\"projectId\" : \"%s\",\"deviceId\" : \"%s\","
					"\"St\" : \"Disconnected\",\"GWID\" : \"%s\"}",
			MqttprojectId, MqttdeviceId, MqttdeviceId);

	uint16_t LengthData = (uint16_t) strlen(WILLData);

	snprintf(atCommand, sizeof(atCommand),
			"AT+QMTCFG=\"willex\",1,1,1,0,\"%s\",%d\r\n",
			MqttPubTopic, LengthData);

	if (atSend(atCommand, ">", responseTime, tryFor)) {
		// Send WILL payloadBuf
		printf("-->Sending WILL....\n");
		Serial_Write((uint8_t*) WILLData, LengthData);
		printf("Wait!\n");
		HAL_Delay(2);

		// Just wait for "OK" after payloadBuf
		if (atSend("", "OK", responseTime, 1)) {
			printf("Lg: Will configured\n");
		} else {
			printf("Er: Will payloadBuf failed\n");
		}
	}

	// Open MQTT connection
	snprintf(atCommand, sizeof(atCommand), "AT+QMTOPEN=1,\"%s\",1883\r\n",
	MqttBroker);

//    GetRTCCurrentTime();

	if (atSend(atCommand, "+QMTOPEN:", responseTime, tryFor)) {
		char *randomID = generateRandomID();
		snprintf(atCommand, sizeof(atCommand),
				"AT+QMTCONN=1,\"%s%s\",\"%s\",\"%s\"\r\n",
				MqttclientIDNo, randomID, MqttuserName, MqttPassword);

		if (atSend(atCommand, "+QMTCONN: 1,0,0\r\n", responseTime, tryFor)) {
			printf("Lg: Connected Mqtt\n");

			// Publish status
			snprintf(atCommand, sizeof(atCommand),
					"AT+QMTPUB=1,1,1,0,\"%s\"\r\n", MqttPubTopic);

			if (atSend(atCommand, ">", responseTime, tryFor)) {
				char payloadBuf[228];
				snprintf(payloadBuf, sizeof(payloadBuf),
						"{\"projectId\" : \"%s\",\"deviceId\" : \"%s\",\"St\" : \"Connected\","
								"\"TimeStamp\" : \"%s\",\"GWID\" : \"%s\"}",
						MqttprojectId, MqttdeviceId, TimeFormat.TimeStamp,
						MqttdeviceId);

				uint16_t plen = (uint16_t) strlen(payloadBuf);

				printf("-->Sending status....\n");
				Serial_Write((uint8_t*) payloadBuf, plen);
				printf("Wait!\n");
				HAL_Delay(2);

				if (atSend("0x1A", "+QMTPUB: 1,1,0", responseTime, tryFor)) {
					GSMVari.FaildCount = 0;
					printf("\t\tLg:Done Pub\n");
				} else {
					printf("Er: Status Pub Failed_1\n");
				}
				GSMVari.MessStarus = 1;
			} else {
				printf("Er: Status Pub Failed\n");
			}

			// Subscribe
			snprintf(atCommand, sizeof(atCommand),
					"AT+QMTSUB=1,1,\"sensor/output/%s/%s\",1\r\n",
					MqttprojectId, MqttdeviceId);

			if (atSend(atCommand, "+QMTSUB: 1,1,0,1", responseTime, tryFor)) {
				printf("Lg: Sub Done\n");
			}

			GSMVari.MessStarus = 1;
			return 1;
		}
	}

	printf("Wr:Mqtt Disconnected\n");
	return 0;
}

// ---------------- GSM SMS ----------------
// ---------- delete SMS by index ----------
static void GSM_DeleteSmsByIndex(int index) {
	char delCmd[32];
	snprintf(delCmd, sizeof(delCmd), "AT+CMGD=%d,0\r\n", index);
	atSend(delCmd, "OK", 2000, 2);
}

// ---------- get used SMS count via AT+CPMS? (returns used, -1 on error) ----------
static int GSM_GetSmsUsedCount(void) {
	if (!atSend("AT+CPMS?\r\n", "+CPMS:", 2000, 2)) {
		return -1;
	}
	char *p = strstr(UartString, "+CPMS:");
	if (!p)
		return -1;
	int used = -1, total = -1;
	if (sscanf(p, "+CPMS: \"%*[^\"]\",%d,%d", &used, &total) == 2) {
		(void) total;
		return used;
	}
	return -1;
}

static int parse_sender_from_cmgr_header(const char *buf, char *outNumber,
		size_t outSize) {
	// buf contains header+body (header starts with "+CMGR:")
	if (!buf || !outNumber || outSize == 0)
		return 0;
	outNumber[0] = '\0';

	const char *hdr = strstr(buf, "+CMGR:");
	if (!hdr)
		hdr = strstr(buf, "+CMGL:"); // tolerate +CMGL too if needed
	if (!hdr)
		return 0;

	// find first quote (status), then second quote (end status),
	// then third quote (start number) and fourth quote (end number)
	const char *p = strchr(hdr, '\"');
	if (!p)
		return 0;
	p = strchr(p + 1, '\"');
	if (!p)
		return 0;
	p = strchr(p + 1, '\"');
	if (!p)
		return 0;
	p++; // now at start of number
	const char *q = strchr(p, '\"');
	if (!q)
		return 0;

	size_t len = (size_t) (q - p);
	if (len == 0 || len >= outSize)
		return 0;
	memcpy(outNumber, p, len);
	outNumber[len] = '\0';
	return 1;
}

// Return pointer to SMS body inside u (header must start with +CMGR: or +CMGL:).
// The function will NUL-terminate the body in-place at the earliest terminator found.
// Returns NULL on failure.
static char* sms_body_ptr_in_uartstring(char *u) {
	if (!u)
		return NULL;

	// Find header for this message
	char *hdr = strstr(u, "+CMGR:");
	if (!hdr)
		hdr = strstr(u, "+CMGL:");
	if (!hdr)
		return NULL;

	// Find end of header line
	char *hdr_end = strstr(hdr, "\r\n");
	if (!hdr_end)
		hdr_end = strchr(hdr, '\n');
	if (!hdr_end)
		return NULL;

	// Body starts after header CRLF (handle CRLF or LF)
	char *body = hdr_end + ((hdr_end[0] == '\r' && hdr_end[1] == '\n') ? 2 : 1);
	// skip any leading CR/LF
	while (*body == '\r' || *body == '\n')
		body++;

	// Now find earliest terminator among candidates
	char *t_ok = strstr(body, "\r\nOK");
	char *t_cmgr = strstr(body, "\r\n+CMGR");
	char *t_cmgl = strstr(body, "\r\n+CMGL");
	char *t_cmgr_nl = strstr(body, "\n+CMGR");
	char *t_cmgl_nl = strstr(body, "\n+CMGL");
	char *t_ok_nl = strstr(body, "\nOK");

	// pick earliest non-NULL pointer
	char *terms[] =
			{ t_ok, t_ok_nl, t_cmgr, t_cmgr_nl, t_cmgl, t_cmgl_nl, NULL };
	char *earliest = NULL;
	for (int i = 0; terms[i] != NULL; ++i) {
		char *p = terms[i];
		if (!p)
			continue;
		if (!earliest || p < earliest)
			earliest = p;
	}

	if (earliest) {
		// Null-terminate at earliest terminator (remove preceding CR/LF if present)
		// If earliest points at "\r\nOK" we want to remove the preceding CRLF
		// Trim possible preceding CR/LF characters first
		char *cut = earliest;
		// remove any trailing CRLF before terminator
		while (cut > body && (*(cut - 1) == '\r' || *(cut - 1) == '\n'))
			--cut;
		*cut = '\0';
	} else {
		// No explicit terminator found: trim trailing CR/LF
		size_t L = strlen(body);
		while (L > 0 && (body[L - 1] == '\r' || body[L - 1] == '\n')) {
			body[L - 1] = '\0';
			--L;
		}
	}

	// If body is empty return NULL
	if (body[0] == '\0')
		return NULL;
	return body;
}



// returns 1 if reassembled JSON looks complete (balanced braces), else 0
static int json_braces_balanced(const char *s) {
	if (!s)
		return 0;
	int depth = 0;
	int in_str = 0;
	for (const char *p = s; *p; ++p) {
		char c = *p;
		if (c == '"' && (p == s || *(p - 1) != '\\')) {
			in_str = !in_str;
			continue;
		}
		if (in_str)
			continue;
		if (c == '{')
			++depth;
		else if (c == '}')
			--depth;
		if (depth < 0)
			return 0;
	}
	return (depth == 0 && strchr(s, '{') != NULL && strchr(s, '}') != NULL) ?
			1 : 0;
}

// Main: process unread parts, reassemble per master sender
void GSM_ProcessStoredSmsFromMaster_Reassemble(void) {

	int n = GSM_GetSmsUsedCount();
	if (n <= 0)
		return;
	printf("\t\tGSM_Process\n\n");
	printf("Unread_indices %d\n", n);
	memset(payloadBuf,0,sizeof(payloadBuf));

	for (uint8_t idx = 0; idx < n; ++idx) {
		char cmd[32];
		snprintf(cmd, sizeof(cmd), "AT+CMGR=%d\r\n", idx);

		if (!atSend(cmd, "OK", 3000, 2)) {
			printf("idx %d no response\n", idx);
			continue;
		}

		// Must have header
		if (!strstr(UartString, "+CMGR:") && !strstr(UartString, "+CMGL:")) {
			printf("no CMGR/CMGL header for idx %d; deleting\n", idx);
			GSM_DeleteSmsByIndex(idx);
			continue;
		}

		// parse sender
		char sender[48] = { 0 };
		if (!parse_sender_from_cmgr_header(UartString, sender,
				sizeof(sender))) {
			printf("can't parse sender idx %d; delete\n", idx);
			GSM_DeleteSmsByIndex(idx);
			continue;
		}

		// not from master -> delete and continue
		if (strstr(sender, MasPhoneNo) == NULL) {
			printf("idx %d not from master (%s); deleting\n", idx, sender);
			GSM_DeleteSmsByIndex(idx);
			continue;
		}

		// extract body pointer and append to payloadBuf
		char *body = sms_body_ptr_in_uartstring(UartString);
		if (!body) {
			printf("idx %d empty body; delete\n", idx);
			GSM_DeleteSmsByIndex(idx);
			continue;
		}

		size_t cur = strlen(payloadBuf);
		size_t add = strlen(body);
		if (cur + add >= sizeof(payloadBuf) - 1) {
			printf("payloadBuf buffer overflow, aborting at idx %d\n", idx);
			GSM_DeleteSmsByIndex(idx); // remove broken part
			break;
		}

		memcpy(payloadBuf + cur, body, add);
		payloadBuf[cur + add] = '\0';

		// delete this part after appending
		GSM_DeleteSmsByIndex(idx);
		HAL_Delay(50); // small pause

		printf("payloadBuf:-%s\n", payloadBuf);

		// check completion
		if (json_braces_balanced(payloadBuf)) {
			printf("json_braces_balanced\n");
			handleReceivedData(payloadBuf,1);
			payloadBuf[0] = '\0'; // reset for next message
			// continue scanning further indices for additional messages
		}
	}
}
/* Extract JSON string value: "key":"value"  (no escaping support, but enough for your config) */
static int json_get_string(const char *json, const char *key, char *out,
		size_t outSize) {
	if (!json || !key || !out || outSize == 0)
		return 0;

	char pattern[64];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);

	const char *p = strstr(json, pattern);
	if (!p)
		return 0;

	p += strlen(pattern);
	while (*p && (*p == ' ' || *p == '\t'))
		p++;
	if (*p != ':')
		return 0;
	p++;
	while (*p && (*p == ' ' || *p == '\t'))
		p++;
	if (*p != '\"')
		return 0;
	p++; // skip opening quote

	size_t i = 0;
	while (*p && *p != '\"' && i < outSize - 1) {
		out[i++] = *p++;
	}
	out[i] = '\0';

	if (*p != '\"')
		return 0; // no closing quote
	return 1;
}

/* Extract JSON integer value: "key":12345 */
static int json_get_int(const char *json, const char *key, int *outVal) {
	if (!json || !key || !outVal)
		return 0;

	char pattern[64];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);

	const char *p = strstr(json, pattern);
	if (!p)
		return 0;

	p += strlen(pattern);
	while (*p && (*p == ' ' || *p == '\t'))
		p++;
	if (*p != ':')
		return 0;
	p++;
	while (*p && (*p == ' ' || *p == '\t'))
		p++;

	if (sscanf(p, "%d", outVal) == 1)
		return 1;

	return 0;
}

/*
 * Brief: handle the Receive data from the GSM (QMTS, CSQ, QMTRECV, CMT)
 */
void handleReceivedData(const char *data, uint8_t FromSMS) {
	if (data == NULL)
		return;

	/* ----------- +QLTS:  (Time sync) ----------- */
	const char *qltsStart = strstr(data, "+QLTS:");
	if (qltsStart != NULL) {
		// Move past '+QLTS: "'
		qltsStart += strlen("+QLTS: \"");

		char qltsString[30] = { 0 };
		strncpy(qltsString, qltsStart, sizeof(qltsString) - 1);

		const char *q = strstr(qltsString, ",0\"");
		if (q != NULL) {
			printf("TimeStamp from the GS %s_End\n", qltsString);

			uint16_t values[6];
			if (sscanf(qltsString, "%hu/%hu/%hu,%hu:%hu:%hu", &values[0],
					&values[1], &values[2], &values[3], &values[4], &values[5])
					== 6) {
				TimeFormat.Date = values[2];
				TimeFormat.Month = values[1];
				TimeFormat.Year = values[0];
				TimeFormat.Hour = values[3];
				TimeFormat.Minute = values[4];
				TimeFormat.Second = values[5];

				snprintf(TimeFormat.TimeStamp, sizeof(TimeFormat.TimeStamp),
						"%02d-%02d-%02d %02d:%02d:%02d", values[2], values[1],
						values[0], values[3], values[4], values[5]);

				snprintf(TimeFormat.DataStamp, sizeof(TimeFormat.DataStamp),
						"%02d-%02d-%02d", values[2], values[1], values[0]);

//                setRTCFromGSM(TimeFormat.TimeStamp);
				printf("Lg:Time %s Date %s \n", TimeFormat.TimeStamp,
						TimeFormat.DataStamp);
			} else {
				printf("\n\tEr TimeStamp Not in the Order\n");
			}
		}
	}

	/* ----------- +CSQ:  (RSSI) ----------- */
	const char *rssiStart = strstr(data, "+CSQ:");
	if (rssiStart != NULL) {
		printf("RSSI from the GS %s_End\n", rssiStart);
		int rssiValue = 0;
		if (sscanf(rssiStart, "+CSQ: %d,", &rssiValue) == 1) {
			if (rssiValue == 0)
				GSMVari.RSSI = -113;
			if (rssiValue == 1)
				GSMVari.RSSI = -111;
			if (rssiValue >= 2 && rssiValue <= 30)
				GSMVari.RSSI = -113 + 2 * rssiValue;
			if (rssiValue == 31)
				GSMVari.RSSI = -51;
			if (rssiValue == 99)
				GSMVari.RSSI = 99;

			printf("RSSI: %d dBm\n", GSMVari.RSSI);
		}
	}

	/* ----------- +QMTRECV:  (MQTT JSON config) ----------- */
//    if (strstr(data, "+QMTRECV:") != NULL)
//    {
//        printf("Lg: Config JSON\n");
//
//        const char* jsonStart = strchr(data, '{');
//        const char* jsonEnd   = strrchr(data, '}');
//
//        if (jsonStart == NULL || jsonEnd == NULL || jsonEnd < jsonStart)
//        {
//            printf("JSON start or end not found, or invalid format\n");
//            return;
//        }
//        else
//        {
//            printf("Lg: JSON start or end found\n");
//        }
//
//        if (strlen(data) > 1024)
//        {
//            printf("Wr: Config Buffer is Out of range size is %lu\n",
//                   (unsigned long)strlen(data));
//            return;
//        }
//
//        size_t jsonLength = (size_t)(jsonEnd - jsonStart + 1);
//        char jsonBuffer[1024 + 1]; // ensure big enough
//        if (jsonLength > sizeof(jsonBuffer) - 1)
//        {
//            printf("Wr: JSON too big for buffer\n");
//            return;
//        }
//
//        strncpy(jsonBuffer, jsonStart, jsonLength);
//        jsonBuffer[jsonLength] = '\0';
//
//        printf("Lg: JSON raw: %s\n", jsonBuffer);
//
//        /* Simple manual JSON parsing (no ArduinoJson) */
//        char projectId[64];
//        if (!json_get_string(jsonBuffer, "projectId", projectId, sizeof(projectId)))
//        {
//            printf("Er: projectId not found in JSON\n");
//            return;
//        }
//
//        if (strcmp(projectId, MqttprojectId) == 0)
//        {
//            char configURL[256];
//            int  configSize = 0;
//
//            int hasURL  = json_get_string(jsonBuffer, "ConfigURL", configURL, sizeof(configURL));
//            int hasSize = json_get_int   (jsonBuffer, "Configsize", &configSize);
//
//            if (hasURL && hasSize)
//            {
//                printf("Lg: In OTA Config\n");
//
//                memset(OTAVar.ConfigURLHold, 0, sizeof(OTAVar.ConfigURLHold));
//                strncpy(OTAVar.ConfigURLHold, configURL, sizeof(OTAVar.ConfigURLHold) - 1);
//
//                OTAVar.ConfigfileSize = configSize;
//                OTAVar.OTARequest     = 1;
//
//                printf("Verified Project ID: %s\n", projectId);
//                printf("Config URL: %s\n",       OTAVar.ConfigURLHold);
//                printf("Config URL Len: %u\n",   (unsigned)strlen(OTAVar.ConfigURLHold));
//                printf("Config Size: %d\n",      OTAVar.ConfigfileSize);
//                printf("JSON: %s\n",             jsonBuffer);
//            }
//        }
//        else
//        {
//            printf("\t\tWr: projectId not verified receivedProjectId %s\r\n",
//                   projectId);
//        }
//    }
	/* ----------- +CMT:  (SMS commands) ----------- */
	if (FromSMS) {
		if (strstr(data, "RESET")) {
			printf("\t\t>>> received Reset CMD <<<\n");
			GSMVari.ResetMCU = 1;   // 0,1,2
		}

		if (GSMVari.MessStarus != 1) {
			if (strstr(data, "Status") != NULL) {
				printf("\t-->Sending the Status SMS\n");
				GSMVari.MessStarus = 1;
			}
		}

		/* parse OTA JSON in-place (no big copy) */
		if (strstr(data, "projectId") && strstr(data, "ConfigURL")
		        && strstr(data, "Configsize") && strstr(data, "url_len")) {

		    printf("\t\t>>> OTA request from the SMS <<<\n");

		    const char *jsonStart = strchr(data, '{');
		    const char *jsonEnd   = strrchr(data, '}');

		    if (jsonStart == NULL || jsonEnd == NULL || jsonEnd < jsonStart) {
		        printf("JSON start or end not found, or invalid format\n");
		        return;
		    }
		    printf("Lg: JSON start or end found\n");

		    size_t jsonLength = (size_t)(jsonEnd - jsonStart + 1);
		    if (jsonLength == 0 || jsonLength > 1024) {
		        printf("Wr: JSON length %u out of allowed range\n", (unsigned)jsonLength);
		        return;
		    }

		    /* --- Temporarily NUL-terminate the JSON in-place --- */
		    /* Note: we cast away const because we must mutate the buffer.
		       Make sure 'data' points to a writable buffer (e.g. UartString). */
		    char *wjsonStart = (char *)jsonStart;
		    char *afterJson  = (char *)(jsonEnd + 1);   // points to char after '}'
		    char saved_char = *afterJson;               // save it
		    *afterJson = '\0';                          // temporarily NUL-terminate

		    /* Now we can call the same json_get_string / json_get_int that expect
		       a NUL-terminated C-string. */
		    printf("Lg: JSON raw: %s\n", wjsonStart);

		    char projectId[64];
		    if (!json_get_string(wjsonStart, "projectId", projectId, sizeof(projectId))) {
		        printf("Er: projectId not found in JSON\n");
		        *afterJson = saved_char; // restore
		        return;
		    }

		    if (strcmp(projectId, MqttprojectId) == 0) {
		        char configURL[256];
		        int  configSize = 0;

		        int hasURL  = json_get_string(wjsonStart, "ConfigURL", configURL, sizeof(configURL));
		        int hasSize = json_get_int   (wjsonStart, "Configsize", &configSize);

		        if (hasURL && hasSize) {
		            printf("Lg: In OTA Config\n");

		            memset(OTAVar.ConfigURLHold, 0, sizeof(OTAVar.ConfigURLHold));
		            strncpy(OTAVar.ConfigURLHold, configURL, sizeof(OTAVar.ConfigURLHold) - 1);

		            OTAVar.ConfigfileSize = configSize;
		            OTAVar.OTARequest = 1;

		            printf("Verified Project ID: %s\n", projectId);
		            printf("Config URL: %s\n", OTAVar.ConfigURLHold);
		            printf("Config URL Len: %u\n", (unsigned)strlen(OTAVar.ConfigURLHold));
		            printf("Config Size: %d\n", OTAVar.ConfigfileSize);
		            printf("JSON: %s\n", wjsonStart);
		        } else {
		            printf("Wr: JSON missing ConfigURL or Configsize\n");
		        }
		    } else {
		        printf("\t\tWr: projectId not verified receivedProjectId %s\r\n", projectId);
		    }

		    /* restore modified character */
		    *afterJson = saved_char;
		}

	}
}

