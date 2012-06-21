/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"
extern TX_QUEUE queue_0;
char myIp[]={'7','7','0','7','7','0','7','7'};
#define MAX_SIZE_OF_MES_STRUCT 161
void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size){

}

/*
 * call back when a packet was received.
 */
volatile int data_length;
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length){

	SMS_SUBMIT_ACK subm_ack;
	SMS_DELIVER deliver;
	EMBSYS_STATUS stat= embsys_parse_deliver((char*)buffer,&deliver);
	if(stat==SUCCESS){
		SMS_PROBE prob_ack;
		memcpy(&prob_ack.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&prob_ack.sender_id,&deliver.sender_id,deliver.data_length*sizeof(char));
		memcpy(&prob_ack.timestamp,&deliver.timestamp,sizeof(char)*TIMESTAMP_MAX_LENGTH);
		unsigned int len;
		embsys_fill_probe((char*)buffer, &prob_ack, 'Y',&len);
		result_t res=network_send_packet_start(buffer, MAX_SIZE_OF_MES_STRUCT, len);
		Message m;
		memcpy(&m.content,&deliver.data,deliver.data_length*sizeof(char));
		m.size=deliver.data_length-1;
		memcpy(&m.numberFromTo,&deliver.sender_id,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&m.timeStamp,&deliver.timestamp,sizeof(char)*ID_MAX_LENGTH); //TODO
		m.inOrOut=IN;
		addMessage(m);
	}
	else {
		stat=embsys_parse_submit_ack((char*)buffer,&subm_ack);
	}
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


const uint32_t networkreciveSize = 10;
desc_t networkBufferRecive[networkreciveSize];
typedef struct {
	uint8_t dtata[MAX_SIZE_OF_MES_STRUCT];
}my_mess;
my_mess recivedPointers[networkreciveSize];
const uint32_t  networksendSize = 10;
desc_t networkBufferSend[networksendSize];

result_t initSmsClient(){
	network_init_params_t myCoolNetworkParms;
	for(int i=0;i<networkreciveSize;i++){
		networkBufferRecive[i].pBuffer=(uint32_t)&recivedPointers[i];
	}
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
	network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
	return result;
}
void sendLoop(ULONG nothing){
	ULONG received_message;
	UINT status;
	while(1){
		status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);
		if (status != TX_SUCCESS)break;
		sendToSMSC((Message *)received_message);
	}
}
const char myMess[2]={'a','b'};
SMS_SUBMIT sms;

result_t sendToSMSC(Message * SmsMessage){
	SMS_SUBMIT sms;
	sms.data_length=SmsMessage->size;
	memcpy(&sms.data,&SmsMessage->content,sms.data_length*sizeof(char));
	memcpy(&sms.recipient_id,&SmsMessage->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
	memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	unsigned char buffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned length=MAX_SIZE_OF_MES_STRUCT;
	embsys_fill_submit((char *)buffer, &sms, &length);

	result_t res=network_send_packet_start(buffer, MAX_SIZE_OF_MES_STRUCT, length);

	return res;
}

SMS_PROBE probe;
void ping(ULONG a){

	char ProbeBuffer[MAX_SIZE_OF_MES_STRUCT];
	memcpy(&probe.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
	unsigned len=MAX_SIZE_OF_MES_STRUCT;

	embsys_fill_probe((char *)ProbeBuffer, &probe, 0,&len);

	result_t res=network_send_packet_start((unsigned char *)ProbeBuffer, MAX_SIZE_OF_MES_STRUCT, len);
	//	return res;
}

void receiveLoop(){
	ULONG received_message;
	UINT status;
	while(1){
	status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);
	if (status != TX_SUCCESS)break;
	addNewMessageToMessages(received_message);

	}
}
