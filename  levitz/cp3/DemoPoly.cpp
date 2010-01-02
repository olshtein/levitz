#include <iostream>
#include "MyPoly.h"
#define EPSILON 0.1

bool EqualDouble(double d1, double d2)
{
    return (d1-d2 < EPSILON && d1-d2 > -EPSILON);
}

int main()
{
    double constArr1[3] = {1., 0.000009, -2.};         // -2x^2+1 
    double constArr2[2] = {-0.5, 2.};               // 2x-0.5 
    MyPoly p1(constArr1, 3);
    MyPoly p2(constArr2, 2);
    MyPoly p3(2.);
    MyPoly pStr("2x^2+4x^1+19x^0"); //string ctor

    if (!EqualDouble(p3.evaluate(5.), 2.))
    {
        std::cerr << "Fail test1" << std::endl;
        return 1;
    }
    if (!EqualDouble(p1.evaluate(2.), p1.evaluate(-2.)))
    {
        std::cerr << "Fail test2" << std::endl;
        return 1;
    }
    if (!EqualDouble(p1.evaluate(2.), -7.))
    {
        std::cerr << "Fail test3" << std::endl;
        return 1;
    }
    if (!EqualDouble(p2.evaluate(1.), 1.5))
    {
        std::cerr << "Fail test4" << std::endl;
        return 1;
    }
    
    // Check compilation
    p1 * p3;
    p3 * p1;
    p2 + p3;
    
    p3 *= p2;                                 // 4x-1
    if (!EqualDouble(p3.evaluate(1.), 3.))
    {
        std::cerr << "Fail test5" << std::endl;
        return 1;
    }
    p3 += p1;                                 // -2x^2+4x
    MyPoly p4 = p3.derive(1);                  // -4x+4
    MyPoly p5 = -p4;                          // 4x-4
    if (!EqualDouble(p4.evaluate(2.), -4))
    {
        std::cerr << "Fail test6" << std::endl;
        return 1;
    }
    if (!EqualDouble(p5.evaluate(2.), 4))
    {
        std::cerr << "Fail test7" << std::endl;
        return 1;
    }
    
    // Interpolation
    double X[4] = {0,1,2,3};
    double Y[4] = {0,1,4,9};
    MyPoly interp(X, Y, 4);
    if (!EqualDouble(interp.evaluate(4.), 16.))
    {
        std::cerr << "Fail test8, interp.evaluate(4.) = " << interp.evaluate(4.) << std::endl;
        return 1;
    }    

    std::cout << "Everything OK" << std::endl;
    return 0;
}
