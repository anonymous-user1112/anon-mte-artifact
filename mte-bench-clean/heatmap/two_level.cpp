#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "util.hpp"


volatile uint8_t total; 

struct node {
    uint8_t* data;
    struct node *next;
 };
 struct node *head = NULL;
 
//insertion at the beginning
void insertatbegin_level2(uint8_t* data, int Level1){
    struct node *lk = (struct node*) malloc(sizeof(struct node));
    lk->data = (uint8_t*) malloc(sizeof(uint8_t)*Level1);
    memcpy(lk->data, data, Level1*sizeof(uint8_t));
    lk->next = head;
    head = lk;
}

uint8_t searchlist_level2(int step, int Level1){ 
    struct node *temp = head; 
    while(temp != NULL) {
        uint8_t load_data = temp->data[0];
        for (int i = 1; i < Level1; i+=step) {
            load_data =  (uint8_t)(temp->data[i]);
            FORCE_READ_INT(load_data);
        }
        temp=temp->next;
    }
    return 0;
}

void delete_ll(){
    struct node *temp = head; 
    while(temp != NULL) {
        struct node *curr= temp;
        temp=temp->next;
        free(curr->data);
        free(curr);
    }
}

int main(int argc, char *argv[]) {
	if (argc !=6)
		exit(1);

    int step = atoi(argv[1]);
	int Level1 = atoi(argv[2]); 
	int Level2 = atoi(argv[3]); 
    int cold = atoi(argv[4]); // =0 measure cold, =1 measure warm 
    int cpu = atoi(argv[5]);  
    printf("%u, %u, %u, %u, %u\n", step, Level1, Level2, cold, cpu);

    pin_cpu((size_t)cpu);

    uint64_t total = (uint64_t)Level1*(uint64_t)Level2;

    uint8_t* default_data = (uint8_t*) malloc(sizeof(uint8_t)*Level1);
    for (int i = 0; i < Level1; i++){
        default_data[i] = (uint8_t)i;
    }

    for (int i = Level2; i >= 0; i--) {
        insertatbegin_level2(default_data, Level1);
    }

    free(default_data);
    
    if (cold==0){

        // fill cache with a big buffer (32MB)
        size_t big_size = 32 * 1024 * 1024; 
        uint8_t* big_buffer = (uint8_t*) malloc(sizeof(uint8_t)*big_size);
        for (size_t j = 0; j < 10; j++){
            for (size_t i = 0; i < big_size; i++){
                big_buffer[i] = (uint8_t)i;
            }
        }
        
        MEM_BARRIER;
        unsigned long long start = rdtsc(), end;
        MEM_BARRIER;
        
        total += searchlist_level2(step, Level1);
         
        MEM_BARRIER;
        end = rdtsc();
        MEM_BARRIER;
        printf("%llu\n", end - start);

        free(big_buffer);
    }else{
        for (size_t i = 0; i < 10; i++) {
            total += searchlist_level2(step, Level1);
        }
      
        MEM_BARRIER;
        unsigned long long start = rdtsc(), end;
        MEM_BARRIER;
      
        for (int i = 0; i < 10; i++) {
            total += searchlist_level2(step, Level1);
        }
      
        MEM_BARRIER;
        end = rdtsc();
        MEM_BARRIER;
        printf("%llu\n", end - start);
      
    }

    delete_ll();
    return 0;
     
}