/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"
char myIp[]={'7','7','0','7','7','0','7','7'};
#define MAX_SIZE_OF_MES_STRUCT 160
void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size){

}

/*
 * call back when a packet was received.
 */
int data_length;
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length){
	data_length=length;
//	data_length=smsDELIVER.data_length;
}

/*
 * call back when a packet was dropped during receiving.
 */
void network_packet_dropped_cb1(packet_dropped_reason_t t){
	data_length=t;
}

/*
 * call back when a packet was dropped during transmission.
 */
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length ){
	data_length=t;
}
const uint32_t networkreciveSize = 100;
desc_t networkBufferRecive[networkreciveSize];
const uint32_t  networksendSize = 100;
desc_t networkBufferSend[networksendSize];

result_t initSmsClient(){
	network_init_params_t myCoolNetworkParms;
	result_t result ;
	//	networkParms.list_call_backs = NULL;
	myCoolNetworkParms.recieve_buffer=(desc_t*)networkBufferRecive;
	myCoolNetworkParms.size_r_buffer=(uint32_t)networkreciveSize;
	myCoolNetworkParms.size_t_buffer=(uint32_t)networksendSize;
	myCoolNetworkParms.transmit_buffer=(desc_t*)networkBufferSend;
	myCoolNetworkParms.list_call_backs.dropped_cb=network_packet_dropped_cb1;
	myCoolNetworkParms.list_call_backs.recieved_cb=network_packet_received_cb1;
	myCoolNetworkParms.list_call_backs.transmit_error_cb=network_transmit_error_cb1;
	myCoolNetworkParms.list_call_backs.transmitted_cb=network_packet_transmitted_cb1;
	network_init_params_t * myCoolNetworkParmsPointer=&myCoolNetworkParms;
	result=network_init(myCoolNetworkParmsPointer);

	return result;
}
result_t sendToSMSC(Message SmsMessage){
	SMS_SUBMIT sms;
	memcpy(&sms.data,&SmsMessage.content,sizeof(SmsMessage.content));
	sms.data_length=SmsMessage.size;
	memcpy(&sms.recipient_id,&SmsMessage.numberFromTo,sizeof(SmsMessage.numberFromTo));
	memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	unsigned char buffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned length=MAX_SIZE_OF_MES_STRUCT;
	embsys_fill_submit((char *)buffer, &sms, &length);

	result_t res=network_send_packet_start(buffer, MAX_SIZE_OF_MES_STRUCT, length);

	return res;
	return 0;
}

result_t ping(){
	SMS_PROBE probe;
	network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
	char ProbeBuffer[MAX_SIZE_OF_MES_STRUCT];
//	memcpy(&probe.sender_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
	memcpy(&probe.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
//	memcpy(&probe.device_id,"0\0",2);
	unsigned len=MAX_SIZE_OF_MES_STRUCT;

	embsys_fill_probe((char *)ProbeBuffer, &probe, 0,&len);

	result_t res=network_send_packet_start((unsigned char *)ProbeBuffer, MAX_SIZE_OF_MES_STRUCT, len);
	return res;
}
