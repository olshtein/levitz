/*
 * UI.h
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */

#ifndef UI_H_
#define UI_H_
#include "LCD.h"
//#include "input_panel.h"
#include "tebahpla.h"
//#include "tx_port.h"
#include "tx_api.h"
#include "messages.h"
//void getInput(Button  keyPressed);
void newMessageArrived();
// private
void showListScreen();
void messageDown();
void messageUp();
int getCurrentMessage();
void createMessage();
void deleteMessage();
void initUI();
void mainloop(ULONG b);
void addMessage(Message m);
#endif /* UI_H_ */
