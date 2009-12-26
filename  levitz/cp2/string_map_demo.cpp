#include "StringMap.h"
#include <string>
#include <iostream>
#include <assert.h>

int mainint argc, char *argv[])
{

	// create a new, empty map
	StringMap stringMap;

	// insert some values
	stringMap.setValue(            "Dilbert",               2);
	stringMap.print();
	stringMap.setValue(            "Catbert",               3);
	stringMap.print();
	stringMap.setValue(std::string("Ratbert"),              0);
	stringMap.print();
	stringMap.setValue(std::string("Wally"),               -8);
	stringMap.print();
	stringMap.setValue(std::string("Pointy-haired boss"), 665);

	std::cout << "size: " << stringMap.size() << std::endl;

	int numberOfTheBoss;
	bool hasValue = stringMap.getValue("Pointy-haired boss", &numberOfTheBoss);
	assert(hasValue);
	std::cout << "*** Pointy-haired boss --> " << numberOfTheBoss << std::endl;

	// print contents of the map
	StringMapIterator iter = stringMap.getIterator();
	while (!iter.atEnd()) {
		int value = iter.getValue();
		std::string key = iter.getKey();

		std::cout << key << ": " << value << std::endl;
		iter.goNext();
	}

	// change an existing value
	stringMap.setValue("Catbert", 11);

	int numberOfTheCat;
	hasValue = stringMap.getValue("Catbert", &numberOfTheCat);
	assert(hasValue);
	std::cout << "*** Catbert --> " << numberOfTheCat << std::endl;

	int nb=987;
	std::cout <<nb<<"\n";
	hasValue = stringMap.getValue("Nullbert", &nb);
	std::cout <<nb<<"\n";
assert(!hasValue);

	// yay, nothing needs to be cleaned up!
	return 0;
}
