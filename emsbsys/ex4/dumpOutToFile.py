import sys
import os
import array
byteFF = int('11111111', 2) # 0xFF

def main():
	if len(sys.argv)!=4:
		print('Usage:\n\tpython2.7 ex4.py <input file name> <output file name> <total size>')
		sys.exit(-1);
	size=0
	output= open(sys.argv[2],'wb')
	myInput= open(sys.argv[1], 'r')
	
	#NOT USED
	## fill padding before file 
	#for i in range(size,preSize)):
	#	output.write('%c' % byteFF)
	#size+= preSize
		
	#copy File to img
	output.write(myInput.read())
	#get file size (filled till now )
	size+= os.path.getsize(sys.argv[1])
	myInput.close()
	#Fill File with 1's 
	for i in range(size,int(sys.argv[3])):
		output.write('%c' % byteFF)

main();
