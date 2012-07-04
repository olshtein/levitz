/*
 * messages.h
 *
 *  Created on: Jun 14, 2012
 *      Author: issarh
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

// the # of recipient_id/sender_id/device_id digits
#define NUMBER_DIGTS (ID_MAX_LENGTH)
// the # of Messages time stamp digits
#define TIME_STAMP_DIGITS (8)
// max size of Messages data
#define MESSAGE_SIZE (160)
// max num of Messages
#define MAX_MESSAGES (100)

// Message type
typedef enum InOrOut{
	IN='I',
			OUT='O'
}InOrOut;

// Message struct
#pragma pack(1)
typedef struct message{
	char numberFromTo[NUMBER_DIGTS];//cannot be int 052->52 etc.
	InOrOut inOrOut;
	char content[MESSAGE_SIZE];
	char timeStamp[TIME_STAMP_DIGITS];
	int size; // real size = size+1
}Message;

// the Messages list
typedef struct messagesBuffer{
	Message Messages[MAX_MESSAGES];
}MessagesBuffer;
#pragma pack()


#endif /* MESSAGES_H_ */
