#ifndef GLOBALS_H_
#define LCD_H_
#define LCD_DBUF (0x1f0)//Display data register
#define LCD_DCMD (0x1f1)//Display command register
#define LCD_START_DMA_COPY (0x1)
#define LCD_DIER (0x1f2)//Display interrupt enable register
#define LCD_ENABLE_INTERRUPT (0x1)
#define LCD_DICR (0x1f3)//Display interrupt cause register
#define LCD_DMA_CYCLE_COPMLETED (0x1)
#define STACK_SIZE (0x1000/10)
#define EMPTY (getCHAR(' ',false))
#define NULL (0)
// Maximum number of character that find on a line
#define LCD_LINE_LENGTH (12)

// Number of lines on the screen
#define LCD_NUM_LINES (18)
#define LCD_TOTAL_CHARS (LCD_NUM_LINES*LCD_LINE_LENGTH)
#endif
