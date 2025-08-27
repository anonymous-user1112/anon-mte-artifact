#include "LFSR_util.hpp"

int largestPowerOf2(uint64_t n) {
    if (n <= 1) {
        printf("Input must be greater than 1.\n");
        return -1; // Invalid input
    }

    int x = 0;
    while ((1 << x) < n) {
        x++;
    }
    return x; 
}

bool xor10(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(7);
}

bool xor11(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(9);
}

bool xor12(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(6) ^ lfsr.getBit(8) ^ lfsr.getBit(11);
}

bool xor13(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(9) ^ lfsr.getBit(10) ^ lfsr.getBit(12);
}

bool xor14(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(9) ^ lfsr.getBit(11) ^ lfsr.getBit(13);
}

bool xor15(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(14);
}

bool xor16(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(11) ^ lfsr.getBit(13) ^ lfsr.getBit(14);
}

bool xor17(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(14);
}

bool xor18(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(11);
}

bool xor19(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(14) ^ lfsr.getBit(17) ^ lfsr.getBit(18);
}

bool xor20(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(17);
}

bool xor21(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(19);
}

bool xor22(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(21);
}

bool xor23(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(18);
}

bool xor24(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(20) ^ lfsr.getBit(21) ^ lfsr.getBit(23);
}

bool xor25(LFSR &lfsr) {
    return lfsr.getBit(0) ^ lfsr.getBit(22);
}
