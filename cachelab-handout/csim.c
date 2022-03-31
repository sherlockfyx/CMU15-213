#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

//cache_line 缓存行
typedef struct{
	int valid_bits;			//有效位
	unsigned  tag;
	int stamp;				//0 means recently used;
} cache_line;

char* path = NULL;
// s is set ,E  is line, each line have 2^b bits ,S is 2^s set
int s, E, b, S;           	
int hit = 0, miss = 0, evic = 0;

cache_line** cache = NULL;	//二维数组


//cache_line cache[S][E]
void init(){
	cache = (cache_line* *)malloc(sizeof(cache_line*) * S);             //malloc cache[S][E]
	for(int i = 0; i < S; i++) {
		*(cache + i) = (cache_line*)malloc(sizeof(cache_line) * E);
	}
		
	for(int i = 0; i < S; i++){
		for(int j = 0; j < E; j++){
			cache[i][j].valid_bits = 0;      // set all valid_bits is zero
			cache[i][j].tag = 0xffffffff;    //no address
			cache[i][j].stamp = 0;           //time is 0;		
		}	
	}
}

void update(unsigned addr){
	unsigned s_addr =(addr>>b) & ((0xffffffff)>>(32 - s));//set`s index
	unsigned t_addr = addr>>(s + b);                     //tag`s index            
	//first check is hit
	for(int i = 0; i < E; i++){
		if(cache[s_addr][i].tag == t_addr && cache[s_addr][i].valid_bits == 1){
			cache[s_addr][i].stamp = 0;      //now ,this is used
			hit++;
			return;
		}
	}

	for(int i = 0; i < E; i++){
		if(cache[s_addr][i].valid_bits == 0) { // has free cache_line? miss and insert
			cache[s_addr][i].tag = t_addr;
			cache[s_addr][i].valid_bits = 1;
			cache[s_addr][i].stamp = 0;       //now ,this is load
			miss++;
			return;
		}
	}

	int max_stamp = 0;
	int max_i;
	for(int i = 0; i < E; i++){
		if(cache[s_addr][i].stamp > max_stamp){
			max_stamp = cache[s_addr][i].stamp;
			max_i = i;
		}
	}

	evic++;
	miss++;
	cache[s_addr][max_i].tag = t_addr;
	cache[s_addr][max_i].stamp = 0;	
}

void time(){
	for(int i = 0; i < S; i++){
		for(int j = 0;j < E; j++){
			if(cache[i][j].valid_bits == 1)
				cache[i][j].stamp++;		
		}	
	}
}


int main(int argc, char *argv[])
{   
	int opt;         
	while((opt = getopt(argc,argv,"s:E:b:t:")) !=-1){           //parse command line arguments
		switch(opt){
		case 's':
			s=atoi(optarg);
			break;
		case 'E':
			E=atoi(optarg);
			break;
		case 'b':
			b=atoi(optarg);
			break;
		case 't':
			path = optarg;
			break;
		}
	}
	S = 1<<s;
	init();
	FILE* file=fopen(path, "r");
	if(file == NULL){     // read trace file
		printf("Open file wrong!");		
		exit(-1);
	}
	char op;
	unsigned addr;
	int size;	
	while(fscanf(file, " %c %x,%d", &op, &addr, &size) > 0){
		switch(op){
			case 'L':
				update(addr);
				break;
			case 'M':
				update(addr);
			case 'S':
				update(addr);
				break;
		}
		time();
	}
	for(int i = 0; i < S; i++)                  //free cache[S][E]
		free(*(cache + i));
	free(cache);
	fclose(file);	                //close file	
    
	printSummary(hit,miss,evic);
    return 0;
}
