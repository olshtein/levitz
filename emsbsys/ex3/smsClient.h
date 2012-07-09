/*
 * smsClient.h
 *
 *  Created on: Jun 20, 2012
 *      Author: issarh
 */

#ifndef SMSCLIENT_H_
#define SMSCLIENT_H_

#include "embsys_sms_protocol.h"
#include "network.h"
#include "messages.h"
#include "string.h"
#include "tx_api.h"
#include "UI.h"
#define SEND_LIST_SIZE (5) // the size of send list
#define NULL_DIGIT ('@')   // null digit char

//===========================private methods =====================
/*
 * send Message from toSend List
 */
int sendToSMSC();



//===========================public  methods =====================


/**
 * initlaise to smsClient
 * @ return SUCCESS if success
 */
result_t initSmsClient();

/*
 * the main smsClient thread loop . send and recive messages
 */
void sendReceiveLoop(ULONG nothing);

/**
 * send Message mes
 * @param mes the Message to send
 * @return SUCCESS if the message can be added to the tosend queue
 */
EMBSYS_STATUS sendMessage(Message *mes);

void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size);
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length);
void network_packet_dropped_cb1(packet_dropped_reason_t t);
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length );

#endif /* SMSCLIENT_H_ */
