/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>
#include "Globals.h"
typedef enum state{
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;

MessagesBuffer messages;
ScreenBuffer screenBuffer;
State curState;

void showListScreen(){



	for(int i=0;i<LCD_TOTAL_CHARS;i++){
		screenBuffer.buffer[i]=getCHAR((char)(i%127));
	}

	lcd_set_new_buffer(&screenBuffer,LCD_TOTAL_CHARS-1);//TODO
}

void initUI(){
	messages.size=0;
	messages.currentMessage=-1;
	messages.topMessage=0;
	curState=MESSAGE_LIST;
}
void mainloop( ULONG button ){
	//	//	int currentMessage;
	//	switch (curState){
	//	case MESSAGE_LIST:
	showListScreen();
	//
	//		if (button==BUTTON_STAR){
	//			//			createMessage();
	//			curState=MESSAGE_WRITE_TEXT;
	//			//TODO refresh screen
	//		}
	//		if( button==BUTTON_OK){
	//			//get info and change state
	//			//			selectMessage();
	//			curState=MESSAGE_SHOW;
	//			//TODO refresh screen
	//		}
	//		else{//2,8 up down in screen, # to delete
	//			//				getInput(button);
	//			//TODO send buffer to lcd
	//		}
	//		break;
	//	case MESSAGE_SHOW:
	//		if (button==BUTTON_STAR){
	//
	//			curState=MESSAGE_LIST;
	//			//TODO refresh screen
	//		}
	//		if (button==BUTTON_STAR){
	//			//			deleteMessage();
	//			curState=MESSAGE_LIST;
	//
	//		}
	//		break;
	//	case MESSAGE_WRITE_TEXT:
	//		if (button==BUTTON_STAR){
	//			//			getCurrentListScreenBuffer();
	//			curState=MESSAGE_LIST;
	//		}
	//		if( button==BUTTON_OK){
	//			curState=MESSAGE_WRITE_NUMBER;
	//
	//			//TODO refresh screen
	//		}
	//		break;
	//	case MESSAGE_WRITE_NUMBER:
	//		if( button==BUTTON_OK){
	//			//TODO send message and add to message buffer
	//			//			getCurrentListScreenBuffer();
	//			curState=MESSAGE_LIST;
	//			//TODO refresh screen
	//		}
	//		if (button==BUTTON_STAR){//cancel
	//			//			getCurrentListScreenBuffer();
	//			curState=MESSAGE_LIST;
	//		}
	//		break;
	//	default:
	//		//should happen
	//		break;
	//	}
}


void addMessage(Message m){
	memcpy(&messages.Messages[messages.size],&m,sizeof(Message));
	messages.size++;
}




