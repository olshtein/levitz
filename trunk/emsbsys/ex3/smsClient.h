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
int sendToSMSC(Message SmsMessage);
result_t initSmsClient();
result_t ping();
#endif /* SMSCLIENT_H_ */
