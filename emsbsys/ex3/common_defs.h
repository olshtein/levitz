/*
 * common_defs.h
 *
 * Descriptor: Common definitions that are used by the drivers.
 *
 */

#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

//#include "stdint.h"
typedef unsigned char uint8_t;
typedef unsigned long int uint32_t;
typedef unsigned short int uint16_t;


// Return values of external functions.
typedef enum
{
    OPERATION_SUCCESS				= 0,
    NOT_READY					= 1,
    NULL_POINTER				= 2,
    INVALID_ARGUMENTS	        	        = 3,

    NETWORK_TRANSMIT_BUFFER_FULL	        = 4

} result_t;

#define STACK_SIZE (0x1000)
// empty CHARACTER
#define EMPTY (getCHAR(' ',false))  

// Maximum number of character that find on a line
#define LCD_LINE_LENGTH (12)

// Number of lines on the screen
#define LCD_NUM_LINES (18)
// Number of characters on the screen
#define LCD_TOTAL_CHARS (LCD_NUM_LINES*LCD_LINE_LENGTH)
// CHARACTER a char at gsm7+selcted
typedef union{
	uint8_t data;
	struct{
		uint8_t gsm7:7;
		uint8_t selcted:1;
	}character;
}CHARACTER;

// ScreenBuffer rpesend LCD screen content
typedef struct {
        CHARACTER buffer[LCD_TOTAL_CHARS];
}ScreenBuffer;


#endif /* COMMON_DEFS_H_ */
