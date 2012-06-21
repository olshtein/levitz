/*
 * smsClient.h
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#ifndef SMSCLIENT_H_
#define SMSCLIENT_H_

#include "embsys_sms_protocol.h"
#include "messages.h"
#include "network.h"
#include "string.h"
#include "tx_api.h"
int sendToSMSC(Message * SmsMessage);
void sendLoop(ULONG nothing);
result_t initSmsClient();
void receiveLoop();
void ping(ULONG a);
#endif /* SMSCLIENT_H_ */
