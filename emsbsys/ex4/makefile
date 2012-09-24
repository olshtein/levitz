CC = mcc  -a6 -Os1  -w4 
EX =ex4

# flash.o input_panel.o network.o LCD.o network.o timer.o 
HEADERS = LCD.o timer.o tebahpla.o  input_panel.o UI.o network.o embsys_sms_protocol.o smsClient.o flash.o fs.o
all:  $(EX).out 

codeToFlash: $(EX).out
	$(CC) -g -m $(EX).o -Hldopt=-uInterruptTable  $(HEADERS) ivt.o  tx.a device.svr3  -o  $(EX).out>>$(EX).map	
	python dumpOutToFile.py ex4.out embsys_flash.img 65536
	$(CC) -g -m  bootLoader.svr3 bootLoad.c -o a.out
	
$(EX).out: $(EX).o ivt.o  $(HEADERS) tx.a device.svr3 embsys_sms_protocol.h
	rm -f $(EX).map
	$(CC) -g -m $(EX).o -Hldopt=-uInterruptTable  $(HEADERS) ivt.o  tx.a device.svr3  -o  $(EX).out>>$(EX).map	
 
$(EX).o: $(EX).c fs.o tx_api.h timer.h LCD.h UI.h input_panel.h tx_port.h network.h embsys_sms_protocol.h
	$(CC) -g -c $(EX).c  -o $(EX).o

bonus: embsys_sms_protocol.o

embsys_sms_protocol.o: embsys_sms_protocol.c embsys_sms_protocol.h
	$(CC) -g -c embsys_sms_protocol.c  -o embsys_sms_protocol.o
	
UITester.o:UI.h messages.h input_panel.h timer.h UITester.c embsys_sms_protocol.h
	$(CC) -g -c UITester.c  -o UITester.o
	
chk.out: ivt.o  UITester.o tx.a  $(HEADERS) device.svr3 embsys_sms_protocol.h
	rm -f chk.map
	$(CC) -g  UITester.o -Hldopt=-uInterruptTable  $(HEADERS)  ivt.o tx.a  device.svr3  -o chk.out	-m >chk.map

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

flash.o: flash.c flash.h common_defs.h
	$(CC) -g -c flash.c -o flash.o

fsRun: fs
	runac fs.out @$(EX).args&
	
fsDbg:fs
	mdb fs.out @$(EX).args&


fs: fs.o flash.o  device.svr3
	$(CC) -g -m fs.o flash.o  device.svr3 -o  fs.out>>fs.map	
 
	

fs.o: flash.o fs.h fs.c
	$(CC) -g -c fs.c -o fs.o

LCD.o:  LCD.h LCD.c tebahpla.h  common_defs.h
	$(CC) -g -c LCD.c -o LCD.o
input_panel.o:  input_panel.h input_panel.c common_defs.h 
	$(CC) -g -c input_panel.c -o input_panel.o
	
UI.o:  UI.c UI.h   LCD.h input_panel.h tebahpla.h tx_api.h messages.h
	$(CC) -g -c UI.c -o UI.o

timer.o:  timer.c timer.h common_defs.h
	$(CC) -g -c timer.c -o timer.o

network.o:  network.c network.h 
	$(CC) -g -c network.c -o network.o		
clean:
	rm -f *.o  $(EX).out  $(EX).out $(EX).map chk.map
run:  $(EX).out
	runac  $(EX).out @$(EX).args
dbg:  $(EX).out
	mdb $(EX).out @$(EX).args&
rsol:	
	runac ex3_school_sol.sol @ex3.args & 
	#embsys_networker
tar:
	tar cvf $(EX).tar README makefile  *.c *.h *.sol *.args device.svr3 ivt.s tx.a
	#~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh levio01 issarh

tar2:
	tar cvf $(EX).tar README makefile flash.c input_panel.c LCD.c timer.c network.c *.h  ivt.s tx.a

