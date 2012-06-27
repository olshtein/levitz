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
#define BUFF_SIZE (5)
#define RECIVED_LIST_SIZE (SEND_LIST_SIZE)
desc_t transmit_buffer[BUFF_SIZE];
desc_t recieve_buffer[BUFF_SIZE];
uint8_t recevedMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
uint8_t tranMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
volatile int toSendListHead=0;
SMS_SUBMIT toSendList[SEND_LIST_SIZE];
volatile int recivedListHead=0;
SMS_DELIVER recivedList[RECIVED_LIST_SIZE];
volatile bool sendAckRecived;
volatile int data_length;
volatile SMS_SUBMIT * messageThatWasSending= NULL;
extern TX_QUEUE ToSendQueue;

/**
*
* @return
*/
result_t initSmsClient(){
	network_init_params_t myCoolNetworkParms;
	result_t result ;
	//	networkParms.list_call_backs = NULL;

	for(int i=0;i<BUFF_SIZE ; i++){
		recieve_buffer[i].pBuffer = (uint32_t)recevedMsg[i];
		recieve_buffer[i].buff_size = (uint8_t)NETWORK_MAXIMUM_TRANSMISSION_UNIT;
		recieve_buffer[i].reserved =(uint16_t)0;

		transmit_buffer[i].pBuffer = (uint32_t)tranMsg[i];
		transmit_buffer[i].buff_size = (uint8_t)NETWORK_MAXIMUM_TRANSMISSION_UNIT;
		transmit_buffer[i].reserved =(uint16_t)0;
	}
	myCoolNetworkParms.recieve_buffer=(desc_t*)recieve_buffer;
	myCoolNetworkParms.size_r_buffer=(uint32_t)BUFF_SIZE;
	myCoolNetworkParms.size_t_buffer=(uint32_t)BUFF_SIZE;
	myCoolNetworkParms.transmit_buffer=(desc_t*)transmit_buffer;
	myCoolNetworkParms.list_call_backs.dropped_cb=network_packet_dropped_cb1;
	myCoolNetworkParms.list_call_backs.recieved_cb=network_packet_received_cb1;
	myCoolNetworkParms.list_call_backs.transmit_error_cb=network_transmit_error_cb1;
	myCoolNetworkParms.list_call_backs.transmitted_cb=network_packet_transmitted_cb1;
	network_init_params_t * myCoolNetworkParmsPointer=&myCoolNetworkParms;
	result=network_init(myCoolNetworkParmsPointer);
	network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
	toSendListHead=0;
	recivedListHead=0;
	data_length=0;
	messageThatWasSending= NULL;
	return result;
}
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
		///////////////////////////////////////////////////////////////////////////////////
		Message mes;
		SMS_PROBE ack;
		unsigned int len12;

		memcpy(&ack.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&ack.sender_id,&deliver.sender_id,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&ack.timestamp,&deliver.timestamp,sizeof(char)*TIMESTAMP_MAX_LENGTH);
		char probeBuffer12[MAX_SIZE_OF_MES_STRUCT];
		EMBSYS_STATUS  res1=embsys_fill_probe(probeBuffer12, &ack, 'Y' ,&len12);
		result_t res2=network_send_packet_start((unsigned char *)probeBuffer12, MAX_SIZE_OF_MES_STRUCT, len12);
		if(res1!=SUCCESS||res2!=SUCCESS){
			//TODO
			data_length=res1;
//			break;
		}
		memcpy(&mes.numberFromTo,&deliver.sender_id,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&mes.timeStamp,&deliver.timestamp,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&mes.content,&deliver.data,sizeof(char)*deliver.data_length);
		mes.size=deliver.data_length;
		mes.inOrOut=IN;
		addNewMessageToMessages(&mes);
		/////////////////////////////////////////////////////////////////////////////

		//
		//		recivedList[recivedListHead]=deliver;
		//		if(tx_queue_send(&receiveQueue, (void *)&(recivedListHead), TX_NO_WAIT)==TX_SUCCESS){
		//			recivedListHead=(recivedListHead+1)%RECIVED_LIST_SIZE;
		//		}
	}
	else {
		SMS_SUBMIT_ACK subm_ack;
		stat=embsys_parse_submit_ack((char*)buffer,&subm_ack);
		if(stat==SUCCESS){
			if(messageThatWasSending->msg_reference==subm_ack.msg_reference &&
					memcmp((void*)&(messageThatWasSending->recipient_id),&subm_ack.recipient_id,sizeof(char)*ID_MAX_LENGTH)){
				messageThatWasSending=NULL;
			}
		}
		else {
			//TODO
			data_length=stat;
//			break;

		}
	}
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
* @param mes
* @return SUCCESS if the message can be added to the tosend queue
*/
EMBSYS_STATUS sendMessage(Message *mes){
	int imputthis=toSendListHead;
	if(tx_queue_send(&ToSendQueue, (void *)&(imputthis), TX_NO_WAIT)==TX_SUCCESS){
		//TODO  problem - with other thread!
		toSendListHead=(toSendListHead+1)%SEND_LIST_SIZE;
		SMS_SUBMIT* toSend= &toSendList[imputthis];
		memcpy(&toSend->data,&mes->content,mes->size*sizeof(char));
		toSend->data_length=mes->size;
		memcpy(&toSend->device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&toSend->recipient_id,&mes->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
		return SUCCESS;
	}
	else return FAIL;
}
/**
*
* @param nothing
*/
void sendLoop(ULONG nothing){
	UINT status;
	while(1){
		while (messageThatWasSending!= NULL) {
			if (sendToSMSC(messageThatWasSending)!=OPERATION_SUCCESS){  //send previous message
				tx_thread_sleep(5);//TODO
			}
		}
		status = tx_queue_receive(&ToSendQueue, &messageThatWasSending, TX_WAIT_FOREVER);
		if (status==TX_SUCCESS){
			while (messageThatWasSending!= NULL && (sendToSMSC(messageThatWasSending)!=OPERATION_SUCCESS)){
				tx_thread_sleep(5);//TODO
			}
		}
		else {
			break;//TODO shouldn't happened
		}
	}
}
/**
*
* @param SmsMessage
* @return
*/
result_t sendToSMSC(SMS_SUBMIT*  sms){
//	SMS_SUBMIT sms;
//	sms.data_length=SmsMessage->size;
//	memcpy(&sms.data,&SmsMessage->content,sms.data_length*sizeof(char));
//	memcpy(&sms.recipient_id,&SmsMessage->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
//	memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	unsigned char buffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned length=MAX_SIZE_OF_MES_STRUCT;
	embsys_fill_submit((char *)buffer, sms, &length);

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
		//		else if (status==TX_SUCCESS){//send ping ack
		//			SMS_DELIVER * deliverd=&recivedList[received_message];
		//			Message mes;
		//			isProbAck=1;
		//			memcpy(&probe_ack.sender_id,&deliverd->sender_id,sizeof(char)*ID_MAX_LENGTH);
		//			memcpy(&mes.numberFromTo,&deliverd->sender_id,sizeof(char)*ID_MAX_LENGTH);
		//			memcpy(&probe_ack.timestamp,&deliverd->timestamp,sizeof(char)*TIMESTAMP_MAX_LENGTH);
		//			memcpy(&mes.timeStamp,&deliverd->timestamp,sizeof(char)*ID_MAX_LENGTH);
		//			memcpy(&mes.content,&deliverd->data,sizeof(char)*deliverd->data_length);
		//			mes.size=deliverd->data_length;
		//			mes.inOrOut=IN;
		//			addNewMessageToMessages(&mes);
		//		}
		else{
			break;
		}

		embsys_fill_probe((char *)ProbeBuffer, &probe_ack, isProbAck,&len);
		result_t res=network_send_packet_start((unsigned char *)ProbeBuffer, MAX_SIZE_OF_MES_STRUCT, len);
		if(res!=SUCCESS){
			break;//TODO
		}
	}
}
