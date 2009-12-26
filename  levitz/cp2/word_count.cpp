/*
 * word_count.cpp
 *
 *  Created on: Dec 26, 2009
 *      Author: levio01
 */
# include "StringMap.h"
# include <string>
# include <iostream>
# define TO_SMALL_LETTER(n) (n+32)
# define GERSH (39)
bool isNotLegal(const char& c){
	if ((c!=GERSH && (c<'A')) || ((c>'Z') && (c<'a')) || (c>'z')){
		return true;
	}
	return false;
}
bool isCapital(const char& c){
	if((c>='A') && (c<='Z')) {
		return true;
	}
	return false;
}
bool chkMakeLegall(std::string& str){
	std::string tmp=str;
	std::string::iterator it=str.begin();
	int i=0;
	while(it<str.end() && isNotLegal(*it)) it=str.erase(it);
	while(it<str.end() && !isNotLegal(*it)) it++;
	str.erase(it,str.end());
	if (str.empty()){
		return false;
	}
	for(it =str.begin();it!=str.end();i++){
		if (isCapital(*it)){
			str.replace(it++,it,1,TO_SMALL_LETTER(*it));
		}
		else it++;
	}
	return true;
}
void read (StringMap& map, int& numOfWords){
	std::string str;
//	/*                a*/std::string a("aa");
	while (/*str.compare(a)!=0*/!std::cin.eof()){
		if (chkMakeLegall(str)){
			numOfWords++;
			int n;
			if (map.getValue(str,&n)) map.setValue(str,n+1);
			else map.setValue(str,1);
		}
		std::cin>>str;
	}
}
void print(StringMap& map, int& numOfWords){
	StringMapIterator it=map.getIterator();
	while (!it.atEnd()){
		double freq=(double)it.getValue()/numOfWords;
		std::cout<<it.getKey()<<"\t"<<it.getValue()<<"\t"<<freq<<"\n";
		it.goNext();
	}
}
int main(int argc, char **argv) {
	StringMap map;
	int numOfWords=0;
	read(map,numOfWords);
	print(map,numOfWords);
	return EXIT_SUCCESS;
}

