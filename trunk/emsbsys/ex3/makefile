CC = mcc  -a6 -O6  -w4 
EX =ex3

# flash.o input_panel.o network.o LCD.o network.o timer.o 
HEADERS = timer.o tebahpla.o LCD.o input_panel.o UI.o 
all:  $(EX).out 

 $(EX).out: $(EX).c ivt.o LCD.o UI.o  tebahpla.o tx.a timer.o device.svr3 
	$(CC) -g -m $(EX).c -Hldopt=-uInterruptTable ivt.o  tx.a tebahpla.o UI.o LCD.o timer.o device.svr3  -o  $(EX).out>>$(EX).map	

UITester.o:
	$(CC) -g -c UITester.c UI.c -o UITester.o
	
chk.out: ivt.o  UITester.o tx.a  $(HEADERS) device.svr3 
	$(CC) -g  UITester.o -Hldopt=-uInterruptTable  $(HEADERS)  ivt.o tx.a  device.svr3  -o chk.out	

chk: chk.out

chkDbg:chk.out
	mdb chk.out @$(EX).args&

tebahpla.o: tebahpla.c tebahpla.h
	$(CC) -g -c tebahpla.c -o tebahpla.o

ivt.o: ivt.s
	$(CC) -g -c ivt.s -o ivt.o

LCD.o:  LCD.h LCD.c
	$(CC) -g -c LCD.c -o LCD.o
input_panel.o:  input_panel.h input_panel.c
	$(CC) -g -c input_panel.c -o input_panel.o
	
UI.o:  UI.c UI.h
	$(CC) -g -c UI.c -o UI.o

timer.o:  timer.c timer.h
	$(CC) -g -c timer.c -o timer.o
		
clean:
	rm -f *.o  $(EX).out  $(EX).out $(EX).map
run:  $(EX).out
	runac  $(EX).out @$(EX).args&
dbg:  $(EX).out
	mdb $(EX).out @$(EX).args&
rsol:	
	runac ex3_school_sol.out @ex3.args #& embsys_networker
tar:
	tar cvf $(EX).tar README makefile  *.c *.h *.args device.svr3 ivt.s
	#~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh levio01 issarh

tar2:
	tar cvf $(EX).tar README makefile  flash.c input_panel.c LCD.c timer.c network.c *.h  ivt.s

