/*
 * MyPoly.h
 *
 *  Created on: Jan 2, 2010
 *      Author: levio01
 */

#ifndef MYPOLY_H_
#define MYPOLY_H_
# include<iostream>
#include <string>

class MyPoly {
public:
	MyPoly(double arr[] ,unsigned int arrSize){};
	MyPoly(double num){};
	MyPoly(double X[],double y[], unsigned int arrSize){};
	MyPoly(std::string str){};
	std::string toString() const{};
	double evaluate(double x) const{};
	MyPoly derive(int n) const{};
	~MyPoly(){};
	virtual MyPoly& operator+=(const MyPoly& other){};
	virtual MyPoly& operator*=(const MyPoly& other){};
	virtual MyPoly& operator-=(const MyPoly& other){};
    friend const MyPoly operator- (const MyPoly& p1, const MyPoly& p2){};
    friend const MyPoly operator+ (const MyPoly& p1, const MyPoly& p2){};
    friend const MyPoly operator* (const MyPoly& p1, const MyPoly& p2){};
    friend bool    operator==(const MyPoly& p1, const MyPoly& p2){};
    friend bool    operator!=(const MyPoly& p1, const MyPoly& p2){};
	//----------------------------------
	    // unary operator
	    //----------------------------------
	    // the const on the return type is to
	    // avoid -p1= p10 from working
	const MyPoly operator-() const {};
private:
	enum TYPE{
		REG,SPRASE
	};
	TYPE _type;
	MyPoly * _poly;
	unsigned _deg;
static inline double cut(double d){};
};

#endif /* MYPOLY_H_ */
