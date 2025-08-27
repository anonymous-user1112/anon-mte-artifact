#include "LFSR.hpp"

uint32_t LFSR::getSize()
{
    return binarySize;
}

uint32_t LFSR::getArraySize()
{
    return arraySize;
}

void LFSR::rightShift(bool last)
{
    // for (uint32_t j = 0; j < arraySize-1; j++)
    // {
    //     array[j] >>= 1;
    //     setBit(array[j], 31, getBit(array[j+1], 0));
    // }

    array[0] >>= 1;
    setBit(array[0], lastBitPosition, last);
}


void LFSR::printAll()
{
    for (uint32_t j = 0; j < arraySize; j++){
        cout << array[j];

    }
    cout << endl;
}

uint32_t LFSR::getArray()
{
    return array[0];
}

void LFSR::leftShift(bool first)
{
    for (uint32_t j = arraySize-1; j >= 1; j--)
    {
        array[j] <<= 1;
        setBit(array[j], 0, getBit(array[j-1], 31));
    }

    array[0] <<= 1;
    setBit(array[0], 0, first);
}

uint8_t LFSR::get8bit()
{
    return (uint8_t) array[0];
}

uint16_t LFSR::get16bit()
{
    return (uint16_t) array[0];
}

uint32_t LFSR::get32bit()
{
    return array[0];
}

uint32_t LFSR::get32bitArray(uint32_t position)
{
    return array[position];
}

bool LFSR::getBit(uint32_t bitPosition)
{
    return getBit(array[address[bitPosition]], position[bitPosition]);
}

bool LFSR::getFirstBit()
{
    return getBit(array[0], 0);
}

bool LFSR::getLastBit()
{
    return getBit(array[arraySize-1], lastBitPosition);
}

void LFSR::setBit(uint32_t bitPosition, bool value)
{
    setBit(array[address[bitPosition]], position[bitPosition], value);
}

void LFSR::setFirstBit(bool value)
{
    setBit(array[0], 0, value);
}

void LFSR::setLastBit(bool value)
{
    setBit(array[arraySize-1], lastBitPosition, value);
}

void LFSR::save(uint32_t * &output)
{
    output = new uint32_t[arraySize];
    for (uint32_t i = 0; i < arraySize; i++)
    {
        output[i] = array[i];
    }
}

bool LFSR::compare(uint32_t * &output)
{
    for (uint32_t i = 0; i < arraySize; i++)
    {
        if (output[i] != array[i]) return false;
    }

    return true;
}

void LFSR::set(uint32_t * &output)
{
    for (uint32_t i = 0; i < arraySize; i++)
    {
        array[i] = output[i];
    }
}

///// PRIVATE /////

bool LFSR::getBit(uint32_t &array, uint32_t bitPosition)
{
    return (array >> bitPosition) & 0b1;
}

void LFSR::setBit(uint32_t &array, uint32_t bitPosition, bool data)
{
    if (data == true) setBitTo1(array, bitPosition);
    else setBitTo0(array, bitPosition);
}

void LFSR::setBitTo1(uint32_t &array, uint32_t bitPosition)
{
    array |= (1u << bitPosition);
}

void LFSR::setBitTo0(uint32_t &array, uint32_t bitPosition)
{
    array &= ~(1u << bitPosition);
}