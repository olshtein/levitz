/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"
#define MAX_SIZE_OF_MES_STRUCT 182
int sendToSMSC(Message SmsMessage){
	SMS_SUBMIT sms;
	memcpy(&sms.data,&SmsMessage.content,sizeof(SmsMessage.content));
	sms.data_length=SmsMessage.size;
	memcpy(&sms.recipient_id,&SmsMessage.numberFromTo,sizeof(SmsMessage.numberFromTo));
	unsigned char buffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned length=MAX_SIZE_OF_MES_STRUCT;
	embsys_fill_submit((char *)buffer, &sms, &length);
	 network_send_packet_start(buffer, length, length);
	return 0;
}
