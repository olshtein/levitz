#include "LCD.h"
#include "tebahpla.h"


CHARACTER _lcdData[LCD_LINE_LENGTH*LCD_NUM_LINES];
void (*_lcd_complete_cb)(void);
/**********************************************************************
 *
 * Function:    lcd_init
 *
 * Descriptor:  Initialize the LCD screen.
 *
 * Parameters:  flush_complete_cb: call back whenever flush request was done.
 *
 * Notes:       Default character during initialization should be ' ' (0x20).
 *
 * Return:      OPERATION_SUCCESS:      Initialization successfully done.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t lcd_init(void (*lcd_complete_cb)(void)){
	if(lcd_complete_cb==NULL) return NULL_POINTER;
	_lcd_complete_cb=lcd_complete_cb;
	// init all the registers
	_sr(0,LCD_DIER); // disavle interrupt
	_sr(0,LCD_DBUF);
	_sr(0,LCD_DCMD);
	_sr(0,LCD_DICR);

	for(int i=0;i<LCD_TOTAL_CHARS;i++){
		_lcdData[i]=EMPTY;
	}
	// write to lcd
	_sr((uint32_t)&_lcdData,LCD_DBUF);
	_sr(LCD_ENABLE_INTERRUPT,LCD_DIER);
	_sr(LCD_START_DMA_COPY,LCD_DCMD);



	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    lcd_set_row
 *
 * Descriptor:  Write "length" character that find in the given buffer "line"
 *              start from the beginning of line "row_number".
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Write request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *              NOT_READY:              The device is not ready for a new request.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *
 ***********************************************************************/
result_t lcd_set_row(uint8_t row_number, bool selected, char const line[], uint8_t length){
//	if(line==NULL)return NULL_POINTER;
//	int startPoint=row_number*LCD_LINE_LENGTH;
//	if(startPoint+(int)length>=LCD_LINE_LENGTH*LCD_NUM_LINES) return  INVALID_ARGUMENTS;
//	if((_lr(LCD_DICR)||_lr(LCD_DCMD))&LCD_DMA_CYCLE_COPMLETED!=0) return NOT_READY;
//	for(uint8_t i=0;i<length;i++){
//		_lcdData[startPoint+i]=getCHAR(line[i]);
//		_lcdData[startPoint+i].character.selcted=selected;
//	}
//	// write to lcd
//	_sr((uint32_t)&_lcdData,LCD_DBUF);
//	_sr(LCD_ENABLE_INTERRUPT,LCD_DIER);
//	_sr(LCD_START_DMA_COPY,LCD_DCMD);

	return OPERATION_SUCCESS;
}
result_t lcd_set_new_buffer(ScreenBuffer* sb){
	if(sb==NULL)return NULL_POINTER;
	if((_lr(LCD_DICR)||_lr(LCD_DCMD))&LCD_DMA_CYCLE_COPMLETED!=0) return NOT_READY;
    memcpy(_lcdData,sb->buffer,sizeof(CHARACTER)*LCD_TOTAL_CHARS);

	_sr((uint32_t)&_lcdData,LCD_DBUF);
	_sr(LCD_ENABLE_INTERRUPT,LCD_DIER);
	_sr(LCD_START_DMA_COPY,LCD_DCMD);

	return OPERATION_SUCCESS;

}

void lcd_done(){
	_sr(0,LCD_DIER); // disavle interrupt
	_sr(LCD_DMA_CYCLE_COPMLETED,LCD_DICR);//cleared cycle done
	_lcd_complete_cb();


	//_sr(0,LCD_DIER); // disavle interrupt
	//	_sr(0,LCD_DBUF);
	//	_sr(0,LCD_DCMD);
	//	_sr(0,LCD_DICR);
	//
	//	for(int i=0;i<LCD_LINE_LENGTH*LCD_NUM_LINES;i++){
	//		_lcdData[i].data=tebahpla['9'];
	//	}
	//	// write to lcd
	//	_sr((uint32_t)&_lcdData,LCD_DBUF);
	//	_sr(LCD_ENABLE_INTERRUPT,LCD_DIER);
	//	_sr(LCD_START_DMA_COPY,LCD_DCMD);

}

