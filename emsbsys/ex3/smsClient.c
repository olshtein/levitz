/*
 * smsClient.c
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#include "smsClient.h"
#define MAX_SIZE_OF_MES_STRUCT 161
#define BUFF_SIZE (5)
#define RECIVED_LIST_SIZE (SEND_LIST_SIZE)
char myId[]={'7','7','0','7','7','0','7','7'};
desc_t transmit_buffer[BUFF_SIZE];
desc_t recieve_buffer[BUFF_SIZE];
uint8_t recevedMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
uint8_t tranMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
volatile int toSendListHead=0;
volatile int recivedListHead=0;
volatile bool sendAckRecived;
volatile int data_length;
TX_EVENT_FLAGS_GROUP NetworkOutgoingFlag;
SMS_DELIVER recivedList[RECIVED_LIST_SIZE];
SMS_SUBMIT toSendList[SEND_LIST_SIZE];
SMS_SUBMIT *volatile messageThatWasSent= NULL;
//
extern TX_QUEUE ToSendQueue;
extern TX_QUEUE receiveQueue;

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
	messageThatWasSent= NULL;
	result+=tx_event_flags_create(&NetworkOutgoingFlag,"NetworkOutgoingFlag");
	return result;
}
/**
* call back when a packet was transmissited.
* @param buffer
* @param size
*/
#define TRANSMITED_SUCCSSES (0x1)
#define TRANSMITED_ERROR (0x2)
void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size){
	tx_event_flags_set(&NetworkOutgoingFlag,TRANSMITED_SUCCSSES,TX_OR);
}
/**
* call back when a packet was dropped during transmission.
*/
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length ){
	if(t==BAD_DESCRIPTOR)messageThatWasSent=NULL;
	tx_event_flags_set(&NetworkOutgoingFlag,TRANSMITED_ERROR,TX_OR);
}

void reciveSms(SMS_DELIVER* deliver){
	//	sendProbeAck(deliver);

	Message mes;
	memcpy(&mes.numberFromTo,&(deliver->sender_id),sizeof(char)*ID_MAX_LENGTH);
	memcpy(&mes.timeStamp,&(deliver->timestamp),sizeof(char)*ID_MAX_LENGTH);
	memcpy(&mes.content,&(deliver->data),sizeof(char)*deliver->data_length);
	mes.size=deliver->data_length;
	mes.inOrOut=IN;
	addNewMessageToMessages(&mes);



}

/**
* call back when a packet was received.
* @param buffer
* @param size
* @param length
*/
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length){

	SMS_DELIVER deliver;
	if(embsys_parse_deliver((char*)buffer,&deliver)==SUCCESS){ // deliver message
		recivedList[recivedListHead]=deliver;
		if(tx_queue_send(&receiveQueue, (void *)&(recivedListHead), TX_NO_WAIT)==TX_SUCCESS){
			recivedListHead=(recivedListHead+1)%RECIVED_LIST_SIZE;
			reciveSms(&deliver);
		}
		else {
			//TODO
			data_length=13;
			//			break;

		}


	}
	else{
		SMS_SUBMIT_ACK subm_ack;
		if(embsys_parse_submit_ack((char*)buffer,&subm_ack)==SUCCESS){
			if(messageThatWasSent->msg_reference==subm_ack.msg_reference &&
					memcmp((void*)&(messageThatWasSent->recipient_id),&subm_ack.recipient_id,sizeof(char)*ID_MAX_LENGTH)){
				messageThatWasSent=NULL;
			}
		}
		else {
			//TODO
			data_length=12;
			//	break;

		}
		//		}
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
*
* @param mes
* @return SUCCESS if the message can be added to the tosend queue
*/
EMBSYS_STATUS sendMessage(Message *mes){
	ULONG sizeFreeToSend;
	if(tx_queue_info_get(&ToSendQueue,TX_NULL,TX_NULL,&sizeFreeToSend,TX_NULL,TX_NULL,TX_NULL)==TX_SUCCESS && sizeFreeToSend>0){
		int imputthis=toSendListHead;
		toSendListHead=(toSendListHead+1)%SEND_LIST_SIZE;
		//TODO  problem - with other thread!
		SMS_SUBMIT* toSend= &toSendList[imputthis];
		memcpy(&toSend->data,&mes->content,mes->size*sizeof(char));
		toSend->data_length=mes->size;
		memcpy(&toSend->device_id,&myId,sizeof(char)*ID_MAX_LENGTH);
		memcpy(&toSend->recipient_id,&mes->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
		if(tx_queue_send(&ToSendQueue, (&toSend), TX_NO_WAIT)==TX_SUCCESS) return SUCCESS;
	}
	return FAIL;
}
/**
*
* send messageThatWasSending
* @return
*/
result_t sendToSMSC(){
	//	SMS_SUBMIT sms;
	//	sms.data_length=SmsMessage->size;
	//	memcpy(&sms.data,&SmsMessage->content,sms.data_length*sizeof(char));
	//	memcpy(&sms.recipient_id,&SmsMessage->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
	//	memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

	unsigned char buffer[MAX_SIZE_OF_MES_STRUCT];
	unsigned length=MAX_SIZE_OF_MES_STRUCT;
	SMS_SUBMIT* mymessage=messageThatWasSent;
	embsys_fill_submit((char *)buffer, mymessage, &length);

	result_t res=network_send_packet_start(buffer, MAX_SIZE_OF_MES_STRUCT, length);

	return res;
}

/**
*
* @param nothing
*/
int networkDriverBusy=0;
void sendLoop(ULONG nothing){
	UINT status;
	ULONG actualFlags;
	while(1){
		SMS_SUBMIT* mymess;
		status = tx_queue_receive(&ToSendQueue, &mymess, TX_WAIT_FOREVER);

		if (status==TX_SUCCESS){
			messageThatWasSent=mymess;
			while (messageThatWasSent!= NULL ){
				status=sendToSMSC();
				if (status!=OPERATION_SUCCESS){
					if(status==NETWORK_TRANSMIT_BUFFER_FULL){
						tx_event_flags_get(&NetworkOutgoingFlag,(TRANSMITED_ERROR|TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);

					}
					else { //  message errore;
						messageThatWasSent=NULL;
					}
				}
				else { // message was send toDriver
					tx_event_flags_get(&NetworkOutgoingFlag,(TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
					tx_thread_sleep(80);
				}

			}
		}
		else {
			break;//TODO shouldn't happened
		}
	}
}


SMS_PROBE probe_ack;
unsigned int len12;
char probeBuffer12[MAX_SIZE_OF_MES_STRUCT];
/**
*
* @param deliver
* @param isAck
*/void sendProbe(SMS_DELIVER *deliver){
	char isAck=0;
	if(deliver!=NULL){
		memcpy(&probe_ack.sender_id,&(deliver->sender_id),sizeof(char)*ID_MAX_LENGTH);
		memcpy(&probe_ack.timestamp,&(deliver->timestamp),sizeof(char)*TIMESTAMP_MAX_LENGTH);
		isAck='Y';
	}
	EMBSYS_STATUS  res1=embsys_fill_probe(probeBuffer12, &probe_ack, isAck ,&len12);
	result_t res2=network_send_packet_start((unsigned char *)probeBuffer12, MAX_SIZE_OF_MES_STRUCT, len12);
	if(res1!=SUCCESS||res2!=SUCCESS){
		//TODO
		data_length=res1;
		//			break;
	}
}

/**
*
*/
void receiveLoop(){
	ULONG received_message;
	UINT status;
	memcpy(&probe_ack.device_id,&myId,sizeof(char)*ID_MAX_LENGTH);

	while(1){
		status = tx_queue_receive(&receiveQueue, &received_message, 20);
		if (status == TX_QUEUE_EMPTY){//send ping
			sendProbe(NULL);
		}
		else if (status==TX_SUCCESS){//send ping ack
			sendProbe(&recivedList[received_message]);
		}
		else{
			break;//TODO
		}
	}
}
