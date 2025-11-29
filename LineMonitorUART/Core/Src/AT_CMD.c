/*
 * AT_CMD.c
 *
 *  Created on: Aug 20, 2024
 *      Author: Shashank
 */
#include "AT_CMD.h"
#include "Parameters.h"

const uint8_t AT_ENDebug = 1;
//Handler
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart3;
// Structure


uint8_t rx_buffer[UART_BUFFER_SIZE]; //RX buffer Array
volatile uint16_t rx_insert_idx = 0;
volatile uint16_t rx_extract_idx = 0;
char UartString[2300];
uint8_t UART_Ava=0; // Indicate that we received the \r\n in the UART
//
// const uint8_t certificate[];

//function declarations
static void convertTimeString(const char* input, char* output);
static const char* convertNetworkType(const char* networkType);
static const char* getOperatorName(const char* operatorCode);
static uint8_t check_sim_status();
static char* generateRandomID() ;
/**
  * @brief CallBack function of UART
  * @param pointer UART_HandleTypeDef
  * @retval non
  */
void AT_UART_RxCpltCallback()
{
    rx_insert_idx++;
    if (rx_insert_idx >= UART_BUFFER_SIZE){
        rx_insert_idx = 0; // Wrap buffer index if necessary
    }
    HAL_UART_Receive_IT(&huart3, (uint8_t*)&rx_buffer[rx_insert_idx], 1);
}
/**
  * @brief Give the UART buffer status
  * @param non
  * @retval Status of the UART buffer
  */
uint8_t UART_ReadAvailable(void)
{
    return rx_insert_idx != rx_extract_idx;
}
/**
  * @brief Give the uart buffer value
  * @param non
  * @retval UART buffer byte
  */
uint8_t UART_Read(void)
{
    if (rx_insert_idx == rx_extract_idx)
        return -1; // No data available

    uint8_t data = rx_buffer[rx_extract_idx++];
    if (rx_extract_idx >= UART_BUFFER_SIZE)
        rx_extract_idx = 0; // Wrap buffer index if necessary
    return data;
}

/**
  * @brief handle The AT commands
  * @param AT,Its Response, ResponseTimeout, number of tryes
  * @retval Status of the AT CMD
  */
uint8_t atSend(const char* AT, const char* respons, uint16_t responseTime, uint8_t tryFor,uint8_t AT_ENDebug)
{
    uint32_t startTime = 0; // Use uint32_t for HAL_GetTick() return type
    memset(UartString, 0, sizeof(UartString));

    if(AT_ENDebug)printf("AT CMD: %s\n", AT);


    for (uint8_t id = 1; id <= tryFor; id++)
    {
    	if(AT_ENDebug)printf("Try = %u, Length = %u\n", id, strlen(AT));
        // Correctly transmit the passed AT command instead of a fixed string
    	HAL_StatusTypeDef status = HAL_ERROR;

        if(strcmp(AT, "0x01A") == 0)
        {
            uint8_t ctrlZ = 0x1A;
            status = HAL_UART_Transmit(&huart3, &ctrlZ, 1, 1000);
        }
    	else
    	{
    		status = HAL_UART_Transmit(&huart3, (uint8_t*)AT, strlen(AT), 1000);
    	}

        if (status != HAL_OK) {
            printf("Error: UART transmit failed with status %d\n", status);
            return 0;
        }

        HAL_Delay(100);
        startTime = HAL_GetTick();

        while ((HAL_GetTick() - startTime) < responseTime) {
            if (UART_ReadAvailable())
            {
                uint32_t bytesRead = 0;
                while (UART_ReadAvailable() && bytesRead <= sizeof(UartString) - 1)
                {
                    UartString[bytesRead] = UART_Read();
                    bytesRead++;
                    HAL_Delay(1);
                    HAL_IWDG_Refresh(&hiwdg);

                }
                UartString[bytesRead] = '\0'; // Ensure null-terminated string

                if (strstr(UartString, respons) != NULL) {
                	if(AT_ENDebug)printf("%s \n\tResponse verified. \n",UartString);

                    return 1;
                }
                else
                {
                	if(AT_ENDebug){printf("Received %s\n", UartString);}
                	else
                	{
                		printf("AT CMD not verified: %s ", AT);
                		printf("Try = %u, Length = %u\n", id, strlen(AT));
                		printf("%s\n\tReceived\n", UartString);

                	}

                }

            }
            HAL_Delay(2);  // Sleep for 1 millisecond to allow data to accumulate
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    printf("\n\tEr:%s Failed to get response\n",AT);
    return 0;
}

/**
  * @brief receive the uart data
  * @param non
  * @retval none
  */
void init_Uart_RX()
{
	HAL_UART_Receive_IT(&huart3, (uint8_t*)&rx_buffer[rx_insert_idx], 1);
}

/**
  * @brief initialize GSM
  * @param non
  * @retval none
  */
void GSMIniT()
{
    printf("\t\t>>Lg: GSM Init <<\n");
    const uint16_t responseTime = 1000;
    const uint8_t EnableDebug = 1;
    const uint8_t tryFor = 3, tryfor2 = 1;

//    atSend("AT&F","OK" , 1000, 1, EnableDebug);
    atSend("AT+QPOWD=1\r\n","+QPOWD" , 1000, 1, EnableDebug);

    printf("GSM power Off\n");
    HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, GPIO_PIN_RESET);
    HAL_Delay(2000);
    HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, GPIO_PIN_SET);

    HAL_Delay(500);
    printf("GSM_PWPkey pin High\n");
    HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(2500);
    HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_RESET);

    printf("GSM Powered up\n");

    // Basic AT command check
    atSend("AT\r\n", "OK", 2000, tryFor, EnableDebug);

    // Disable echo
    if (!atSend("ATE0\r\n", "OK", responseTime, tryfor2, EnableDebug)) {
        atSend("ATE0\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    // Check if UART string is empty and retry initialization if necessary
    if (!strlen(UartString)) {
        for(uint8_t i = 0 ; i < 3; i++) {
            atSend("AT+QPOWD=1\r\n","+QPOWD" , 1000, 1, EnableDebug);
            printf("GSM power Off\n");
            HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, GPIO_PIN_RESET);
            HAL_Delay(2000);
            HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, GPIO_PIN_SET);

            HAL_Delay(500);
            printf("GSM_PWPkey pin High\n");
            HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_SET);
            HAL_Delay(2500);
            HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_RESET);

            // Basic AT command check
            atSend("AT\r\n", "OK", 1000, 4, EnableDebug);

            if (strlen(UartString)) {
                break;
            }
        }

        // Disable echo
        if (!atSend("ATE0\r\n", "OK", responseTime, tryfor2,EnableDebug)) {
            atSend("ATE0\r\n", "OK", responseTime, tryFor, EnableDebug);
        }
    }

    // Check SIM card status
    if (!atSend("AT+CPIN?\r\n", "+CPIN: READY", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CPIN?\r\n", "+CPIN: READY", responseTime, tryFor, EnableDebug);
    }

    // Set full functionality mode
    if (!atSend("AT+CFUN?\r\n", "+CFUN: 1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CFUN=1\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    // Check network registration
    if (!atSend("AT+CREG?\r\n", "+CREG: 1,1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CREG=1\r\n", "+CREG:", responseTime, tryFor, EnableDebug);
    }

    // Check GPRS attachment
    if (!atSend("AT+CGATT?\r\n", "+CGATT: 1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CGATT=1\r\n", "OK", responseTime, tryFor, EnableDebug);
    }
//    atSend("AT+CMGD=0,4\r\n", "OK", responseTime, tryfor2, EnableDebug);

//
//    // Set or verify a generic or default APN
    if (!atSend("AT+CGDCONT?\r\n", "+CGDCONT: 1,\"IP\",\"internet\"", responseTime, tryfor2, 0)) {
        atSend("AT+CGDCONT=1,\"IP\",\"internet\"\r\n", "OK", responseTime, tryFor, 0);
    }


    // Check if PDP context is defined correctly
//    if (!atSend("AT+CGDCONT?\r\n", "+CGDCONT: 1,\"IP\",\"airtelgprs.com\"", responseTime, tryfor2,EnableDebug)) {
//        atSend("AT+CGDCONT=1,\"IP\",\"airtelgprs.com\"\r\n", "OK", responseTime, tryFor,EnableDebug);
//    }
    atSend("AT+CGDCONT=1,\"IP\",\"internet\"\r\n", "OK", responseTime, tryFor,EnableDebug);

    // Activate PDP context
    if (!atSend("AT+CGACT?\r\n", "+CGACT: 1,1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CGACT=1,1\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    if (!atSend("AT+QIACT?\r\n", "+QIACT: 1,1,1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+QIACT=1\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    // Set SMS to text mode
    if (!atSend("AT+CMGF?\r\n", "+CMGF: 1", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CMGF=1\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    // Configure new message indications
    if (!atSend("AT+CNMI?\r\n", "+CNMI: 1,0,0,0,0", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CNMI=1,0,0,0,0\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    if (!atSend("AT+CSMP?\r\n", "+CSMP: 17,167,0,0", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+CSMP=17,255,0,0\r\n", "OK", responseTime, tryFor, EnableDebug);
    }
//    atSend("AT+CMGD=1,4", "OK", responseTime, tryFor, EnableDebug);
    // Set preferred message storage to SIM memory
    atSend("AT+CPMS?\r\n", "OK", responseTime, tryFor, EnableDebug);

    // Check for available networks (Optional, to check for available operators)
    if (!atSend("AT+COPS?\r\n", "OK", responseTime, tryfor2, EnableDebug)) {
        atSend("AT+COPS=?\r\n", "OK", responseTime, tryFor, EnableDebug);
    }

    printf("\t\t>>Lg: GSM Done Init <<\n");
}

/**
  * @brief connecting Mqtt
  * @param non
  * @retval Status of the connection
  */
uint8_t GSMConnectMqtt() {
    // Create a character array to store the AT command
    const uint16_t responseTime = 6000;
    const uint8_t tryFor = 5;
    char atCommand[228] = "";
    char data[150] = "";
    printf("Lg: Connecting Mqtt");

    atSend("AT+QMTCLOSE=1\r\n", "OK", 1000, 1, 1); // Closing the Connection
    atSend("AT+QMTDISC=1\r\n", "OK", 1000, 1, 1);

//    atSend("AT+QFLDS=\"UFS\"\r\n", "OK", 5000, 3, 1);
//    atSend("AT+QFLST=\"*\"\r\n", "OK", 5000, 3, 1);//
//    atSend("AT+QFDEL=\"UFS:cacert.pem\"\r\n", "OK", 10000, 3, 1);
//    sprintf(atCommand,"AT+QFUPL=\"UFS:cacert.pem\",%u\r\n",sizeof(certificate));
//    if(atSend(atCommand, "CONNECT", 10000, 3, 1))
//    {
//    	atSend((char*)certificate, "+QFUPL:", 1000, 1, 1);
//
//    }
    atSend("AT+QSSLCFG=\"cacert\",1,\"UFS:cacert.pem\"\r\n", "OK", 5000, 3, 1);
    atSend("AT+QSSLCFG=\"seclevel\",1,1\r\n", "OK", 5000, 3, 1);
    atSend("AT+QSSLCFG=\"sslversion\",1,3\r\n", "OK", 1000, 1, 1);
    atSend("AT+QSSLCFG=\"ciphersuite\",1,0X0035\r\n", "OK", 1000, 1, 1);
    atSend("AT+QSSLCFG=\"sni\",1,1\r\n", "OK", 1000, 1, 1);



//    atSend("AT+QSSLCFG=\"clientcert\",1,\"UFS:client.pem\"\r\n", "OK", 5000, 3, 1);
//    atSend("AT+QSSLCFG=\"clientkey\",1,\"UFS:user_key.pem\"\r\n", "OK", 5000, 3, 1);



    atSend("AT+QMTCFG=\"session\",1,1\r\n", "OK", 1000, 1, 1); // Configuration
    atSend("AT+QMTCFG=\"keepalive\",1,60\r\n", "OK", 1000, 1, 1);
    atSend("AT+QMTCFG=\"version\",1,4\r\n", "OK", 1000, 1, 1);
    atSend("AT+QMTCFG=\"ssl\",1,1,1\r\n", "OK", 1000, 1, 1);


    sprintf(data, "{\"device_id\": \"%s\",\"Connection Status\": 0}", DefaultMqttdeviceId);
    sprintf(atCommand, "AT+QMTCFG=\"willex\",1,1,1,0,\"%s/Willmessage/%s\",%u\r\n", Default_MqttUserName, DefaultMqttdeviceId, strlen(data));

    if (atSend(atCommand, ">", 1000, 1, 1)) {
        HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), 1000);
    }
    atSend("AT\r\n", "OK", 1000, 1, 1);
    sprintf(atCommand, "AT+QMTOPEN=1,\"%s\",%s\r\n", Default_MqttURL, Default_MqttPort);

    if (atSend(atCommand, "+QMTOPEN: 1,0", responseTime, tryFor, 1)) { // Connecting
        char *randomID = generateRandomID(); // Ensure this function is defined

        snprintf(atCommand, sizeof(atCommand), "AT+QMTCONN=1,\"ID%s%s\",\"%s\",\"%s\"\r\n", DefaultMqttdeviceId, randomID, Default_MqttUserName, Default_MqttPWR);
        if (atSend(atCommand, "+QMTCONN: 1,0,0", responseTime, 2, 1)) { // Include the missing AT_ENDebug argument
            printf("\n\n\t\t>>>>>>>Lg: Connected To MQTT<<<<<<<<<<\n\n");

            sprintf(data, "{\"device_id\": \"%s\",\"Connection Status\": 1}", DefaultMqttdeviceId);
            sprintf(atCommand, "AT+QMTPUBEX=1,1,1,0,\"%s/Willmessage/%s\",%u\r\n", Default_MqttUserName, DefaultMqttdeviceId, strlen(data));
            if (atSend(atCommand, ">", 1000, 1, 1)) {
//                HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), 1000);
                atSend(data, "+QMTPUBEX: 1,1,0", 1000, 1, 1);
            }

            sprintf(atCommand, "AT+QMTSUB=1,1,\"%s/Sub/%s\",1\r\n", Default_MqttUserName, DefaultMqttdeviceId);
            if (atSend(atCommand, "+QMTSUB: 1,1,0", 1000, 6, 1)) {
                printf("Done Subscribe to Mqtt broker\n");
            } else {
                printf("\t\t-->Failed Subscribe to Mqtt broker\n");
            }

            return 1;
        }
    }

    printf("\t\t>>>>>>>>Wr:Mqtt Disconnected<<<<<<<<<<\n");
    return 0;
}

/**
  * @brief Publish to Mqtt broker
  * @param  data pointer
  * @retval function status
  */
uint8_t GSMpublishMqtt(const char *jsonBuffer) {
    uint8_t MqttConnect = 1;
    if (!atSend("AT+QMTCONN?\r\n", "+QMTCONN: 1,3", 1000, 1, 1)) { // Include the missing AT_ENDebug argument
    	MqttConnect = 0;
        for (uint8_t id = 0; id < 3; id++) {
            if (GSMConnectMqtt()) {
                MqttConnect = 1;
                break;
            }
        }
    }

    if (MqttConnect) {
        char atCommand[100] = "";
        uint16_t LengthData = strlen(jsonBuffer);
        snprintf(atCommand, sizeof(atCommand), "AT+QMTPUB=1,1,1,0,\"%s/Data/%s\"\r\n", Default_MqttUserName, DefaultMqttdeviceId);

        for (uint8_t id = 0; id < 3; id++) {
            if (atSend(atCommand, ">", 1000, 2, 1)) { // Include the missing AT_ENDebug argument
                HAL_UART_Transmit(&huart3, (uint8_t *)jsonBuffer, LengthData, 1000);

                if (atSend("0x01A", "+QMTPUB: 1,1,0", 1000, 1, 1)) { // Include the missing AT_ENDebug argument
                    printf("\t\tLg:Done Pub\n");
                    return 1;
                } else {
                    printf("Er: Status Pub Failed\n");
                }
            } else {
                printf("Er: AT+QMTPUBEX Failed_1\n");
            }
        }
    }

    return 0;
}

static char* generateRandomID() {
  static char randomID[20];  // Adjust the size based on your needs

  // Seed the random number generator with the current time
//  srand((unsigned int)time(NULL));

  // Generate a random number and convert it to a string
  uint16_t randomValue = rand();
  snprintf(randomID, sizeof(randomID), "%05d", randomValue);
  return randomID;
}
/**
 * @brief processUnreadMessages
 * @param  masterPhoneNumber: The master phone number to filter messages from.
 * @retval None
 */
void GSMprocessUnreadMessages(const char *masterPhoneNumber)
{
// Init Variables
#define CMD_BUFFER_SIZE 64
#define SMS_BUFFER_SIZE 2500
#define UID_BUFFER_SIZE 10

    uint8_t messageIndex = 0; // Message index starts from 1
    uint8_t totalConcatenation = 0;
    char commandBuffer[CMD_BUFFER_SIZE] = {0};    // Buffer to store AT command
    char SMSConcatenation[SMS_BUFFER_SIZE] = {0}; // Buffer to store concatenated SMS
    char lastUID[UID_BUFFER_SIZE] = {0};          // To store last message's UID

    // Send AT command to list unread messages
    if (!atSend("AT+CMGL=\"ALL\"\r\n", "+CMGL:", 5000, 1, 1))
    {
        return;
    }
    printf("\t\tFound unread messages.\n");

    HAL_Delay(1000);

    while (1)
    {
        HAL_IWDG_Refresh(&hiwdg);
        atSend("AT+CMGL=\"ALL\"\r\n", "+CMGL:", 5000, 1, 1);
        // Read the response buffer to get each message using AT+QCMGR
        snprintf(commandBuffer, CMD_BUFFER_SIZE, "AT+QCMGR=%d\r\n", messageIndex);
        if (!atSend(commandBuffer, "+QCMGR:", 5000, 3, 1))
        {
            printf("No more unread messages.\n");
            if (messageIndex)
            {
                messageIndex = 0;
                printf("\t\tDeleting all and requesting SMS resend.\n");
                atSend("AT+CMGD=1,4\r\n", "OK", 1000, 2, 1);
            }
            break;
        }
        //+CME ERROR: 321
        // Starting the Read process
        char *msgDetails = strstr(UartString, "+QCMGR:");
        if (msgDetails)
        {
            uint16_t msg_seg = 0, msg_total = 0; // To track message segment number and total segments
            char uid[UID_BUFFER_SIZE] = {0};     // Buffer to store UID

            // Extract message UID, msg_seg, and msg_total
            if (sscanf(msgDetails, "+QCMGR: \"REC READ\",\"%*[^\"]\",,\"%*[^\"]\",%9[^,],%hu,%hu", uid, &msg_seg, &msg_total) == 3)
            {
                printf("-->Uid %s, msg_seg %d, msg_total %d\n", uid, msg_seg, msg_total);

                // Check if this is a new message or continuation
                if (strcmp(lastUID, uid) != 0)
                {
                    printf("New UID detected\n");
                    // New message, reset the concatenation buffer
                    memset(SMSConcatenation, 0, sizeof(SMSConcatenation));
                    strncpy(lastUID, uid, UID_BUFFER_SIZE - 1);
                    totalConcatenation = 0; // Reset concatenation counter
                }

                // Iterate over each segment of the concatenated message
                for (uint8_t id = messageIndex; id < messageIndex + msg_total; id++)
                {
                    // Read the response buffer to get each message using AT+QCMGR
                    snprintf(commandBuffer, CMD_BUFFER_SIZE, "AT+QCMGR=%d\r\n", id);
                    if (!atSend(commandBuffer, "+QCMGR:", 5000, 3, 0))
                    {
                        printf("Failed to read message segment %d.\n", id);
                        break;
                    }

                    char *msgDetails_1 = strstr(UartString, "+QCMGR:");
                    if (msgDetails_1)
                    {
                        totalConcatenation++;
                        printf("\t\tidx%u-[%u/%u]\n", id, totalConcatenation, msg_total);

                        // Extract message content
                        char *messageStart = strstr(msgDetails_1, "\n");
                        if (messageStart)
                        {
                            char *end = strstr(messageStart, "\n\r\nOK");
                            if (end != NULL)
                            {
                                *end = '\0'; // Null-terminate to remove unwanted part
                            }

                            // Safely concatenate the message content
                            strncat(SMSConcatenation, messageStart + 1, SMS_BUFFER_SIZE - strlen(SMSConcatenation) - 1);
                        }
                    }
                    else
                    {
                        printf("Failed to parse message content for segment %d.\n", id);
                        break;
                    }
                }

                // Check if all parts have been concatenated
                if (totalConcatenation == msg_total)
                {
                    printf("\nComplete message received:\n%s\n", SMSConcatenation);

                    // Update messageIndex for the next message set
                    messageIndex += totalConcatenation;
                }
                else
                {
                    printf("\t\t--> Not able to complete msg_seg [%u/%u], deleting all and requesting SMS resend.\n", totalConcatenation, msg_total);
                    break;
                }
            }
            else
            {
                printf("Failed to parse UID or message details.\n");

                messageIndex++;
            }
        }
        else
        {
            printf("Failed to find +QCMGR: in response, deleting all and requesting SMS resend.\n");

            break;
        }
    }
    if (messageIndex)
    {
        messageIndex = 0;
        printf("\t\tDeleting all and requesting SMS resend.\n");
        atSend("AT+CMGD=1,4\r\n", "OK", 1000, 2, 1);
    }
}

/**
  * @brief initialize GSM
  * @param non
  * @retval none
  */
void Get_Timestamp(char* Time)
{
	sprintf(Time,"00:00 00-00-00");
    if (atSend("AT+QLTS=2 \r\n", "+QLTS: ", 300,5,0))
    {
		convertTimeString(UartString,Time);
    }
    printf("Time Stamp %s\n",Time);
}

static void convertTimeString(const char* input, char* output)
{
    // Find the start of the date and time string
	char year[5], month[3], day[3], hour[3], minute[3], second[3], timezone[4];
	if (strstr(input, "+QLTS: \"\"") != NULL) {
		return;
	}

    const char *start = strchr(input, '\"') + 1;
    sscanf(start, "%4s/%2s/%2s,%2s:%2s:%2s+%2s", year, month, day, hour, minute, second, timezone);
    sprintf(output,"%s:%s %s-%s-%s",hour,minute,day,month,&year[2]);
}


// Function to retrieve cellular information
uint8_t getCellularInfo(CellularInfo* info) {
    int csqValue = 0;
    uint8_t status = 0;
    char *startPtr, *endPtr;

    // Initialize the CellularInfo structure to ensure no garbage data
    memset(info, 0, sizeof(CellularInfo));
//    strncpy(info->networkType," ",sizeof(info->networkType));
//    strncpy(info->operatorName,"*",sizeof(info->operatorName));
    if(!check_sim_status())
    {
    	return 0;
    }

    // Get Signal Strength
    status = atSend("AT+CSQ\r\n", "+CSQ:", 1500, 10, 1);
    if (status) {
        printf("Received String: %s\n", UartString);

        startPtr = strstr(UartString, "+CSQ: ");
        if (startPtr) {
            startPtr += strlen("+CSQ: ");
            csqValue = atoi(startPtr);
            printf("Parsed csqValue = %d\n", csqValue);

            if (csqValue >= 0 && csqValue <= 31) {
                info->signalStrength = (csqValue * 100) / 31;
            } else {
                info->signalStrength = 0; // Invalid CSQ value
            }
        } else {
            printf("Failed to parse CSQ value\n");
            info->signalStrength = 0;
        }
    } else {
        printf("Failed to retrieve signal strength\n");
    }

    // Get Network Type and Operator Name
    status = atSend("AT+QNWINFO\r\n", "+QNWINFO:", 5000, 3, 1);
    if (status) {
        printf("Received String: %s\n", UartString);

        startPtr = strstr(UartString, "+QNWINFO: \"");
        if (startPtr) {
            startPtr += strlen("+QNWINFO: \"");
            endPtr = strchr(startPtr, '\"');
            if (endPtr) {
                strncpy(info->networkType, startPtr, endPtr - startPtr);
                info->networkType[endPtr - startPtr] = '\0';

                // Convert network type to human-readable form
                const char* networkGeneration = convertNetworkType(info->networkType);
                strncpy(info->networkType, networkGeneration, sizeof(info->networkType) - 1);

                // Find operator code and convert it to operator name
                startPtr = strchr(endPtr + 1, '\"');
                if (startPtr) {
                    startPtr += 1;
                    endPtr = strchr(startPtr, '\"');
                    if (endPtr) {
                        strncpy(info->operatorName, startPtr, endPtr - startPtr);
                        info->operatorName[endPtr - startPtr] = '\0';

                        // Convert to human-readable operator name
                        const char* operatorHumanReadable = getOperatorName(info->operatorName);
                        strncpy(info->operatorName, operatorHumanReadable, sizeof(info->operatorName) - 1);
                    }
                }
            }
        } else {
            printf("Failed to parse network type and operator name\n");
        }
    } else {
        printf("Failed to retrieve network type and operator name\n");
    }

    // Check Mobile Data Connectivity
//    if (!checkMobileDataConnectivity(info)) {
//        printf("Failed to check mobile data connectivity.\n");
//    }

    return 1;
}// Function to convert network type to 2G/3G/4G/5G

static const char* convertNetworkType(const char* networkType) {
    if (strcmp(networkType, "TDD LTE") == 0 || strcmp(networkType, "FDD LTE") == 0) {
        return "4G";
    } else if (strcmp(networkType, "WCDMA") == 0 || strcmp(networkType, "HSPA") == 0) {
        return "3G";
    } else if (strcmp(networkType, "GSM") == 0 || strcmp(networkType, "GPRS") == 0 || strcmp(networkType, "EDGE") == 0) {
        return "2G";
    } else if (strcmp(networkType, "NR") == 0) {
        return "5G";
    } else {
        return "";
    }
}
// Function to map MNC/MCC to operator name
static const char* getOperatorName(const char* operatorCode) {
    // Example lookup, should be extended with real mappings
    if (strcmp(operatorCode, "40449") == 0) {
        return "Air";
    } else if (strcmp(operatorCode, "40410") == 0) {
        return "Vod";
    } else if (strcmp(operatorCode, "40486") == 0) {
        return "Jio";
    }
    // Add more mappings as needed
    return "";
}


static uint8_t check_sim_status() {
    // Command to check SIM status
    const char* AT_CMD = "AT+CPIN?\r";
    // Expected response if SIM is ready
    const char* EXPECTED_RESPONSE_READY = "+CPIN: READY";


    const uint16_t responseTime = 2000; // Response time in milliseconds
    const uint8_t tryCount = 1;        // Number of attempts
    const uint8_t debugEnabled = 0;    // Enable debug messages


    // Check result
    if (atSend(AT_CMD, EXPECTED_RESPONSE_READY, responseTime, tryCount, debugEnabled)) {
        printf("SIM card is inserted and ready.\n");
        return 1;
    } else {
    	 printf("\t\tWr:SIM card is not inserted.\n");
    	return 0;
    }
}

