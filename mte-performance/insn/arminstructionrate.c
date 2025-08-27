// modifeid version of CAGE (CGO '25) artifact
#include "arminstructionrate.h"

#include <sys/prctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

void *tag_mem;
void *untag_mem;

int main(int argc, char *argv[]) 
{
  //Base value for iterations.  Latency tests require a separate, higher value
  uint64_t iterations = 1500000000;
  uint64_t latencyIterations = iterations * 5;

  float clkSpeed;//CPU Frequency

  untag_mem = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (untag_mem == MAP_FAILED) {
    perror("mmap() failed");
    return EXIT_FAILURE;
  }

  tag_mem = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tag_mem == MAP_FAILED) {
    perror("mmap() failed");
    return EXIT_FAILURE;
  }

  //Establish baseline clock speed for CPU, for all further calculations
  clkSpeed = measureFunction(latencyIterations, clkSpeed, clktest);
  printf("Estimated clock speed: %.2f GHz\n", clkSpeed);

  printf("ldr (PROT_MTE, off) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_tag));
  printf("ldr (no PROT_MTE, off) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_untag));
  printf("str (PROT_MTE, off) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_tag));
  printf("str (no PROT_MTE, off) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_untag));

  if (prctl(PR_SET_TAGGED_ADDR_CTRL, PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_ASYNC | (0xfffe << PR_MTE_TAG_SHIFT), 0, 0, 0)) {
    perror("prctl() failed");
    return EXIT_FAILURE;
  }
  printf("ldr (PROT_MTE, async) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_tag));
  printf("ldr (no PROT_MTE, async) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_untag));
  printf("str (PROT_MTE, async) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_tag));
  printf("str (no PROT_MTE, async) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_untag));

  if (prctl(PR_SET_TAGGED_ADDR_CTRL, PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | (0xfffe << PR_MTE_TAG_SHIFT), 0, 0, 0)) {
    perror("prctl() failed");
    return EXIT_FAILURE;
  }
  printf("ldr (PROT_MTE, sync) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_tag));
  printf("ldr (no PROT_MTE, sync) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldrtestwrapper_untag));
  printf("str (PROT_MTE, sync) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_tag));
  printf("str (no PROT_MTE, sync) per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, strtestwrapper_untag));


  printf("irg throughput: %.2f\n", measureFunction(latencyIterations, clkSpeed, irgtest));
  printf("irg latency: %.2f\n", 1/measureFunction(latencyIterations, clkSpeed, irglatencytest));
  // printf("addg throughput: %.2f\n", measureFunction(latencyIterations, clkSpeed, addgtest));
  // printf("addg latency: %.2f\n", 1/measureFunction(latencyIterations, clkSpeed, addglatencytest));
  // printf("subg throughput: %.2f\n", measureFunction(latencyIterations, clkSpeed, subgtest));
  // printf("subg latency: %.2f\n", 1/measureFunction(latencyIterations, clkSpeed, subglatencytest));
  // printf("subp throughput: %.2f\n", measureFunction(latencyIterations, clkSpeed, subptest));
  // printf("subp latency: %.2f\n", 1/measureFunction(latencyIterations, clkSpeed, subplatencytest));
  // printf("subps throughput: %.2f\n", measureFunction(latencyIterations, clkSpeed, subpstest));
  // printf("subps latency: %.2f\n", 1/measureFunction(latencyIterations, clkSpeed, subpslatencytest));
  printf("stg per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, stgtestwrapper));
  printf("st2g per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, st2gtestwrapper));
  printf("stzg per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, stzgtestwrapper));
  printf("stz2g per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, stz2gtestwrapper));
  printf("stgp per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, stgptestwrapper));
  printf("ldg per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, ldgtestwrapper));
  printf("dcgva per cycle: %.2f\n", measureFunction(latencyIterations, clkSpeed, dcgvatestwrapper));

  return 0;
}

/*Measures the execution time of the test specified, assuming a fixed clock speed.
 *Then calculates the number of operations executed per clk as a measure of throughput.
 *Returns the clk speed if the test was clktest, otherwise returns the opsperclk
 *Param uint64_t iterations: the number of iterations the test should run through
 *Param float clkspeed: the recorded clock frequency of the CPU for the test.
 *Param uint64t (*testfunc) uint64_t: a pointer to the test function to be executed
 */
float measureFunction(uint64_t iterations, float clkSpeed, uint64_t (*testfunc)(uint64_t))
{
  //Time structs for sys/time.h
  struct timeval startTv, endTv;
  struct timezone startTz, endTz;

  uint64_t time_diff_ms, retval;
  float latency, opsPerNs;

  gettimeofday(&startTv, &startTz);//Start timing
  retval = testfunc(iterations);//Assembly Test Execution
  gettimeofday(&endTv, &endTz);//Stop timing

  //Calculate the ops per iteration, or if clktest, the clock speed
  time_diff_ms = 1000 * (endTv.tv_sec - startTv.tv_sec) + ((endTv.tv_usec - startTv.tv_usec) / 1000);
  latency = 1e6 * (float)time_diff_ms / (float)iterations;
  opsPerNs = 1 / latency;

  //Determine if outputting the clock speed or the op rate by checking whether clktest was run
  if(testfunc == clktest)
  {
    clkSpeed = opsPerNs;
    return clkSpeed;
  }
  else
    return opsPerNs / clkSpeed;
}

uint64_t stgtestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return stgtest(iterations, tag_mem);
}

uint64_t st2gtestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return st2gtest(iterations, tag_mem);
}

uint64_t stzgtestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return stzgtest(iterations, tag_mem);
}

uint64_t stz2gtestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return stz2gtest(iterations, tag_mem);
}

uint64_t stgptestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return stgptest(iterations, tag_mem);
}

uint64_t ldgtestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }

  return ldgtest(iterations, tag_mem);
}

uint64_t ldrtestwrapper_tag(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }
  return ldrtest(iterations, tag_mem);
}

uint64_t ldrtestwrapper_untag(uint64_t iterations) {
  if (((uint64_t)untag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }
  return ldrtest(iterations, untag_mem);
}

uint64_t strtestwrapper_tag(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }
  return strtest(iterations, tag_mem);
}

uint64_t strtestwrapper_untag(uint64_t iterations) {
  if (((uint64_t)untag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }
  return strtest(iterations, untag_mem);
}

uint64_t dcgvatestwrapper(uint64_t iterations) {
  if (((uint64_t)tag_mem & 63) != 0) {
    printf("Warning - load may not be 64B aligned\n");
  }
  return dcgvatest(iterations, tag_mem);
}
