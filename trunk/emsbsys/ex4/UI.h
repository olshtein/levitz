/*
 * UI.h
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */
#ifndef UI_H_
#define UI_H_
#include "input_panel.h"
#include "tx_port.h"
#include "messages.h"
//void getInput(Button  keyPressed);
void showListScreen(ULONG a);
void startUI();
void initUI();
void inputPanelCallBack(Button button );
void addMessage(Message* m);
void addNewMessageToMessages(Message *received_message);
#endif /* UI_H_ */
