/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"
extern TX_QUEUE receiveQueue;
char myIp[]={'7','7','0','7','7','0','7','7'};
#define MAX_SIZE_OF_MES_STRUCT 161
const uint32_t networkreciveSize = 5;
desc_t networkBufferRecive[networkreciveSize];
typedef struct {
	uint8_t dtata[MAX_SIZE_OF_MES_STRUCT];
}my_mess;
my_mess recivedPointers[networkreciveSize];
const uint32_t  networksendSize = 5;
desc_t networkBufferSend[networksendSize];

SMS_DELIVER recivedList[networkreciveSize];
SMS_SUBMIT SendList[networkreciveSize];
volatile bool sendAckRecived;
volatile int data_length;
volatile int deliverListHead;;
/**
*
* @param buffer
* @param size
*/
void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size){

}

/**
* call back when a packet was received.
* @param buffer
* @param size
* @param length
*/
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length){

	SMS_DELIVER deliver;
	EMBSYS_STATUS stat= embsys_parse_deliver((char*)buffer,&deliver);
	if(stat==SUCCESS){
		//		volatile int del=deliverListHead;
		recivedList[deliverListHead]=deliver;
		if(tx_queue_send(&receiveQueue, (void *)&(deliverListHead), TX_NO_WAIT)==TX_SUCCESS){
			deliverListHead=(deliverListHead+1)%networkreciveSize;
		}
		//		addMessage(m);
	}
	else {
		SMS_SUBMIT_ACK subm_ack;
		stat=embsys_parse_submit_ack((char*)buffer,&subm_ack);
		sendAckRecived=true;
		//TODO
	}
	//	data_length=length;
	//	data_length=smsDELIVER.data_length;
}

/**
* call back when a packet was dropped during receiving.
* @param t
*/
void network_packet_dropped_cb1(packet_dropped_reason_t t){
	data_length=t;
}

/**
* call back when a packet was dropped during transmission.
*/
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length ){
	data_length=t;
}


/**
*
* @return
*/
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
/**
*
* @param nothing
*/
void sendLoop(ULONG nothing){
	ULONG sendMessage;
	UINT status;
	while(1){
	sendAckRecived=false;
		status = tx_queue_receive(&queue_0, &sendMessage, TX_WAIT_FOREVER);
		if (status==TX_SUCCESS){
			while ((sendToSMSC(&SendList[sendMessage])!=OPERATION_SUCCESS) && noSendAckRecived){
				tx_thread_sleep(5);
			}
		}
	}
}
/**
*
* @param SmsMessage
* @return
*/
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
/**
*
*/
void receiveLoop(){
	ULONG received_message;
	UINT status;
	char ProbeBuffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned len;
	char isProbAck=0;
	SMS_PROBE probe_ack;
	memcpy(&probe_ack.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	while(1){
		status = tx_queue_receive(&receiveQueue, &received_message, 10);
		if (status == TX_QUEUE_EMPTY){//send ping
			isProbAck=0;
		}
		else if (status==TX_SUCCESS){//send ping ack
			SMS_DELIVER * deliverd=&recivedList[received_message];
			Message mes;
			isProbAck=1;
			memcpy(&probe_ack.sender_id,&recivedList->sender_id,sizeof(char)*ID_MAX_LENGTH);
			memcpy(&mes.numberFromTo,&recivedList->sender_id,sizeof(char)*ID_MAX_LENGTH);
			memcpy(&probe_ack.timestamp,&recivedList->timestamp,sizeof(char)*TIMESTAMP_MAX_LENGTH);
			memcpy(&mes.timeStamp,&recivedList->timestamp,sizeof(char)*ID_MAX_LENGTH);
			memcpy(&mes.content,&recivedList->data,sizeof(char)*recivedList->data_length);
			mes.size=recivedList->data_length;
			mes.inOrOut=IN;
			addNewMessageToMessages(&mes);
		}
		else{
			//			break;
		}

		embsys_fill_probe((char *)ProbeBuffer, &probe_ack, isProbAck,&len);
		result_t res=network_send_packet_start((unsigned char *)ProbeBuffer, MAX_SIZE_OF_MES_STRUCT, len);
		if(res!=SUCCESS){
			//TODO
		}
	}
}
