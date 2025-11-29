/*
 * Credentials.h
 *
 *  Created on: Nov 18, 2025
 *      Author: fervi
 */

#ifndef INC_CREDENTIALS_H_
#define INC_CREDENTIALS_H_


// Define your MQTT credentials
//Mqtt

#define MqttPort 1883

#define MqttBuffSize 1500
// #define MqttBroker "148.113.9.237"

#define MqttBroker "mqtt.fervidlabs.in"
// #define MqttBroker "broker.emqx.io"

#define MqttuserName "fervid"

#define MqttPassword "Fervid@123"

#define MqttclientIDNo "SNMP"

#define MqttPubTopic "sensor/input/IBRAH544OPSR"

#define MqttprojectId "IBRAH544OPSR"
#define MqttdeviceId "FSS_SNMP_GW01"

#define MqttpayloadSize 2000  //this is limeted in the mqtt



//GSM Reset Time
#define GSM_RESET_INTERVAL_MIN 1440UL  // 24 hours is 1440 minutes

//Master control Phone Number
#define MasPhoneNo "7893738368"


#define ResetAfter 86400000UL  // MCU reset for every msec
#define GraceTimeLimit 10000UL // Grace period before reset

#endif /* INC_CREDENTIALS_H_ */
