/*
 * UI.c
 *
 *  Created on: Jun 12, 2012
 *      Author: issarh
 */


#include "UI.h"
#include "tebahpla.h"
#include "LCD.h"
#include "smsClient.h"

#include "tx_api.h"
#include "fs.h"
#define FLASH_BLOCK_SIZE_FOR_UI (5)
#define FLASH_BLOCK_SIZE_FOR_LOADER_UI (16)

#define BOTTOM_LINE (LCD_LINE_LENGTH-1)					  // the bottom line index
#define UPDATE_SCREEN (lcd_set_new_buffer(&screenBuffer)); // call the lcd and print the screenBuffer
#define MOVE_CURSOR_INTERVAL (10)						  // the time interval for move the cursor (if no button pressed) 
#define MESSAGE_CUTOFF_LENGTH (141)
typedef enum state{										  // the UI states (which screen to show now)
	MESSAGE_LIST=0,
			MESSAGE_SHOW=1,
			MESSAGE_WRITE_TEXT=2,
			MESSAGE_WRITE_NUMBER=3,
}State;

// HARD CODED message for bottom line of screen "new delete" used for message list screen
CHARACTER const newDeleteMessage[]={206,229,247,160,160,160,196,229,236,229,244,229,176}; //"New   Delete" in hex
// HARD CODED message for bottom line of screen "back delete"
CHARACTER const backDeleteMessage[]={194,225,227,235,160,160,196,229,236,229,244,229,176}; //"Back   Delete" in hex

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


// ============================================ UI attrbite ============================================= 

volatile Button myButton;			// the button that was prresd
volatile Button lastButton;			// the last pressed button
volatile int numOfTimes;			// the # of times the biutton pressed
volatile int size;					// the num of messages at the message list
volatile int currentMessage;		// the index of the selected message (-1 if none selected)
volatile int topMessage;			// the index of the message that appear at the top row 
// ====================================================================================================== 


TX_EVENT_FLAGS_GROUP event_flags_0;//used for signaling button press


Message toSend;
MessagesBuffer messages;
ScreenBuffer screenBuffer;
int newMessageNumberPos;
volatile State curState;
/**
 * prints an empty line used to initialize the screen
 * @param line pointer to start of line
 */
void emptyLine(CHARACTER * line){
	for (int i=0;i<LCD_LINE_LENGTH;i++){
		*line=EMPTY;
		line++;
	}
}
/**
 * prints the bottom line depending on state
 * @param state current state
 * @param line  pointer to start of bottom line
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
 * writes the number the message was to or from
 * @param line pointer to start of line to write number
 * @param message Number the number of the message (id)
 * @param selected should the text appear selected
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
 * print a line in the message list
 * @param messageNumber the number of the message (id)
 * @param line is a pointer to the start of line to write number
 */
void getMessage(int messageNumber,CHARACTER * line){
	bool selected=(messageNumber==currentMessage);
	*(line++)=getCHAR((char)('0'+messageNumber/10),selected);//implicit casting from int to char
	*(line++)=getCHAR((char)('0'+messageNumber%10),selected);//implicit casting from int to char
	*(line++)=getCHAR(' ',selected);
	line+=drawNum(line,messageNumber,selected);
	*(line)=getCHAR(messages.Messages[messageNumber].inOrOut,selected);
}
/**
 * fills the line from where draw num finished till the end
 * @param line is a pointer to the start of line to write number
 * @return the amount of char moved in order to move pointer
 */
int showMessageSource(CHARACTER * line){
	for(int i=drawNum(line,currentMessage,true);i<LCD_LINE_LENGTH;i++){
		*(line+i)=getCHAR(' ',true);
	}
	return i;
}
/**
 * used to print the time stamp in the message show screen fills the rest with blanks
 * @param line is a pointer to the start of line to write the time stamp
 * @return the amount of chars printed
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
 * prints the message contents for the show message screen
 * @param line is a pointer to the start of line to write the message contents
 * @return the amount of chars printed
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
 * show on screen a message th echoosen message (used when state is show message)
 */
void showMessage(){
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

	menuLine(curState,/*line1);*/&screenBuffer.buffer[LCD_TOTAL_CHARS-LCD_LINE_LENGTH]); // show bottom line message
	UPDATE_SCREEN;
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
	UPDATE_SCREEN;
}
int l=0;
/**
 * empty method used for testing
 */
void noneUI(){
}


/**
 * wake up UI thread on key press
 * @param button
 */
void inputPanelCallBack(Button button ){
	myButton=button;
	int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
}
/**
 * method converts num of presses to correct letter
 * @param button pressed
 * @param numOftimes number of times the button was pressed in interval
 * @return the appropriate letter
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
	//should never happen
	return ']';
}
/**
 * Initialize the variables of a new message
 * and show it
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
	UPDATE_SCREEN;
}
ULONG lastTime=0;
/**
 * method used to write a single letter
 * uses a timer to move between letters for the same button
 * @param button the button pressed
 */
void writeLetter(Button button){

	if (button!=BUTTON_OK &&
			button!=BUTTON_1&&
			button!=BUTTON_2&&
			button!=BUTTON_3&&
			button!=BUTTON_4&&
			button!=BUTTON_5&&
			button!=BUTTON_6&&
			button!=BUTTON_7&&
			button!=BUTTON_8&&
			button!=BUTTON_9&&
			button!=BUTTON_0)return;

	ULONG current_time= tx_time_get();

	if(button!=lastButton || (current_time-lastTime)>MOVE_CURSOR_INTERVAL){
		numOfTimes=0;
		lastButton=button;
		if(toSend.size<MESSAGE_CUTOFF_LENGTH-1)toSend.size++;
	}
	else{
		numOfTimes++;
	}
	lastTime=current_time;
	//reset clock
	char currChar=getLetter(lastButton,numOfTimes);
	screenBuffer.buffer[toSend.size]=getCHAR(currChar,false);
	toSend.content[toSend.size]=currChar;
	UPDATE_SCREEN;
}
/**
 * Initialize the variables of a new message number screen
 * and show it
 *
 */
void createNewMessageNumber(){
	newMessageNumberPos=-1;
	lastButton=BUTTON_STAR;
	for(int i=0;i<LCD_TOTAL_CHARS-LCD_LINE_LENGTH;i++)screenBuffer.buffer[i]=EMPTY;
	for(int j=0;j<NUMBER_DIGTS;j++)toSend.numberFromTo[j]=NULL_DIGIT;
	ScreenBuffer * s2=&screenBuffer;
	CHARACTER* line2=s2->buffer+LCD_TOTAL_CHARS-LCD_LINE_LENGTH;
	menuLine(curState,line2);
	UPDATE_SCREEN;

}
/**
 * writes at newMessageNumberPos the number pressed
 * @param button the button pressed
 */
void writeDigit(Button button){
	if (button!=BUTTON_OK &&
			button!=BUTTON_1&&
			button!=BUTTON_2&&
			button!=BUTTON_3&&
			button!=BUTTON_4&&
			button!=BUTTON_5&&
			button!=BUTTON_6&&
			button!=BUTTON_7&&
			button!=BUTTON_8&&
			button!=BUTTON_9&&
			button!=BUTTON_0)return;
	lastButton=button;
	if(newMessageNumberPos<NUMBER_DIGTS-1)newMessageNumberPos++;
	//	}
	//reset clock
	int num=3;
	if(lastButton==BUTTON_7 || lastButton==BUTTON_9) num=4;
	char currChar=getLetter(lastButton,num);
	screenBuffer.buffer[newMessageNumberPos]=getCHAR(currChar,false);
	toSend.numberFromTo[newMessageNumberPos]=currChar;

	//	update the show screen
	UPDATE_SCREEN;
}
/**
 * appent str to be <str><num>
 * assume str pointer  has space for at list 8 chars
 * and num<999
 */
#define ZERO_CHAR (48)
void createFileName(char* str,int num){
	strcpy(str,"Mess");
	char * tmp_s=str+4;
	*tmp_s++=(char)(((int)(num/100))+ZERO_CHAR);
	num-=((int)(num/100))*100;
	*tmp_s++=(char)(((int)(num/10))+ZERO_CHAR);
	num-=((int)(num/10))*10;
	*tmp_s++=(char)(num+ZERO_CHAR);
	*tmp_s=0;
}
volatile err=0;
Message TMP_load_Message;
char TMP_FileName[FILE_NAME_SIZE];
void loadMessages(){
	FS_STATUS stat;
	for(size=0;size<MAX_MESSAGES;size++){
		unsigned len=0;
		createFileName(TMP_FileName,size);
		stat= fs_read(TMP_FileName,&len,(char*)&TMP_load_Message);
		if (stat==FILE_NOT_FOUND) return;
		if(len>sizeof(Message)||stat!=FS_SUCCESS){
			//TODO handle eror;
			err=stat;
		}
		memcpy(&messages.Messages[size],&TMP_load_Message,sizeof(Message));
		memset(&TMP_load_Message,0,sizeof(Message));
	}

}
void deleteCurrentMessage(){
	FS_STATUS stat;
	size--;
	//delete message size:
	createFileName(TMP_FileName,size);
	stat= fs_erase(TMP_FileName);
	if(stat!=FS_SUCCESS){
		//TODO handle eror;
		err=stat;
	}
	unsigned len=sizeof(Message);
	for (int i=currentMessage;i<size;i++){
		// change data of mess i fromstat= fs_ FS
		createFileName(TMP_FileName,i);
		stat= fs_write(TMP_FileName,len,(char*)&messages.Messages[i+1]);
		if(stat!=FS_SUCCESS){
			//TODO handle eror;
			err=stat;
		}
		messages.Messages[i]=messages.Messages[i+1];
	}

	if(currentMessage==size){
		topMessage=0;
		currentMessage=0;
	}
}

/**
 * adds message to list
 * used for adding message received by network and for testing
 * @param received_message the message to add
 */
Message TMP_write_Message;
int addNewMessage=0;
void addNewMessageToMessages(Message *received_message){
	if (size==MAX_MESSAGES){
		memcpy(&messages.Messages[size-1],received_message,sizeof(Message));

	}
	else {
		memcpy(&messages.Messages[size],received_message,sizeof(Message));
		size++;
	}
	addNewMessage=true;
	myButton=0;
	int status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);//used to refresh screen

}
/**
 * Infinite loop for UI thread sleep when not pressed on flag  (event_flags_0)
 */
void inputPanelLoop(){
	ULONG actual_flags;
	showListScreen(0);
	while (1){
		int status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
		if ((status != TX_SUCCESS) || (actual_flags != 0x1))break;
		if(addNewMessage==true){
			addNewMessage=false;
			createFileName(TMP_FileName,size-1);
			FS_STATUS stat=fs_write(TMP_FileName,sizeof(Message),(char*)&messages.Messages[size-1]);
			if(stat!=FS_SUCCESS){
				//TODO handle eror;
				err=stat;
			}
		}
		switch (curState){
		//  GUI FOR MESSAGE_LIST
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
						deleteCurrentMessage();

					}
					showListScreen(0);
				}
			}
			break;
			//  GUI FOR MESSAGE_SHOW
		case MESSAGE_SHOW:
			if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			if (myButton==BUTTON_NUMBER_SIGN){
				curState=MESSAGE_LIST;
				deleteCurrentMessage();
				showListScreen(0);

			}
			break;
			//  GUI FOR MESSAGE_WRITE_TEXT
		case MESSAGE_WRITE_TEXT:
			if(myButton==BUTTON_NUMBER_SIGN){
				if(toSend.size>=0){
					screenBuffer.buffer[toSend.size]=EMPTY;
					lastButton=BUTTON_STAR;
					toSend.size--;
					UPDATE_SCREEN;
				}
			}
			else if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			else if( myButton==BUTTON_OK){
				curState=MESSAGE_WRITE_NUMBER;
				createNewMessageNumber();

			}
			else writeLetter(myButton);
			break;
			//  GUI FOR MESSAGE_WRITE_NUMBER
		case MESSAGE_WRITE_NUMBER:
			if(myButton==BUTTON_NUMBER_SIGN){
				if(newMessageNumberPos>=0){
					screenBuffer.buffer[newMessageNumberPos]=EMPTY;
					lastButton=BUTTON_STAR;
					newMessageNumberPos--;
					UPDATE_SCREEN;
				}
			}
			else if (myButton==BUTTON_STAR){

				curState=MESSAGE_LIST;
				showListScreen(0);
			}
			else if( myButton==BUTTON_OK){
				toSend.size++;
				for(int i=newMessageNumberPos+1;i<NUMBER_DIGTS;i++){
					toSend.numberFromTo[i]=NULL_DIGIT;
				}// fill the number with ' '
				if(size==MAX_MESSAGES) size--;
				curState=MESSAGE_LIST;
				// sendToNetwrok
				int stat=sendMessage(&toSend);
				addNewMessageToMessages(&toSend);
				showListScreen(status);

			}
			else writeDigit(myButton);
			break;
		default:
			//should never happen
			break;
		}
	}
}
/**
 * initalize UI on load
 */
void initUI(){
	size=0;
	currentMessage=0;
	topMessage=0;
	curState=MESSAGE_LIST;
	addNewMessage=false;
	int stat=tx_event_flags_create(&event_flags_0, "event flags 0");

}
/**
 * Start point for UI thread
 */
void startUI(){
	FS_SETTINGS fs_set;
	fs_set.block_count=FLASH_BLOCK_SIZE_FOR_LOADER_UI;
	FS_STATUS status =fs_init(fs_set);
	if(status!=SUCCESS){
		//TODO handle eror;
		err=status*err;
	}
	loadMessages();
	inputPanelLoop();
}






