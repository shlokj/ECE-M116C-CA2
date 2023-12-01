#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 
	for (int i=0; i<VICTIM_SIZE; i++)
		victim[i].valid = false; 
	// Do the same for Victim Cache ...

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;

	// Add stat for Victim cache ... 
	
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	// add your code here
	if (MemR) {
		load(*data, adr, myMem);
	}
	else if (MemW) {
		store(*data, adr, myMem);
	}
	else {
		cerr << "invalid instruction" << endl;
	}

}

void cache::store(int& data, int addr, int* memory) {
	cout << "================= found a store =================" << endl;
	cout << "data: " << data << " addr: " << addr << endl;
	int offset = addr & 0b11; // the last 2 bits
	int idx = (addr >> 2) & 0xF; // the next 4 bits to the left
	int tag = addr >> 6; // everything except the last 6 bits

	if (L1[idx].valid && L1[idx].tag == tag) {
		cout << "is in L1 cache, will update LRU" << endl;
	}
	else {
		cout << "is not in L1 cache" << endl;
	}
	cout << "================= end store =================" << endl;
}

void cache::load(int& data, int addr, int* memory) {
	cout << "================= found a load =================" << endl;
	cout << "data: " << data << " addr: " << addr << endl;
	myStat.accL1++; // unconditionally increase the number of accesses on a load
	int offset = addr & 0b11; // the last 2 bits
	int idx = (addr >> 2) & 0xF; // the next 4 bits to the left
	int tag = addr >> 6; // everything except the last 6 bits
	cout << "idx: " << idx << " tag: " << tag << endl;

	if (L1[idx].valid && L1[idx].tag == tag) {
		cout << "is in L1 cache, will update LRU" << endl;
	}
	else {
		myStat.missL1++;
		cout << "miss in L1 cache, will get from memory" << endl; // we don't actually need to fetch the data because we only need to output the time
		L1[idx].valid = true;
		L1[idx].tag = tag;
		// no need to set lru pos because L1 is DM

	}
	cout << "================= end load =================" << endl;
}

