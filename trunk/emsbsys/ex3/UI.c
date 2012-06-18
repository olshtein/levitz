/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "timer.h"

//#include "Globals.h"
typedef enum state{
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;
CHARACTER message1[]={206,229,247,160,160,160,196,229,236,229,244,229,176};
MessagesBuffer messages;
ScreenBuffer screenBuffer;
State curState;

void emptyLine(CHARACTER * line){
	for (int i=0;i<LCD_LINE_LENGTH;i++){
		*line=EMPTY;
		line++;
	}
}
void menuLine(State state,CHARACTER * line){
	switch (state) {
	case MESSAGE_LIST:
		memcpy(line,message1,LCD_LINE_LENGTH);
		//		char *new="New   Delete";
		//		for (int i=0;i<LCD_LINE_LENGTH;i++){
		//		*(line++)=getCHAR(new[i],true);
		//		}
		break;
	case MESSAGE_SHOW:

		break;
	case MESSAGE_WRITE_TEXT:

		break;
	case MESSAGE_WRITE_NUMBER:

		break;
	default:
		break;
	}
}
void getMessage(int messageNumber,CHARACTER * line){
	bool selected=(messageNumber==messages.currentMessage);
	*(line++)=getCHAR((char)('0'+messageNumber/10),selected);
	*(line++)=getCHAR((char)('0'+messageNumber%10),selected);
	*(line++)=getCHAR(' ',selected);
	for (int i=0;i<NUMBER_DIGTS;i++){
		*(line++)=getCHAR(messages.Messages[messageNumber].numberFromTo[i],selected);
	}
	*(line)=getCHAR(messages.Messages[messageNumber].inOrOut,selected);
}


void showListScreen(ULONG a){
	CHARACTER* line=screenBuffer.buffer;
	for (int i=0; i<LCD_NUM_LINES-1;i++){
		if (i<messages.size){
			getMessage((messages.topMessage+i)%messages.size,line);

		}
		else {
			emptyLine(line);
		}
		line+=LCD_LINE_LENGTH;

	}
	menuLine(curState,line);
	while(lcd_set_new_buffer(&screenBuffer)!=OPERATION_SUCCESS);;
}
int l=0;
void noneUI(){
	//	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE%d\n",l++);
}

void initUI(){
	messages.size=0;
	messages.currentMessage=0;
	messages.topMessage=0;
	curState=MESSAGE_LIST;
//	lcd_init(noneUI);
}
void inputPanelCallBack(Button button ){
	switch (curState){
	case MESSAGE_LIST:
		if (button==BUTTON_STAR){
			//			createMessage();
			curState=MESSAGE_WRITE_TEXT;
			//TODO refresh screen
		}
		else if( button==BUTTON_OK){
			//get info and change state
			//			selectMessage();
			curState=MESSAGE_SHOW;
			//TODO refresh screen
		}
		else if(messages.size>0){//2,8 up down in screen, # to delete
				if( button==BUTTON_2){
					if(messages.currentMessage==messages.topMessage){
						messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
						messages.topMessage=messages.currentMessage;
					}
					else messages.currentMessage=((messages.currentMessage+(messages.size-1))%messages.size);
				}
				if( button==BUTTON_8){
					if(messages.currentMessage==(messages.topMessage+LCD_NUM_LINES-2)%messages.size){
						messages.topMessage=(messages.topMessage+1)%messages.size;
					}
					messages.currentMessage=((messages.currentMessage+1)%messages.size);
				}
				if( button==BUTTON_NUMBER_SIGN){
					Message * del=&(messages.Messages[messages.currentMessage]);
					int length=messages.size-messages.currentMessage;
					memmove(del, ++del,length*sizeof(Message) );
					messages.size--;
					messages.currentMessage--;
				}
		showListScreen(0);
			}

		break;
	case MESSAGE_SHOW:
		if (button==BUTTON_STAR){

			curState=MESSAGE_LIST;
			//TODO refresh screen
		}
		if (button==BUTTON_STAR){
			//			deleteMessage();
			curState=MESSAGE_LIST;

		}
		break;
	case MESSAGE_WRITE_TEXT:
		if (button==BUTTON_STAR){
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
		}
		if( button==BUTTON_OK){
			curState=MESSAGE_WRITE_NUMBER;

			//TODO refresh screen
		}
		break;
	case MESSAGE_WRITE_NUMBER:
		if( button==BUTTON_OK){
			//TODO send message and add to message buffer
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
			//TODO refresh screen
		}
		if (button==BUTTON_STAR){//cancel
			//			getCurrentListScreenBuffer();
			curState=MESSAGE_LIST;
		}
		break;
	default:
		//should happen
		break;
	}
}


void addMessage(Message m){
	memcpy(&messages.Messages[messages.size],&m,sizeof(Message));
	messages.size++;
}




