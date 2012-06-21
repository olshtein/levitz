/*
 * UI.h
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */
#ifndef UI_H_
#define UI_H_
#include "LCD.h"
#include "input_panel.h"
#include "tebahpla.h"
//typedef unsigned long                           ULONG;
#include "tx_port.h"
#include "string.h"
#include "messages.h"
#include "smsClient.h"

//void getInput(Button  keyPressed);
void newMessageArrived();
// private
void showListScreen(ULONG a);
void startUI();
void initUI();
void inputPanelCallBack(Button button );
void addMessage(Message m);
#endif /* UI_H_ */
