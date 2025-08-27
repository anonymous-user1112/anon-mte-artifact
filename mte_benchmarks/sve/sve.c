#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif
#include <stdio.h>   
#include <stdlib.h> 
#include <inttypes.h>
#include <stdint.h>
#include "util.h"
#include <sys/resource.h>

void svshow(svuint8_t va) {
  int n = svcntb();
  uint8_t* a = (uint8_t*)malloc(n*sizeof(uint8_t));
  svbool_t tp = svptrue_b8();
  svst1_u8(tp, a, va);
  for (int i = 0; i < n; i++) {
    printf("%d ", a[i]);
  }
  printf("\n");
  free(a);
}

void svshow_32(svuint32_t va) {
  int n = svcntw();
  uint32_t* a = (uint32_t*)malloc(n*sizeof(uint32_t));
  svbool_t tp = svptrue_b32();
  svst1_u32(tp, (uint32_t*)a, va);
  for (int i = 0; i < n; i++) {
    printf("%d ", a[i]);
  }
  printf("\n");
  free(a);
}

void svshow_64_to_8(svuint64_t va) {
  int n = svcntb();
  uint8_t* a = (uint8_t*)malloc(n*sizeof(uint8_t));
  svbool_t tp = svptrue_b8();
  svst1_u64(tp, (uint64_t*)a, va);
  for (int i = 0; i < n; i++) {
    printf("%d ", a[i]);
  }
  printf("\n");
  free(a);
}

void svshow_64(svuint64_t va) {
  int n = svcntd();
  uint64_t* a = (uint64_t*)malloc(n*sizeof(uint64_t));
  svbool_t tp = svptrue_b64();
  svst1_u64(tp, (uint64_t*)a, va);
  for (int i = 0; i < n; i++) {
    printf("%d ", a[i]);
  }
  printf("\n");
  free(a);
}

int test_mte(int testmte){

    int page_size = sysconf(_SC_PAGESIZE);

    unsigned char *temp1, *temp2;
    temp1 = mmap_option(0, page_size);
    temp2 = mmap_option(0, page_size);
    printf("temp1: %p\n", temp1);
    printf("temp2: %p\n", temp2);
    for (int i = 0; i < page_size; i++){
      temp1[i] = i % 256;
    }
    for (int i = 0; i < page_size; i++){
      temp2[i] = i % 256;
    }
    unsigned long long curr_key1 = (unsigned long long) (1);
    unsigned long long curr_key2 = (unsigned long long) (2);
    temp1 = mte_tag(temp1, curr_key1, 0);
    temp2 = mte_tag(temp2, curr_key2, 0);
    printf("temp1 after tag: %p\n", temp1);
    printf("temp2 after tag: %p\n", temp2);
    unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 & 0x00FFFFFFFFFFFFFF);
    unsigned char *temp2_no_tag = (unsigned char *) ((uint64_t) temp2 & 0x00FFFFFFFFFFFFFF);
    printf("temp1_no_tag: %p\n", temp1_no_tag);
    printf("temp2_no_tag: %p\n", temp2_no_tag);
    uint64_t diff = (uint64_t) temp1_no_tag - (uint64_t) temp2_no_tag;
    printf("diff: %" PRIu64 "\n", diff);

    svuint8_t va = svld1_u8(svptrue_b8(), &temp2[diff]);
    svshow(va);

    munmap(temp1, page_size);
    munmap(temp2, page_size);

    
    return 0;
            
}


int main(int argc, char *argv[]) {

  // 1 mte, 0 no mte, -1 asyn_mte
  int testmte = atoi(argv[1]);
  int workload = atoi(argv[2]);
  int granularity = atoi(argv[3]);

  pin_cpu(0);
  setpriority(PRIO_PROCESS, 0, -20);
  
  // Initialize MTE
  init_mte(testmte);
  // Init cycle counter
  init();
  int mte = abs(testmte);
  // test_mte(testmte);
  
  int buffer_size = 4096*4*1024;

  uint64_t buffer_size_8 = 0;
  if(granularity==0){ // every 32 chunks
    buffer_size_8 = buffer_size/4;
  }else if(granularity==1){ // every 64 chunks
    buffer_size_8 = buffer_size/8;
  }

  unsigned char * buffer = NULL;
  buffer = mmap_option(-1, buffer_size);

  mprotect_option(mte, buffer, buffer_size);
  for (int i = 0; i < buffer_size; i++){
    buffer[i] = i % 256;
  }
  
  unsigned long long curr_key = (unsigned long long) (1);
  unsigned char *buffer_tag = buffer;
  if(mte){
    buffer_tag = mte_tag(buffer, curr_key, 0);
    for (int j = 16; j < buffer_size; j += 16){
      mte_tag(buffer+j, curr_key, 0);
    }
  }

  uint64_t* indices = (uint64_t*)mmap_option(0, buffer_size_8*sizeof(uint64_t));
  create_random_chain(indices, buffer_size_8);

  uint64_t *buffer_tag_64 = (uint64_t *)buffer_tag;
  uint32_t *buffer_tag_32 = (uint32_t *)buffer_tag;

  int n = 0;
  uint64_t begin_temp = 0;
  uint64_t end_temp = 0;
  #ifdef __ARM_FEATURE_SVE
  n = svcntb();
  #endif

  if (!n) {
    printf("SVE is unavailable.\n");
    return 0;
  }
  
  printf("SVE is available. The length is %d bytes\n", n);
  int num_128b = buffer_size/n;
  printf("Number of vector element %d \n", num_128b);

  if(workload==1){ // streaming load

    MEM_BARRIER;
    begin_temp = cpucycles();
    MEM_BARRIER;

    for (int i = 0; i < num_128b; i++){
      //printf("%d %p %d \n", i, &buffer_tag[i*16], buffer_tag[i*16]);
      svuint8_t va = svld1_u8(svptrue_b8(), &buffer_tag[i*16]);
      //svshow(va);
    }
    MEM_BARRIER;
    end_temp = cpucycles();
    MEM_BARRIER;
      
    printf("vector load : %zu ticks \n", (size_t)end_temp-begin_temp);
  
  }else if(workload==2){ // streaming store

    uint8_t a[] = {100, 101, 102, 103, 104, 105, 106, 107, 100, 101, 102, 103, 104, 105, 106, 107};
    svuint8_t va = svld1_u8(svptrue_b8(), a);
    svbool_t tp = svptrue_b8();

    MEM_BARRIER;
    begin_temp = cpucycles();
    MEM_BARRIER;

    for (int i = 0; i < num_128b; i++){
      svst1_u8(tp, &buffer_tag[i*16], va);
    }

    MEM_BARRIER;
    end_temp = cpucycles();
    MEM_BARRIER;
      
    printf("vector store : %zu ticks \n", (size_t)end_temp-begin_temp);

  }else if(workload==3){ // gather load

    printf("number of 64-bit elements in a pattern: %d\n", svcntd());
    printf("number of 32-bit elements in a pattern: %d\n", svcntw());

    int step = 0;
    if(granularity==0){ // every 32 chunks
      step = svcntw();
    }else if(granularity==1){ // every 64 chunks
      step = svcntd();
    }

    MEM_BARRIER;
    begin_temp = cpucycles();
    MEM_BARRIER;

    if(granularity==0){ 
      for (int i = 0; i < buffer_size_8; i += step) {
        svuint32_t index_vec = svld1_u32(svptrue_b32(), indices + i);
        svuint32_t gathered_data = svld1_gather_u32index_u32(svptrue_b32(), buffer_tag_32, index_vec);   
        svshow_32(index_vec);
        //svshow_64_to_8(gathered_data);
      }
    }else if(granularity==1){ 
      for (int i = 0; i < buffer_size_8; i += step) {
        svuint64_t index_vec = svld1_u64(svptrue_b64(), indices + i);
        svuint64_t gathered_data = svld1_gather_u64index_u64(svptrue_b64(), buffer_tag_64, index_vec);
        // svshow_64(index_vec);
        // svshow_64_to_8(gathered_data);
      }
    }

    MEM_BARRIER;
    end_temp = cpucycles();
    MEM_BARRIER;
      
    printf("vector gather load : %zu ticks \n", (size_t)end_temp-begin_temp);

  }else if(workload==4){ // scatter store

    uint8_t a[] = {100, 101, 102, 103, 104, 105, 106, 107, 100, 101, 102, 103, 104, 105, 106, 107};
    svuint64_t va = svld1_u64(svptrue_b64(), (uint64_t*)a);
    svbool_t tp = svptrue_b64();

    svshow_64(va);

    MEM_BARRIER;
    begin_temp = cpucycles();
    MEM_BARRIER;

    for (int i = 0; i < buffer_size_8; i += svcntd()) {
      svuint64_t index_vec = svld1_u64(tp, indices + i);
      svst1_scatter_u64index_u64(tp, buffer_tag_64, index_vec, va);
      // svshow_64(index_vec);
      // printf("%zu %d \n", indices[i], buffer_tag_64[indices[i]]);
    }
    
    MEM_BARRIER;
    end_temp = cpucycles();
    MEM_BARRIER;
      
    printf("vector gather store : %zu ticks \n", (size_t)end_temp-begin_temp);
  }

  
  munmap(buffer, buffer_size);
  
}