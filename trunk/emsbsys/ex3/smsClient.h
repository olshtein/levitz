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
#define SEND_LIST_SIZE (5)

int sendToSMSC(SMS_SUBMIT*  sms);
void sendLoop(ULONG nothing);
result_t initSmsClient();
void receiveLoop();
/**
 *
 * @param mes
 * @return SUCCESS if the message can be added to the tosend queue
 */
EMBSYS_STATUS sendMessage(Message *mes);



void network_packet_transmitted_cb1(const uint8_t *buffer, uint32_t size);
void network_packet_received_cb1(uint8_t buffer[], uint32_t size, uint32_t length);
void network_packet_dropped_cb1(packet_dropped_reason_t t);
void network_transmit_error_cb1(transmit_error_reason_t t,uint8_t *buffer,uint32_t size,uint32_t length );

#endif /* SMSCLIENT_H_ */
