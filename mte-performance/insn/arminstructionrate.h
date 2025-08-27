#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

//extern uint64_t noptest(uint64_t iterations) asm("noptest");
extern uint64_t clktest(uint64_t iterations) asm("clktest");
// mte tests
extern uint64_t irgtest(uint64_t iterations) asm("irgtest");
extern uint64_t irglatencytest(uint64_t iterations) asm("irglatencytest");
extern uint64_t addgtest(uint64_t iterations) asm("addgtest");
extern uint64_t addglatencytest(uint64_t iterations) asm("addglatencytest");
extern uint64_t subgtest(uint64_t iterations) asm("subgtest");
extern uint64_t subglatencytest(uint64_t iterations) asm("subglatencytest");
extern uint64_t subptest(uint64_t iterations) asm("subptest");
extern uint64_t subplatencytest(uint64_t iterations) asm("subplatencytest");
extern uint64_t subpstest(uint64_t iterations) asm("subpstest");
extern uint64_t subpslatencytest(uint64_t iterations) asm("subpslatencytest");
extern uint64_t stgtest(uint64_t iterations, char *ptr) asm("stgtest");
extern uint64_t st2gtest(uint64_t iterations, char *ptr) asm("st2gtest");
extern uint64_t stzgtest(uint64_t iterations, char *ptr) asm("stzgtest");
extern uint64_t stz2gtest(uint64_t iterations, char *ptr) asm("stz2gtest");
extern uint64_t stgptest(uint64_t iterations, char *ptr) asm("stgptest");
extern uint64_t ldgtest(uint64_t iterations, char *ptr) asm("ldgtest");
extern uint64_t ldrtest(uint64_t iterations, char *ptr) asm("ldrtest");
extern uint64_t strtest(uint64_t iterations, char *ptr) asm("strtest");
extern uint64_t dcgvatest(uint64_t iterations, char *ptr) asm("dcgvatest");

float measureFunction(uint64_t iterations, float clockSpeedGhz, uint64_t (*testfunc)(uint64_t));
uint64_t stgtestwrapper(uint64_t iterations);
uint64_t st2gtestwrapper(uint64_t iterations);
uint64_t stzgtestwrapper(uint64_t iterations);
uint64_t stz2gtestwrapper(uint64_t iterations);
uint64_t stgptestwrapper(uint64_t iterations);
uint64_t ldgtestwrapper(uint64_t iterations);
uint64_t ldrtestwrapper_tag(uint64_t iterations);
uint64_t ldrtestwrapper_untag(uint64_t iterations);
uint64_t strtestwrapper_tag(uint64_t iterations);
uint64_t strtestwrapper_untag(uint64_t iterations);
uint64_t dcgvatestwrapper(uint64_t iterations);
