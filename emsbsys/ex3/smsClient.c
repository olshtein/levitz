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
char myId[]={'1','1','0','1','1','0','1','1'};
desc_t transmit_buffer[BUFF_SIZE];
desc_t recieve_buffer[BUFF_SIZE];
uint8_t recevedMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
uint8_t tranMsg[BUFF_SIZE][NETWORK_MAXIMUM_TRANSMISSION_UNIT];
volatile int toSendListHead=0;
volatile int recivedListHead=0;
volatile bool sendAckRecived;
volatile int data_length;
TX_EVENT_FLAGS_GROUP NetworkWakeupFlag;
SMS_DELIVER recivedList[RECIVED_LIST_SIZE];
SMS_SUBMIT toSendList[SEND_LIST_SIZE];
SMS_SUBMIT *volatile messageThatWasSent= NULL;
//
extern TX_QUEUE ToSendQueue;
extern TX_QUEUE receiveQueue;
TX_TIMER my_timer;
#define TIMER_EXPIRED (0x1)
#define PING_TIME (50)
#define RECIVED_MESSAGE (0x2)
#define RECIVED_SUMBIT_ACK (0x3)
#define ONE_SHOT (0)

void wakeUp(ULONG reason){
	//    tx_timer_activate(&my_timer);//reset timer
	UINT status=0;
	status=tx_timer_deactivate(&my_timer);
	status+=    tx_timer_change(&my_timer,PING_TIME,0);
	status+=tx_timer_activate(&my_timer);

	//    status+=tx_timer_create(&my_timer,"my_timer_name",wakeUp, TIMER_EXPIRED, PING_TIME, 0,TX_NO_ACTIVATE);
	//    status+= tx_timer_activate(&my_timer);
	status+=tx_event_flags_set(&NetworkWakeupFlag,reason,TX_OR);
	if(status!=SUCCESS){
		//TODO handle error
		data_length=(int)status;
	}
}


/**
 *
 * @return
 */
result_t initSmsClient(){
	network_init_params_t myCoolNetworkParms;
	result_t result ;
	//    networkParms.list_call_backs = NULL;

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
	result+=tx_event_flags_create(&NetworkWakeupFlag,"NetworkWakeupFlag");
	result+=tx_timer_create(&my_timer,"my_timer_name",wakeUp, TIMER_EXPIRED, PING_TIME, PING_TIME,TX_AUTO_ACTIVATE);
	//	wakeUp(TIMER_EXPIRED);
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
	//    tx_event_flags_set(&NetworkWakeupFlag,TRANSMITED_SUCCSSES,TX_OR);
}
/**
 * call back when a packet was dropped during transmission.
 */
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length ){
	if(t==BAD_DESCRIPTOR)messageThatWasSent=NULL;
	//    tx_event_flags_set(&NetworkWakeupFlag,TRANSMITED_ERROR,TX_OR);
}
void getTime(char* mes_timeStamp,char* deliver_timestamp){
	int mes_index=0;
	mes_timeStamp[mes_index++]=deliver_timestamp[7];
	mes_timeStamp[mes_index++]=deliver_timestamp[6];
	mes_timeStamp[mes_index++]=':';
	mes_timeStamp[mes_index++]=deliver_timestamp[9];
	mes_timeStamp[mes_index++]=deliver_timestamp[8];
	mes_timeStamp[mes_index++]=':';
	mes_timeStamp[mes_index++]=deliver_timestamp[11];
	mes_timeStamp[mes_index++]=deliver_timestamp[10];
}
void reciveSms(SMS_DELIVER* deliver){
	//    sendProbeAck(deliver);

	Message mes;
	memcpy(&mes.numberFromTo,&(deliver->sender_id),sizeof(char)*ID_MAX_LENGTH);
	getTime(mes.timeStamp,(deliver->timestamp));
	//	memcpy(&mes.timeStamp,&(deliver->timestamp),sizeof(char)*ID_MAX_LENGTH);

	memcpy(&(mes.content),&(deliver->data),sizeof(char)*deliver->data_length);
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
			wakeUp(RECIVED_MESSAGE);
		}
		else {
			//TODO handle error

			data_length=13;
			//            break;
		}


	}
	else{
		SMS_SUBMIT_ACK subm_ack;
		if(embsys_parse_submit_ack((char*)buffer,&subm_ack)==SUCCESS){
			if(messageThatWasSent->msg_reference==subm_ack.msg_reference){
				for(int k=0;k<ID_MAX_LENGTH ;k++){
					if(messageThatWasSent->recipient_id[k]!=subm_ack.recipient_id[k]) return;
					if(subm_ack.recipient_id[k]=='/0') break;
				}
				messageThatWasSent=NULL;
				wakeUp(RECIVED_SUMBIT_ACK);
			}
		}
		else {
			//TODO handle error
			data_length=12;
			//    break;

		}
		//        }
	}
}

/**
 * call back when a packet was dropped during receiving.
 * @param t
 */
void network_packet_dropped_cb1(packet_dropped_reason_t t){
	data_length=t;
	//TODO handle error

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
	//    SMS_SUBMIT sms;
	//    sms.data_length=SmsMessage->size;
	//    memcpy(&sms.data,&SmsMessage->content,sms.data_length*sizeof(char));
	//    memcpy(&sms.recipient_id,&SmsMessage->numberFromTo,sizeof(char)*ID_MAX_LENGTH);
	//    memcpy(&sms.device_id,&myIp,sizeof(char)*ID_MAX_LENGTH);

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
		 //TODO handle error
		 data_length=res1;
		 //            break;
	 }
 }

 // int networkDriverBusy=0;
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
						 tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_ERROR|TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);

					 }
					 else { //  message errore;
						 messageThatWasSent=NULL;
					 }
				 }
				 else { // message was send toDriver
					 tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
					 tx_thread_sleep(80);
				 }

			 }
		 }
		 else {
			 break;//TODO shouldn't happened
		 }
	 }
 }
 void sendReceiveLoop(){
	 ULONG received_message;
	 UINT status;
	 memcpy(&probe_ack.device_id,&myId,sizeof(char)*ID_MAX_LENGTH);

	 SMS_SUBMIT* mymess;
	 SMS_DELIVER* prob;
	 ULONG actualFlags;
	 while(1){
		 tx_event_flags_get(&NetworkWakeupFlag,(TIMER_EXPIRED|RECIVED_MESSAGE|RECIVED_SUMBIT_ACK)
				 ,TX_OR_CLEAR,&actualFlags,PING_TIME); // wait for timer/flag set
		 if(messageThatWasSent== NULL){
			 status = tx_queue_receive(&ToSendQueue, &mymess, TX_NO_WAIT);
			 if (status==TX_SUCCESS){
				 messageThatWasSent=mymess;
			 }
		 }
		 if(messageThatWasSent!= NULL){
			 status=sendToSMSC();
			 if (status!=OPERATION_SUCCESS){
				 if(status==NETWORK_TRANSMIT_BUFFER_FULL){
					 //                         tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_ERROR|TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
				 }
				 else { //  message errore ;
					 messageThatWasSent=NULL;
				 }
			 }
			 //                 else { // message was send toDriver
			 //                     tx_event_flags_get(&NetworkWakeupFlag,(TRANSMITED_SUCCSSES),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
			 //                     tx_thread_sleep(80);
			 //                 }
		 }
		 else {// no message to send , send ping/recived_ack
			 status = tx_queue_receive(&receiveQueue, &received_message, TX_NO_WAIT);
			 if (status == TX_QUEUE_EMPTY){//send ping
				 prob=NULL;//send ping
			 }
			 else if (status==TX_SUCCESS){//send recived ack
				 prob=&recivedList[received_message];
			 }
			 else{
				 break;//TODO error
			 }
			 sendProbe(prob); //send the ping/recived_ack
		 }

	 }
 }
 //void networkLoop(){
 //    initNetwork();
 //    while (true){
 //        tx_event_flags_get(&receiveSendTimeout,(TRANSMITED_ERROR),TX_OR_CLEAR,&actualFlags,TX_WAIT_FOREVER);
 //        if (ShouldSend)sendSms();
 //        else if (ShouldrecieveMessage)recieveSms();
 //        else
 //        else SendPing();
 //    }
 //}
