CC = mcc  -a6 -Os1  -w4 
EX =ex3

# flash.o input_panel.o network.o LCD.o network.o timer.o 
HEADERS = LCD.o timer.o tebahpla.o  input_panel.o UI.o network.o embsys_sms_protocol.sol embsys_sms_protocol.o smsClient.o
all:  $(EX).out 

$(EX).out: $(EX).o ivt.o  $(HEADERS) tx.a device.svr3 embsys_sms_protocol.h
	rm -f $(EX).map
	$(CC) -g -m $(EX).o -Hldopt=-uInterruptTable  $(HEADERS) ivt.o  tx.a device.svr3  -o  $(EX).out>>$(EX).map	
 
$(EX).o: $(EX).c tx_api.h timer.h LCD.h UI.h input_panel.h tx_port.h network.h embsys_sms_protocol.h
	$(CC) -g -c $(EX).c  -o $(EX).o
 
 
UITester.o:UI.h messages.h input_panel.h timer.h UITester.c embsys_sms_protocol.h
	$(CC) -g -c UITester.c  -o UITester.o
	
chk.out: ivt.o  UITester.o tx.a  $(HEADERS) device.svr3 embsys_sms_protocol.h
	rm -f chk.map
	$(CC) -g  UITester.o -Hldopt=-uInterruptTable  $(HEADERS)  ivt.o tx.a  device.svr3  -o chk.out	-m >chk.map

bonus: embsys_sms_protocol.o 

chk: chk.out

chkDbg:chk.out
	mdb chk.out @$(EX).args&
chkRun:chk.out
	runac chk.out @$(EX).args
tebahpla.o: tebahpla.c tebahpla.h LCD.h common_defs.h
	$(CC) -g -c tebahpla.c -o tebahpla.o

ivt.o: ivt.s
	$(CC) -g -c ivt.s -o ivt.o

smsClient.o: smsClient.c smsClient.h  embsys_sms_protocol.h messages.h network.h
	$(CC) -g -c smsClient.c -o smsClient.o


LCD.o:  LCD.h LCD.c tebahpla.h  common_defs.h
	$(CC) -g -c LCD.c -o LCD.o
input_panel.o:  input_panel.h input_panel.c common_defs.h 
	$(CC) -g -c input_panel.c -o input_panel.o
	
UI.o:  UI.c UI.h   LCD.h input_panel.h tebahpla.h tx_api.h messages.h
	$(CC) -g -c UI.c -o UI.o

timer.o:  timer.c timer.h common_defs.h
	$(CC) -g -c timer.c -o timer.o

embsys_sms_protocol.o: embsys_sms_protocol.c embsys_sms_protocol.h tebahpla.h
	$(CC) -g -c embsys_sms_protocol.c -o embsys_sms_protocol.o		
network.o:  network.c network.h 
	$(CC) -g -c network.c -o network.o		
clean:
	rm -f *.o  $(EX).out  $(EX).out $(EX).map chk.map
run:  $(EX).out
	runac  $(EX).out @$(EX).args
dbg:  $(EX).out
	mdb $(EX).out @$(EX).args&
rsol:	
	runac ex3_school_sol.sol @ex3.args & #embsys_networker
tar:
	tar cvf $(EX).tar README makefile  *.c *.h *.args device.svr3 ivt.s
	#~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh levio01 issarh

tar2:
	tar cvf $(EX).tar README makefile  flash.c input_panel.c LCD.c timer.c network.c *.h  ivt.s

