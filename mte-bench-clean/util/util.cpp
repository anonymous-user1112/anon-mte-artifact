#include "util.hpp"

volatile uint64_t* chase_pointers_global; // to defeat optimizations for pointer chasing algorithm

unsigned long long rdtsc(void)
{
  unsigned long long val;
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));

  return val;
}

void pin_cpu(size_t core_ID)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core_ID, &set);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
		printf("Unable to Set Affinity\n");
		exit(EXIT_FAILURE);
	}
}


int init_mte(int mte){
  /*
   * Use the architecture dependent information about the processor
   * from getauxval() to check if MTE is available.
   */
  if (!((getauxval(AT_HWCAP2)) & HWCAP2_MTE)){
      printf("MTE is not supported\n");
      return EXIT_FAILURE;
  }else{
      printf("MTE is supported\n");
  }

  /*
   * Enable MTE with synchronous checking
   */
  int MTE = PR_MTE_TCF_SYNC;
  if(mte==-1){
      MTE = PR_MTE_TCF_ASYNC;
  }
  if (prctl(PR_SET_TAGGED_ADDR_CTRL,
            PR_TAGGED_ADDR_ENABLE | MTE | (0xfffe << PR_MTE_TAG_SHIFT),
            0, 0, 0)){
          perror("prctl() failed");
          return EXIT_FAILURE;
  }
  return 0;
}



unsigned char * mmap_option(int mte, uint64_t size){
  unsigned char *ptr;
  if(mte==1){
      ptr = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (ptr == MAP_FAILED){
          perror("mmap() failed");
          exit(EXIT_FAILURE);
      }
  }else if(mte==0){
      ptr = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (ptr == MAP_FAILED){
          perror("mmap() failed");
          exit(EXIT_FAILURE);
      }
  }else if(mte==-1){
      ptr = (unsigned char *)mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (ptr == MAP_FAILED){
          perror("mmap() failed");
          exit(EXIT_FAILURE);
      }
  }
  return ptr;
}


unsigned char * mte_tag(unsigned char *ptr, unsigned long long tag, int random){
  unsigned char * ptr_mte = NULL;
  if(random==1){
      ptr_mte = (unsigned char *) insert_random_tag(ptr);
  }else{
      ptr_mte = insert_my_tag(ptr, tag);
  }

  set_tag(ptr_mte);
  return ptr_mte;
}


/* create a cyclic pointer chain that covers all words
   in a memory section of the given size in a randomized order */
// Inspired by: https://github.com/afborchert/pointer-chasing/blob/master/random-chase.cpp
void create_random_chain(uint64_t* indices, uint64_t len) {
  // shuffle indices
  for (uint64_t i = 0; i < len; ++i) {
      indices[i] = i;
  }
  for (uint64_t i = 0; i < len-1; ++i) {
      uint64_t j = i + rand()%(len - i);
      if (i != j) {
          uint64_t temp = indices[i];
          indices[i] = indices[j];
          indices[j] = temp;
      }
  }
}

void read_write_random_order(uint64_t* indices, uint64_t len, uint64_t** ptr) {
  // fill memory with pointer references
  for (uint64_t i = 1; i < len; ++i) {
    ptr[indices[i-1]] = (uint64_t*) &ptr[indices[i]];  
  }
  ptr[indices[len-1]] = (uint64_t*) &ptr[indices[0]];
}

// read after read: dependency
void read_read_dependency(uint64_t** ptr, uint64_t count) {
  // chase the pointers count times
  int curr_count = count;
  uint64_t** p = (uint64_t**) ptr;
  while (curr_count-- > 0) {
    p = (uint64_t**) *p;
  }
  chase_pointers_global = *p;
  
}

// write after write: no dependency
void write_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < (int)count; i++){
    ptr[indices[i]] = (uint64_t)i;  
  }  
}

void write_random_order_DSB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < (int)count; i++){
    ptr[indices[i]] = (uint64_t)i;
    DATA_SYNC_ST;  
  }  
}

void write_random_order_DMB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < (int)count; i++){
    ptr[indices[i]] = (uint64_t)i;
    DMB_SYNC_ST;  
  }  
}

// read after read: no dependency
void read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < (int)count; i++){
    FORCE_READ_INT(ptr[indices[i]]);
  }
}

// read after write: dependency
void store_load_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < ((int)count)-1; i++) {
    ptr[indices[i+1]] = ptr[indices[i]] + i;
  }
}

void store_load_random_order_DSB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < ((int)count)-1; i++) {
    ptr[indices[i+1]] = ptr[indices[i]] + i;
    DATA_SYNC_ST;
  }
}

void store_load_random_order_DMB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count) {
  for (int i = 0; i < ((int)count)-1; i++) {
    ptr[indices[i+1]] = ptr[indices[i]] + i;
    DMB_SYNC_ST; 
  }
}