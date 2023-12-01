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
		load(*data, adr, myMem);
	}

}

void cache::store(int& data, int addr, int* memory) {
	cout << "found a store" << endl;
	cout << "data: " << data << " addr: " << addr << endl;
}

void cache::load(int& data, int addr, int* memory) {
	cout << "found a store" << endl;
	cout << "data: " << data << " addr: " << addr << endl;
}