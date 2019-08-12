#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>

struct Node {
	long long unsigned int tag;
	bool validity;
	struct Node *next;
};

struct Queue {
	struct Node *front, *tail;
};

//Global Variables: ------------------------
int cacheSize;
char *policy;
int blockSize;
bool direct = false;
bool assoc = false;
bool assocn = false;
int assocNum = 0;
int setNum;
int tag;
int indexBitsNeeded;
int blockOffsetBitsNeeded;
struct Queue **sets;
int memoryReads = 0;
int memoryWrites = 0;
int hit = 0;
int miss = 0;
struct Queue **pSets;
int pMemoryReads = 0;
int pMemoryWrites = 0;
int pHit = 0;
int pMiss = 0;
//Methods: ------------------------
bool isPower(int x, int y);
struct Node *createNode(long long unsigned int tag);
struct Node *createEmptyNode();
struct Queue *createQueue();
void enqueue(struct Queue *queue, long long unsigned int tag);
void enqueueNode(struct Queue *queue, struct Node *newNode);
void nEnqueue(struct Queue *queue, long long unsigned int tag, int sizeOfQueue);
struct Node *dequeueFront(struct Queue *queue);
struct Node *dequeueTail(struct Queue *queue);
void traverseQueue(struct Queue *queue);
int sizeOfQueue(struct Queue *queue);
bool queueIsFull(struct Queue *queue, int assoc);
bool queueContainsTag(struct Queue *queue, long long unsigned int tag);

int main(int argc, char** argv) {	
	/* ----./first <cache size> <associativity> <cache policy> <block size> <tracefile.txt>---- */
	/* ----    0         1              2             3              4             5       ---- */
	 
	// 1.) <cache size> is the total size of the cache in bytes. This number should be a power of 2.
	// 2.) associativity is one of:
	// 	direct - simulate a direct mapped cache.		
	//	assoc - simulate a fully associative cache.
	//	assoc:n - simulate an n way associative cache. n will be a power of 2.
	// 3.) <cache policy>Here is valid cache policy is fifo
	// 4.) <block size> is a power of 2 integer that specifies the cache block in bytes
	// 5.) <tracefile.txt> trace file
	//		
	//printf("\n");
	if(argc != 6) { printf("error\n"); return 0; }
	// --------FIRST ARGUMENT
	if(!isPower(2,atoi(argv[1]))) { printf("error\n"); return 0;}
	else{/*printf("cache size: %s\n",argv[1]);*/ cacheSize = atoi(argv[1]);}	
	// --------SECOND ARGUMENT
	if(argv[2][0] == 'd') { /*printf("associativity = direct\n");*/ direct = true; assocNum = 1;}
	else if(argv[2][0] == 'a') {
		if(argv[2][5] != '\0') {
			int n = argv[2][6]-'0';
				if(argv[2][7] != '\0') {
					//
					if(argv[2][8] != '\0') {
						char num[4] = {argv[2][6], argv[2][7], argv[2][8]};
						int number = atoi(num);
						if(isPower(2,number)) {
						  //	printf("associativity = assoc:n, where n = %d\n",number);
							assocn = true;
							assocNum = number;
						} else {
						  //	printf("associativity is not a power of 2\n");
							return 0;
						}
					} else {
						char num[3] = {argv[2][6], argv[2][7]};
						int number = atoi(num);
						if(isPower(2,number)) {
						  //	printf("associativity = assoc:n, where n = %d\n",number);
							assocn = true;
							assocNum = number;
						} else {
						  //	printf("associativity is not a power of 2\n");
							return 0;
						}
					}
				} else {
					if(isPower(2,n)) {
					  //	printf("associativity = assoc:n, where n = %d\n",n);
						assocn = true;
						assocNum = n;
					} else {
					  //	printf("associativity is not a power of 2\n");
						return 0;
					}
				}
		} else {
		  //	printf("associativity = assoc\n");
			assoc = true;
		}
	}
	// --------THIRD ARGUMENT
	if(strcmp("fifo",argv[3]) == 0){
		policy = "fifo";
		//	printf("policy is FIFO\n");
	} else {		
		policy = "lru";
		//	printf("policy is LRU\n");
	}
	// --------FOURTH ARGUMENT
	if(!isPower(2,atoi(argv[4]))) { printf("error\n"); return 0;}	
	else{/*printf("block size: %s\n",argv[4]);*/ blockSize = atoi(argv[4]);}	

	// --------FIFTH ARGUMENT
	FILE *fp = fopen(argv[5], "r"); /*open file*/ if(fp == NULL) { printf("error\n"); return 0; }
	
	/*-----------------------------------------------------------------------------
 *					MAIN CODE
 * 	-----------------------------------------------------------------------------*/
	long long unsigned int pc;
	char op;
	long long unsigned int address;
	
	if(assocn == true) {
		setNum = cacheSize/(assocNum * blockSize);
		//printf("setNum: %d\n",setNum);
		sets = malloc(sizeof(struct Queue) * setNum);
		for(int i = 0; i < setNum; i++){
			sets[i] = createQueue();
		}

	} else if(assoc == true) {
		assocNum = cacheSize/blockSize;		
		setNum = 1;	
		sets = malloc(sizeof(struct Queue) * setNum);
		sets[0] = createQueue();
	
	} else if(direct == true) {
		setNum = cacheSize/(1 * blockSize);
		sets = malloc(sizeof(struct Queue) * setNum);
		for(int i = 0; i < setNum; i++){
			sets[i] = createQueue();
		}
	}

	while(fscanf(fp, "%llx: %c %llx", &pc,&op,&address) == 3) {
		if(op == 'W') { memoryWrites++; pMemoryWrites++; };
		//printf("pc: %llx \nop: %c \naddress: %llx\n",pc, op, address);
		blockOffsetBitsNeeded = log2(blockSize); //this will give you the lower bits needed for address
		indexBitsNeeded = log2(setNum);	//gives set being inspected
		
		long long unsigned int addressWithoutBlockOffsetBits = address >> blockOffsetBitsNeeded;
		int index = addressWithoutBlockOffsetBits % (long long unsigned int) pow(2, indexBitsNeeded);
		long long unsigned int tag = address >> (blockOffsetBitsNeeded + indexBitsNeeded);		
		//if(assoc == true) {
		//	tag = address >> blockOffsetBitsNeeded;
		//}
		if(assocn == true) { //WORKS
			if(queueContainsTag(sets[index], tag) == false) {
				miss++;
				nEnqueue(sets[index], tag, sizeOfQueue(sets[index]));
				if(op == 'R') { memoryReads++; };
				if(op == 'W') { memoryReads++; };
			} else {
				hit++;
			}
		} 
		else if(assoc == true) { //WORKS
			//assocNum = cacheSize/blockSize;		
			if(queueContainsTag(sets[0], tag) == false) {
					miss++;
					nEnqueue(sets[0], tag, sizeOfQueue(sets[0]));
					if(op == 'R') { memoryReads++; };
					if(op == 'W') { memoryReads++; };
					
			} else {
				hit++;
			}
					
		
		}

		else if(direct == true) { //WORKS
			if(queueContainsTag(sets[index], tag) == false) {
				miss++;
				nEnqueue(sets[index], tag, 1);
				if(op == 'R') { memoryReads++; };
				if(op == 'W') { memoryReads++; };

			} else {
				hit++;
			}
			
		}
	}
	
	fclose(fp);
	/*---------------
	*---------------
 *
 *	WITH PREFETCH
 *
	*---------------
	*---------------
	*/
	
	FILE *fp2 = fopen(argv[5], "r"); if(fp == NULL) { printf("error\n"); return 0; }
	
	long long unsigned int pc2;
	char op2;
	long long unsigned int address2;
	if(assocn == true) { //aka it's assoc:n
		setNum = cacheSize/(assocNum * blockSize);
		pSets = malloc(sizeof(struct Queue) * setNum);
		for(int i = 0; i < setNum; i++){
			pSets[i] = createQueue();
		}
	} else if(assoc == true) {
		assocNum = cacheSize/blockSize;	
		setNum = 1;
		pSets = malloc(sizeof(struct Queue) * setNum);
		pSets[0] = createQueue();
	
	} else if(direct == true) {
		setNum = cacheSize/(1 * blockSize);
		pSets = malloc(sizeof(struct Queue) * setNum);
		for(int i = 0; i < setNum; i++){
			pSets[i] = createQueue();
		}

	}

	while(fscanf(fp2, "%llx: %c %llx", &pc2,&op2,&address2) == 3) {
		//if(op == 'R') { memoryReads++; };
		//if(op == 'W') { pMemoryWrites++; };
		//printf("pc: %llx \nop: %c \naddress: %llx\n",pc, op, address);
		blockOffsetBitsNeeded = log2(blockSize); //this will give you the lower bits needed for address
		indexBitsNeeded = log2(setNum);	//gives set being inspected
			
		//ADDRESS 
		long long unsigned int addressWithoutBlockOffsetBits = address2 >> blockOffsetBitsNeeded;
		int index = addressWithoutBlockOffsetBits % (long long unsigned int) pow(2, indexBitsNeeded);
		long long unsigned int tag = address2 >> (blockOffsetBitsNeeded + indexBitsNeeded);

		//PREFETCH-ADDRESS
		long long unsigned int paddress = address2 + blockSize;	
		long long unsigned int pAddressWithoutBlockOffsetBits = paddress >> blockOffsetBitsNeeded;
		int pindex = pAddressWithoutBlockOffsetBits % (long long unsigned int) pow(2,indexBitsNeeded);
		long long unsigned int ptag = paddress >> (blockOffsetBitsNeeded + indexBitsNeeded);
	
		if(assocn == true) {
			if(queueContainsTag(pSets[index], tag) == false) {
				pMiss++;
				nEnqueue(pSets[index], tag, sizeOfQueue(pSets[index]));
				if(op2 == 'R') { pMemoryReads++; };
				if(op2 == 'W') { pMemoryReads++; };

				//PRE-FETCH: ONLY DO THE FOLLLOWING IF FIRST ADDRESS IS A MISS
				if(queueContainsTag(pSets[pindex], ptag) == false) {
					pMemoryReads++;
					nEnqueue(pSets[pindex], ptag, sizeOfQueue(pSets[pindex]));	
				}

			} else {
				pHit++;
			//	pMemoryReads++;
			}

		}

		else if(assoc == true) {
			//assocNum = cacheSize/blockSize;		
			if(queueContainsTag(pSets[0], tag) == false) {
					pMiss++;
					nEnqueue(pSets[0], tag, sizeOfQueue(pSets[0]));
					if(op == 'R') { pMemoryReads++; };
					if(op == 'W') { pMemoryReads++; };
					//PRE-FETCH:
					if(queueContainsTag(pSets[0], ptag) == false) {
						pMemoryReads++;
						nEnqueue(pSets[0], ptag, sizeOfQueue(pSets[0]));	
						//enqueue(pSets[pindex], ptag);	
					}
			} else {
				pHit++;
			}
		}

		else if(direct == true) {
			if(queueContainsTag(pSets[index], tag) == false) {
				pMiss++;
				nEnqueue(pSets[index], tag, 1);
				if(op == 'R') { pMemoryReads++; };
				if(op == 'W') { pMemoryReads++; };
				//PRE-FETCH:
				if(queueContainsTag(pSets[pindex], ptag) == false) {
					pMemoryReads++;
					nEnqueue(pSets[pindex], ptag, sizeOfQueue(pSets[pindex]));	
					//enqueue(pSets[pindex], ptag);	
				}
			} else {
				pHit++;
			}
		}
	}
	fclose(fp2);

	printf("no-prefetch\nMemory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n",memoryReads,memoryWrites,hit,miss);
	printf("with-prefetch\nMemory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n",pMemoryReads,pMemoryWrites,pHit,pMiss);

	//printf("\nEnd of program----------------\n");	
	
	return 0;
}


bool isPower(int x, int y){ 
	if (x == true) { return (y == true); }
	int power = 1;
	while (power < y) {
		power = power * x;
	}
	if(power == y) { return true; } else { return false; }
}

bool queueIsFull(struct Queue *queue, int assoc) {
	struct Node *ptr = queue->front;
	if(ptr == NULL) {
		return false;
	}
	int count = 0;
	while(ptr != NULL) {
		count++;
		ptr = ptr->next;
	}
	if(count==assoc) {
		return true;
	} else {
		return false;
	}
}

bool queueContainsTag(struct Queue *queue, long long unsigned int tag) {
	struct Node *ptr = queue->front;
	if(ptr == NULL) {
		return false;
	}
	while(ptr != NULL) {
		if(ptr->tag == tag) {
			return true;
		}
		ptr = ptr->next;	
	}
	return false;
}

struct Node *createNode(long long unsigned int tag) {
	struct Node *temp = (struct Node*) malloc(sizeof(struct Node));
	temp->tag = tag;
	temp->validity = false;
	temp->next = NULL;
	return temp;
}

struct Node *createEmptyNode() {
	struct Node *temp = (struct Node*) malloc(sizeof(struct Node));
	temp->tag = 0;
	temp->validity = false;
	temp->next = NULL;
	return temp;
}

struct Queue *createQueue(){
	struct Queue *queue = (struct Queue*) malloc(sizeof(struct Queue));
	queue->tail = NULL;
	queue->front = queue->tail;
	return queue;
}

void nEnqueue(struct Queue *queue, long long unsigned int tag, int sizeOfQueue) {
	
	if(sizeOfQueue == assocNum) {
		if(assocNum == 0) {
			struct Node *newNode = createNode(tag);
			if(queue->tail == NULL) {
				queue->tail = newNode;
				queue->front = queue->tail;
				return;
			}
			queue->tail->next = newNode;
			queue->tail = newNode;	
			return;
		}
		dequeueFront(queue);
		struct Node *newNode = createNode(tag);
		enqueueNode(queue, newNode);
	} else {
		struct Node *newNode = createNode(tag);
		if(queue->tail == NULL) {
		queue->tail = newNode;
		queue->front = queue->tail;
		return;
		}
		queue->tail->next = newNode;
		queue->tail = newNode;
	}
	
}


void enqueue(struct Queue *queue, long long unsigned int tag /*should be long long ...*/){
	struct Node *temp = createNode(tag);
	if(queue->tail == NULL) {
		queue->tail = temp;
		queue->front = queue->tail;
		return;
	}
	queue->tail->next = temp;
	queue->tail = temp;	
}

void enqueueNode(struct Queue *queue, struct Node *newNode){
	if(queue->tail == NULL) {
		queue->tail = newNode;
		queue->front = queue->tail;
		return;
	}
	queue->tail->next = newNode;
	queue->tail = newNode;	
}

//NOTE: the fact that it's returning a struct and not void could possibly cause an issue
struct Node *dequeueFront(struct Queue *queue) {
	if(queue->front == NULL) {
		return NULL;
	}

	struct Node *temp = queue->front; 
	queue->front = queue->front->next;
	
	if(queue->front == NULL) {
		queue->tail = NULL;
	}
	
	return temp;
}

struct Node *dequeueTail(struct Queue *queue) {
	if(queue->front == NULL) {
		return NULL;
	}
	if(queue->front->next == NULL) {
		return NULL;
	}

	struct Node *prev = NULL;; 
	struct Node *ptr = queue->front; 
	
	while(ptr->next != NULL) {
		prev = ptr;
		ptr = ptr->next;
	}	
	prev->next = NULL;
	return ptr;
}

void traverseQueue(struct Queue *queue) {
	struct Node *ptr = queue->front;
	if(ptr == NULL) {
	  //printf("Empty Queue\n");
		return;
	}
	while(ptr != NULL) {
		printf("%llx\n",ptr->tag);
		ptr = ptr->next;
	}
}

int sizeOfQueue(struct Queue *queue) {
	int count = 0;
	struct Node *ptr = queue->front;
	if(ptr == NULL) {
		//printf("Empty Queue\n");
		return 0;
	}
	while(ptr != NULL) {
		//printf("%d\n",ptr->tag);
		count++;
		ptr = ptr->next;
	}
	return count;
}


