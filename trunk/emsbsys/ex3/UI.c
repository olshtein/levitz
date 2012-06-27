/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "tx_api.h"
#define BOTTOM_LINE (LCD_LINE_LENGTH-1)
#define PRINT_SCREEN (lcd_set_new_buffer(&screenBuffer));
// the last botton
volatile Button myButton;
volatile Button lastButton;
// the # of times the biutton pressed
volatile int numOfTimes;
volatile int size; // num of messages
volatile int currentMessage; // the selected message (-1 if none selected)
volatile int topMessage;// the top message
#define MOVE_CURSOR_INTERVAL (10)
//volatile int timer;
extern TX_QUEUE receiveQueue;
// Sequence of letters for each button
char const button1[]=".,?1";
char const button2[]="abc2";
char const button3[]="def3";
char const button4[]="ghi4";
char const button5[]="jkl5";
char const button6[]="mno6";
char const button7[]="pqrs7";
char const button8[]="tuv8";
char const button9[]="wxyz9";
char const button0[] =" 0";
TX_EVENT_FLAGS_GROUP event_flags_0;//used for signaling button press

typedef enum state{//which screen to show now
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;
// new delete
CHARACTER const newDeleteMessage[]={206,229,247,160,160,160,196,229,236,229,244,229,176};//"New   Delete" in hex
// back delete
CHARACTER const backDeleteMessage[]={194,225,227,235,160,160,196,229,236,229,244,229,176};//"Back   Delete" in hex

Message toSend;
MessagesBuffer messages;
ScreenBuffer screenBuffer;
volatile State curState;
/**
 *
 * @param line
 */
void emptyLine(CHARACTER * line){
	for (int i=0;i<LCD_LINE_LENGTH;i++){
		*line=EMPTY;
		line++;
	}
}
/**
 *
 * @param state
 * @param line
 */
void menuLine(State state,CHARACTER * line){
	switch (state) {
	case MESSAGE_LIST:
		memcpy(line,newDeleteMessage,LCD_LINE_LENGTH);
		break;
	case MESSAGE_SHOW:
	case MESSAGE_WRITE_TEXT:
	case MESSAGE_WRITE_NUMBER:
		memcpy(line,backDeleteMessage,LCD_LINE_LENGTH);
		break;
	default:
		break;
	}
}
/**
 *
 * @param line
 * @param messageNumber
 * @param selected
 * @return
 */
int drawNum(CHARACTER * line,int messageNumber, bool selected){
	int i;
	for ( i=0;i<NUMBER_DIGTS;i++){
		if(messages.Messages[messageNumber].numberFromTo[i]==0)break;
		*(line++)=getCHAR(messages.Messages[messageNumber].numberFromTo[i],selected);
	}
	for (;i<NUMBER_DIGTS;i++){
		*(line++)=getCHAR(' ',selected);
	}
	return NUMBER_DIGTS;

}
/**
 *
 * @param messageNumber
 * @param line
 */
void getMessage(int messageNumber,CHARACTER * line){
	bool selected=(messageNumber==currentMessage);
	*(line++)=getCHAR((char)('0'+messageNumber/10),selected);
	*(line++)=getCHAR((char)('0'+messageNumber%10),selected);
	*(line++)=getCHAR(' ',selected);
	line+=drawNum(line,messageNumber,selected);
	*(line)=getCHAR(messages.Messages[messageNumber].inOrOut,selected);
}
/**
 *
 * @param line
 * @return
 */
int showMessageSource(CHARACTER * line){
	for(int i=drawNum(line,currentMessage,true);i<LCD_LINE_LENGTH;i++){
		*(line+i)=getCHAR(' ',true);
	}
	return i;
}
/**
 *
 * @param line
 * @return
 */
int showTimeRecived(CHARACTER * line){
	int i=0;
	for(;i<TIME_STAMP_DIGITS;i++){
		if(messages.Messages[currentMessage].inOrOut==IN){

			*line++=getCHAR((messages.Messages[currentMessage]).timeStamp[i],true);
		}
		else *line++=EMPTY;
	}
	for(;i<LCD_LINE_LENGTH;i++){
		if(messages.Messages[currentMessage].inOrOut==IN){

			*line++=getCHAR(' ',true);
		}
		else *line++=EMPTY;

	}
	return i;
}
/**
 *
 * @param line
 * @return
 */
int showMessageContent(CHARACTER * line){
	int i=0;
	for(;i<messages.Messages[currentMessage].size;i++){
		*line++=getCHAR(messages.Messages[currentMessage].content[i],false);
	}
	for(;i<LCD_TOTAL_CHARS-3*LCD_LINE_LENGTH;i++){
		*line++=EMPTY;
	}

	return i;
}
/**
 *
 */
void showMessage(){
	//	ScreenBuffer * sc=&(screenBuffer);
	//	CHARACTER* line1=sc->buffer;
	int i;
	for (i=0;i<NUMBER_DIGTS;i++){
		if(messages.Messages[currentMessage].numberFromTo[i]==0)break;
		screenBuffer.buffer[i]=getCHAR(messages.Messages[currentMessage].numberFromTo[i],true);
	}// show number
	for(;i<LCD_LINE_LENGTH;i++){
		screenBuffer.buffer[i]=getCHAR(' ',true);
	}//fill the line

	for(i=0;i<TIME_STAMP_DIGITS;i++){
		if(messages.Messages[currentMessage].inOrOut==IN){
			screenBuffer.buffer[i+LCD_LINE_LENGTH]=getCHAR((messages.Messages[currentMessage]).timeStamp[i],true);
		}
		else screenBuffer.buffer[i+LCD_LINE_LENGTH]=EMPTY;
	}// show time stamp
	for(;i<LCD_LINE_LENGTH;i++){
		if(messages.Messages[currentMessage].inOrOut==IN){
			screenBuffer.buffer[i+LCD_LINE_LENGTH]=getCHAR(' ',true);
		}
		else screenBuffer.buffer[i+LCD_LINE_LENGTH]=EMPTY;
	}//fill the line

	for(i=0;i<messages.Messages[currentMessage].size;i++){
		screenBuffer.buffer[i+2*LCD_LINE_LENGTH]=getCHAR(messages.Messages[currentMessage].content[i],false);
	} // show the message content
	for(;i<LCD_TOTAL_CHARS-3*LCD_LINE_LENGTH;i++){
		screenBuffer.buffer[i+2*LCD_LINE_LENGTH]=EMPTY;
	}// fill empty

	//	line1+=showMessageSource(line1);
	//	line1+=showTimeRecived(line1);
	//	line1+=showMessageContent(line1);
	menuLine(curState,/*line1);*/&screenBuffer.buffer[LCD_TOTAL_CHARS-LCD_LINE_LENGTH]); // show bottom line message
	PRINT_SCREEN;
}
/**
 *
 * @param a
 */
void showListScreen(ULONG a){
	CHARACTER* line=screenBuffer.buffer;
	for (int i=0; i<LCD_NUM_LINES-1;i++){
		if (i<size){
			getMessage((topMessage+i)%size,line);

		}
		else {
			emptyLine(line);
		}
		line+=LCD_LINE_LENGTH;

	}
	menuLine(curState,line);
	PRINT_SCREEN;
}
int l=0;
void noneUI(){
	//	printf("NONEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE%d\n",l++);
}


//void goUp(){
//}
//void goDown(){
//}
//void deleteMess(){
//
//}
/**
 *
 * @param button
 */
void inputPanelCallBack(Button button ){
	myButton=button;
	int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
}
/**
 *
 * @param button
 * @param numOftimes
 * @return
 */
char getLetter(Button button,int numOftimes){
	//
	if(button== BUTTON_1){
		return button1[numOftimes%4];
	}
	if(button== BUTTON_2){
		return button2[numOftimes%4];
	}
	if(button== BUTTON_3){
		return button3[numOftimes%4];
	}
	if(button== BUTTON_4){
		return button4[numOftimes%4];
	}
	if(button== BUTTON_5){
		return button5[numOftimes%4];
	}
	if(button== BUTTON_6){
		return button6[numOftimes%4];
	}
	if(button== BUTTON_7){
		return button7[numOftimes%5];
	}
	if(button== BUTTON_8){
		return button8[numOftimes%4];
	}
	if(button== BUTTON_9){
		return button9[numOftimes%5];
	}
	if(button== BUTTON_0){
		return button0[numOftimes%2];
	}
	if(button== BUTTON_NUMBER_SIGN){
		return ' ';
	};
	return ']';
}
/**
 *
 */
void createNewMessage(){
	toSend.inOrOut=OUT;
	toSend.size=-1;
	lastButton=BUTTON_STAR;
	//	messageEditMoveCursor();
	for(int i=0;i<LCD_TOTAL_CHARS-LCD_LINE_LENGTH;i++)screenBuffer.buffer[i]=EMPTY;
	ScreenBuffer * s2=&screenBuffer;
	CHARACTER* line2=s2->buffer+LCD_TOTAL_CHARS-LCD_LINE_LENGTH;
	menuLine(curState,line2);
	PRINT_SCREEN;
}
ULONG lastTime=0;
/**
 *
 * @param button
 */
void writeLetter(Button button){
	ULONG current_time= tx_time_get();

	if(button!=lastButton || (current_time-lastTime)>MOVE_CURSOR_INTERVAL/*&& clock jump*/){
		numOfTimes=0;
		lastButton=button;
		if(toSend.size<MESSAGE_SIZE-1)toSend.size++;
	}
	else{
		numOfTimes++;
	}
	lastTime=current_time;
	//reset clock
	char currChar=getLetter(lastButton,numOfTimes);
	screenBuffer.buffer[toSend.size]=getCHAR(currChar,false);
	toSend.content[toSend.size]=currChar;
	//	//TODO show
	PRINT_SCREEN;
}
int newMessageNumberPos;
/**
 *
 * @return
 */
createNewMessageNumber(){
	newMessageNumberPos=-1;
	lastButton=BUTTON_STAR;
	//	messageEditMoveCursor();
	for(int i=0;i<LCD_TOTAL_CHARS-LCD_LINE_LENGTH;i++)screenBuffer.buffer[i]=EMPTY;
	for(int j=0;j<NUMBER_DIGTS;j++)toSend.numberFromTo[j]=0;
	ScreenBuffer * s2=&screenBuffer;
	CHARACTER* line2=s2->buffer+LCD_TOTAL_CHARS-LCD_LINE_LENGTH;
	menuLine(curState,line2);
	PRINT_SCREEN;

}
/**
 *
 * @param button
 */
void writeDigit(Button button){
	//	if(button!=currButton /*&& clock jump*/){
	lastButton=button;
	if(newMessageNumberPos<NUMBER_DIGTS-1)newMessageNumberPos++;
	//	}
	//reset clock
	int num=3;
	if(lastButton==BUTTON_7 || lastButton==BUTTON_9) num=4;
	char currChar=getLetter(lastButton,num);
	screenBuffer.buffer[newMessageNumberPos]=getCHAR(currChar,false);
	toSend.numberFromTo[newMessageNumberPos]=currChar;

	//	//TODO show
	PRINT_SCREEN;
}
/**
 *
 */
void inputPanelLoop(){
	ULONG actual_flags;
	showListScreen(0);
	while (1){
		int status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
		if ((status != TX_SUCCESS) || (actual_flags != 0x1))break;
		switch (curState){
		case MESSAGE_LIST:
			if (myButton==BUTTON_STAR){
				curState=MESSAGE_WRITE_TEXT;
				createNewMessage();
			}
			else if(size>0){//2,8 up down in screen, # to delete
				if( myButton==BUTTON_OK){
					curState=MESSAGE_SHOW;
					showMessage();

				}
				else{
					if( myButton==BUTTON_2){//goUp();
						if(currentMessage==topMessage && (size>=LCD_NUM_LINES)){
							currentMessage=((currentMessage+(size-1))%size);
							topMessage=currentMessage;
						}
						else currentMessage=((currentMessage+(size-1))%size);
					}
					if( myButton==BUTTON_8){//goDown();
						if(currentMessage==(topMessage+LCD_NUM_LINES-2)%size && (size>=LCD_NUM_LINES)){
							topMessage=(topMessage+1)%size;
						}
						currentMessage=((currentMessage+1)%size);

					}
					if( myButton==BUTTON_NUMBER_SIGN){//deleteMess();
						size--;
						for (int i=currentMessage;i<size;i++){
							messages.Messages[i]=messages.Messages[i+1];
						}
						if(currentMessage==size){
							topMessage=0;
							currentMessage=0;
						}
					}
					showListScreen(0);
				}
			}

			break;
		case MESSAGE_SHOW:
			if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			if (myButton==BUTTON_NUMBER_SIGN){
				//			deleteMessage();
				curState=MESSAGE_LIST;
				size--;
				for (int i=currentMessage;i<size;i++){
					messages.Messages[i]=messages.Messages[i+1];
				}
				if(currentMessage==size){
					topMessage=0;
					currentMessage=0;
				}
				showListScreen(0);

			}
			break;
		case MESSAGE_WRITE_TEXT:
			if(myButton==BUTTON_NUMBER_SIGN){
				if(toSend.size>=0){
					screenBuffer.buffer[toSend.size]=EMPTY;
					lastButton=BUTTON_STAR;
					toSend.size--;
					PRINT_SCREEN;
				}
			}
			else if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			else if( myButton==BUTTON_OK){
				curState=MESSAGE_WRITE_NUMBER;
				createNewMessageNumber();
				//TODO refresh screen
			}
			else writeLetter(myButton);
			break;
		case MESSAGE_WRITE_NUMBER:
			if(myButton==BUTTON_NUMBER_SIGN){
				if(newMessageNumberPos>=0){
					screenBuffer.buffer[newMessageNumberPos]=EMPTY;
					lastButton=BUTTON_STAR;
					newMessageNumberPos--;
					PRINT_SCREEN;
				}
			}
			else if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			else if( myButton==BUTTON_OK){
				toSend.size++;
				for(int i=newMessageNumberPos+1;i<NUMBER_DIGTS;i++){
					toSend.numberFromTo[i]=' ';
				}// fill the number with ' '
				if(size==MAX_MESSAGES) size--;
				addMessage(&toSend);
				curState=MESSAGE_LIST;
				//TODO sendToNetwrok
				int stat=sendMessage(&toSend);


				showListScreen(status);

			}
			else writeDigit(myButton);
			break;
		default:
			//should happen
			break;
		}
	}
}
/**
 *
 */
void initUI(){
	size=0;
	currentMessage=0;
	topMessage=0;
	curState=MESSAGE_LIST;
	int status=tx_event_flags_create(&event_flags_0, "event flags 0");
}
/**
 *
 */
void startUI(){
	inputPanelLoop();
}
/**
 * used for adding message received by network and for testing
 * @param m
 */
void addMessage(Message* m){
	memcpy(&messages.Messages[size],m,sizeof(Message));
	size++;
}
/**
 *
 * @param received_message
 */
void addNewMessageToMessages(Message *received_message){
	addMessage(received_message);
	myButton=0;
	int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);//used to refresh screen
}





