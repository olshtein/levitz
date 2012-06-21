/*
 * messages.h
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_
#define NUMBER_DIGTS (8)
#define TIME_STAMP_DIGITS (8)
#define MESSAGE_SIZE (160)
#define MAX_MESSAGES (100)
typedef enum InOrOut{
	IN='I',
			OUT='O'
}InOrOut;
#pragma pack(1)
typedef struct message{
	char numberFromTo[NUMBER_DIGTS];//cannot be int 052->52 etc.
	InOrOut inOrOut;
	char content[MESSAGE_SIZE];
	char timeStamp[TIME_STAMP_DIGITS];
	int size; // real size = size+1
}Message;
typedef struct messagesBuffer{
	Message Messages[MAX_MESSAGES];

}MessagesBuffer;
#pragma pack()


#endif /* MESSAGES_H_ */
