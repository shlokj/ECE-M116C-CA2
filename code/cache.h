#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	// int data; // the actual data stored in the cache/memory - don't need this for this project
	bool valid;
	// add more things here if needed
};

struct Stat
{
	int missL1; 
	int missL2; 
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	// Add your Victim cache here ...
	cacheBlock victim[VICTIM_SIZE];
	
	Stat myStat;
	// add more things here
	// the self-documenting function names below describe their purpose
	void store(int& data, int addr, int* memory);
	void load(int& data, int addr, int* memory);
	bool isInVictimCache(int addr, int& idx);
	void updateVictimCacheLRURanking_promote(int prev_idx);
	void promoteFromVictimToL1(int victim_idx);
	cacheBlock evictFromL1AndPlaceInVictim(int idx);
	int firstFreeSpotInVictimCache();
	int leastRecentlyUsedIndexInVictimCache();
	bool isInL2Cache(int idx, int tag, int& wayNum);
	void updateL2CacheLRURanking_promote(int setNum, int wayNum);
	int firstFreeSpotInL2BySet(int setNum);
	int leastRecentlyUsedIndexInL2BySet(int setNum);
	void placeInL2(cacheBlock evicted_from_victim);
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	// add more functions here ...	
	Stat getStat() { return myStat; };
};


