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


int main(int argc, char *argv[]) {

    // 1 mte, 0 no mte, -1 asyn_mte
    int mte = atoi(argv[1]);
    int size = atoi(argv[2]);
    int iteration = atoi(argv[3]);
    int cpu = atoi(argv[4]);

    pin_cpu(cpu);
    
    // Initialize MTE
    init_mte(mte);
    // Init cycle counter
    init();
    int mte_enable = abs(mte);
    
    int buffer_size = size*1024;
    
    printf("testmte %d\n", mte);
    printf("buffer_size %" PRIu64 "\n", buffer_size);
    printf("iteration %d\n", iteration);
    printf("Pin to CPU %d\n", cpu);

    uint64_t buffer_size_8 = buffer_size/8;

    unsigned char * buffer = NULL;
    buffer = mmap_option(-1, buffer_size);
    mprotect_option(mte_enable, buffer, buffer_size);
    for (int i = 0; i < buffer_size; i++){
        buffer[i] = i % 256;
    }
    
    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag = buffer;
    if(mte_enable){
        buffer_tag = mte_tag(buffer, curr_key, 0);
        for (int j = 16; j < buffer_size; j += 16){
            mte_tag(buffer+j, curr_key, 0);
        }
    }

    uint64_t* indices = (uint64_t*)mmap_option(0, buffer_size_8*sizeof(uint64_t));
    create_random_chain(indices, buffer_size_8);

    uint64_t *buffer_tag_64 = (uint64_t *)buffer_tag;

    int n = 0;
    
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

    for (int j = 0; j < iteration; j++){

        // streaming load
        MEASURE_CYCLE(
            for (int i = 0; i < num_128b; i++){
                //printf("%d %p %d \n", i, &buffer_tag[i*16], buffer_tag[i*16]);
                svuint8_t va = svld1_u8(svptrue_b8(), &buffer_tag[i*16]);
                //svshow(va);
            }
        , 
            "Vector streaming load"
        );
            
        // streaming store
        uint8_t a[] = {100, 101, 102, 103, 104, 105, 106, 107, 100, 101, 102, 103, 104, 105, 106, 107};
        svuint8_t va = svld1_u8(svptrue_b8(), a);
        svbool_t tp = svptrue_b8();

        MEASURE_CYCLE(
            for (int i = 0; i < num_128b; i++){
                svst1_u8(tp, &buffer_tag[i*16], va);
            }
            , 
            "Vector streaming store"
        );
        

        // gather load
        printf("number of 64-bit elements in a pattern: %d\n", svcntd());
        printf("number of 32-bit elements in a pattern: %d\n", svcntw());

        int step = svcntd();
        
        MEASURE_CYCLE(
            for (int i = 0; i < buffer_size_8; i += step) {
                svuint64_t index_vec = svld1_u64(svptrue_b64(), indices + i);
                svuint64_t gathered_data = svld1_gather_u64index_u64(svptrue_b64(), buffer_tag_64, index_vec);
                // svshow_64(index_vec);
                // svshow_64_to_8(gathered_data);
            }
            , 
            "Vector gather load"
        );
        

        // scatter store
        svuint64_t va_64 = svld1_u64(svptrue_b64(), (uint64_t*)a);
        svbool_t tp_64 = svptrue_b64();

        svshow_64(va_64);

        MEASURE_CYCLE(
            for (int i = 0; i < buffer_size_8; i += svcntd()) {
                svuint64_t index_vec = svld1_u64(tp_64, indices + i);
                svst1_scatter_u64index_u64(tp_64, buffer_tag_64, index_vec, va_64);
                // svshow_64(index_vec);
                // printf("%zu %d \n", indices[i], buffer_tag_64[indices[i]]);
            }
            , 
            "Vector scatter store"
        );

        MEASURE_CYCLE(
            for (int i = 0; i < buffer_size_8; i += svcntd()) {
                svuint64_t index_vec = svld1_u64(tp_64, indices + i);
                svst1_scatter_u64index_u64(tp_64, buffer_tag_64, index_vec, va_64);
                DMB_SYNC_ST;
            }
            , 
            "Vector scatter store + DMB ST"
        );

        MEASURE_CYCLE(
            for (int i = 0; i < buffer_size_8; i += svcntd()) {
                svuint64_t index_vec = svld1_u64(tp_64, indices + i);
                svst1_scatter_u64index_u64(tp_64, buffer_tag_64, index_vec, va_64);
                DATA_SYNC_ST;
            }
            , 
            "Vector scatter store + DSB ST"
        );
        
    }

    munmap(buffer, buffer_size);
    munmap(indices, buffer_size_8*sizeof(uint64_t));

  
}