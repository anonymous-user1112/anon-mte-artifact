/*
MIT License

Copyright (c) 2017 https://github.com/nmrr

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include "LFSR.hpp"
#include "LFSR_util.hpp"

using namespace std;

// This example will calculate the length of the sequence of a 15 bits register with (x0) xor (x1) as feedback

int main(int argc, char **argv)
{
    bool (*getBitOperation)(LFSR &);

    
    for (int i = 10; i < 26; i++)
    {
        switch (i) {
            case 10:
                getBitOperation = xor10;
                break;
            case 11:
                getBitOperation = xor11;
                break;
            case 12:
                getBitOperation = xor12;
                break;
            case 13:
                getBitOperation = xor13;
                break;
            case 14:
                getBitOperation = xor14;
                break;
            case 15:
                getBitOperation = xor15;
                break;
            case 16:
                getBitOperation = xor16;
                break;
            case 17:
                getBitOperation = xor17;
                break;
            case 18:
                getBitOperation = xor18;
                break;
            case 19:
                getBitOperation = xor19;
                break;
            case 20:
                getBitOperation = xor20;
                break;
            case 21:
                getBitOperation = xor21;
                break;
            case 22:
                getBitOperation = xor22;
                break;
            case 23:
                getBitOperation = xor23;
                break;
            case 24:
                getBitOperation = xor24;
                break;
            case 25:
                getBitOperation = xor25;
                break;
            default:
                getBitOperation = xor25;
        }

        // Create a x bits register, by default all bits are set to 0
        LFSR lfsr(i);
        
        // Set the first bit to 1
        lfsr.setBit(0, true);

        // Save the register
        uint32_t * output;
        lfsr.save(output);

        lfsr.printAll();

        uint64_t counter = 0;
        
        // Iterate while the output value is not equal to the initial state
        do
        {
            bool result = getBitOperation(lfsr);
            lfsr.rightShift(result);
            counter++;
        }
        while(!lfsr.compare(output));

        cout << "counter = " << counter << endl;

        delete [] output;
    }
    
    
    
    
    return (0);
}