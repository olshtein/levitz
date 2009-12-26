#include "StringMap.h"
#include <string>
#include <iostream>
#include <assert.h>

#define CHECK_VALUE(str, value) \
	assert(map.getValue(str, &val) && val == value); \
	assert(map.getValue(std::string(str), &val) && val == value)

#define CHECK_NO_VALUE(str) \
	assert(map.getValue(str, &val) == false); \
	assert(map.getValue(std::string(str), &val) == false)

#define ITERATE(str, value) \
	assert(!it.atEnd()); \
	assert(it.getKey().compare(str) == 0); \
	assert(it.getValue() == value); \
	it.goNext();

#define CHECK_SIZE(s) \
	assert(map.size() == s)

int main(int argc, char *argv[])
{
	std::cout << "Running tester (no output should be print)..." << std::endl;

    StringMap map;
	CHECK_SIZE(0);
	
	int val = 0;
	CHECK_NO_VALUE("some value");

	StringMapIterator it = map.getIterator();
	assert(it.atEnd());

	map.setValue("1000", 1000);
	CHECK_VALUE("1000", 1000);
	CHECK_SIZE(1);

	CHECK_NO_VALUE("other value");
	CHECK_SIZE(1);

	std::string str = "1005";
	map.setValue(str, 1005);
	CHECK_VALUE("1005", 1005);
	CHECK_SIZE(2);

	str = "905";
	CHECK_NO_VALUE("905");
	CHECK_VALUE("1005", 1005);
	CHECK_SIZE(2);


	//////////////// iterate... ////////////////
	it = map.getIterator();
	ITERATE("1000", 1000);
	ITERATE("1005", 1005);
	assert(it.atEnd());
	//////////////// end iterate ////////////////


	map.setValue("2000", 2000);
	CHECK_VALUE("2000", 2000);
	CHECK_SIZE(3);


	//////////////// iterate... ////////////////
	it = map.getIterator();
	ITERATE("1000", 1000);
	ITERATE("1005", 1005);
	ITERATE("2000", 2000);
	assert(it.atEnd());
	//////////////// end iterate ////////////////


	map.setValue("0900", 900);
	CHECK_VALUE("0900", 900);
	CHECK_VALUE("1005", 1005);
	CHECK_VALUE("1000", 1000);
	CHECK_SIZE(4);


	//////////////// iterate... ////////////////
	it = map.getIterator();
	ITERATE("0900", 900);
	ITERATE("1000", 1000);
	ITERATE("1005", 1005);
	ITERATE("2000", 2000);
	assert(it.atEnd());
	//////////////// end iterate ////////////////


	map.setValue("1600", 1600);
	CHECK_VALUE("1600", 1600);
	CHECK_VALUE("0900", 900);
	CHECK_NO_VALUE("0902");
	CHECK_SIZE(5);


	//////////////// iterate... ////////////////
	it = map.getIterator();
	ITERATE("0900", 900);
	ITERATE("1000", 1000);
	ITERATE("1005", 1005);
	ITERATE("1600", 1600);
	ITERATE("2000", 2000);
	assert(it.atEnd());
	//////////////// end iterate ////////////////


	map.setValue("1605", 1605);
	CHECK_VALUE("1605", 1605);
	CHECK_VALUE("1000", 1000);
	CHECK_NO_VALUE("0");
	CHECK_SIZE(6);


	//////////////// iterate... ////////////////
	it = map.getIterator();
	ITERATE("0900", 900);
	ITERATE("1000", 1000);
	ITERATE("1005", 1005);
	ITERATE("1600", 1600);
	ITERATE("1605", 1605);
	ITERATE("2000", 2000);
	assert(it.atEnd());
	//////////////// end iterate ////////////////


	std::cout << "Tester end." << std::endl;
    return 0;
}
