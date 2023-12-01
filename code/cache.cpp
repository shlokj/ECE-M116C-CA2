#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 
	for (int i=0; i<VICTIM_SIZE; i++) {
		victim[i].valid = false;
		victim[i].lru_position = i;
	}
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
		cout << "is in L1 cache" << endl;
	}
	else {
		cout << "is not in L1 cache" << endl;
		int victim_idx = -1;
		if (isInVictimCache(addr, victim_idx)) { // if true, victim_idx has been set
			// update the LRU positions
			updateVictimCacheLRURanking_promote(victim_idx);
		}
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
		cout << "is in L1 cache" << endl;
	}
	else {
		myStat.missL1++;
		myStat.accVic++;
		cout << "miss in L1 cache, checking victim cache" << endl; // we don't actually need to fetch the data because we only need to output the time

		int victim_idx = -1;
		if (isInVictimCache(addr, victim_idx)) {
			cout << "HIT in victim cache" << endl;
			// promote the block at victim_idx to L1 cache. this may involve removing a block from L1.
			promoteFromVictimToL1(victim_idx);

		}
		else {
			cout << "MISS in victim cache, putting in L1" << endl;
			myStat.missVic++;
			// TODO: if L1 is valid, evict it and put the block in victim cache, and if there is no space in the victim cache, evict the LRU
			if (L1[idx].valid) {
				evictFromL1AndPlaceInVictim(idx);
			}
			L1[idx].valid = true;
			L1[idx].tag = tag;
		}

		// no need to set lru pos because L1 is DM



	}
	cout << "================= end load =================" << endl;
}

bool cache::isInVictimCache(int addr, int& idx) {
	int offset = addr & 0b11;
	int tag = addr >> 2;
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].valid && victim[i].tag == tag) {
			idx = i;
			return true;
		}
	}
	return false;
}

void cache::updateVictimCacheLRURanking_promote(int idx_to_promote) { // the one at idx_to_promote becomes the most recently used, and all others are adjusted
	// everything with a higher lru position is decremented by 1
	int prev_lru_pos = victim[idx_to_promote].lru_position;
	victim[idx_to_promote].lru_position = 3;
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].lru_position > prev_lru_pos) {
			victim[i].lru_position--;
		}
	}
}

void cache::promoteFromVictimToL1(int victim_idx) { // this may swap out a block from L1 if there is one in the L1 spot already
	// this function is called when we find something in victim, so we assume the victim cache block is valid
	cacheBlock victimCacheBlock = victim[victim_idx];
	// tag and index for the L1 format
	// the tag in the victim cache is the L1 tag + the L1 index
	int l1_idx = victimCacheBlock.tag & 0xF; // victimCacheBlock.tag is just addr >> 2
	int l1_tag = victimCacheBlock.tag >> 4; // this is the same as addr >> 2 >> 4, which is addr >> 6
	cacheBlock l1CacheBlock = L1[l1_idx];
	if (l1CacheBlock.valid) { // the L1 block has to be evicted and brought into victim cache - we swap
		int l1_tag_temp = L1[l1_idx].tag;
		L1[l1_idx].tag = l1_tag;
		victim[victim_idx].tag = (l1_tag_temp << 4) | l1_idx;
	}
	else { // just invalidate the promoted block in victim cache and copy it to L1
		victim[victim_idx].valid = false;
		L1[l1_idx].tag = l1_tag;
		L1[l1_idx].valid = true;
	}
}

int cache::firstFreeSpotInVictimCache() {
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (!victim[i].valid) {
			return i;
		}
	}
	return -1;
}

int cache::leastRecentlyUsedIndexInVictimCache() { // assumes that all victim cache blocks are valid/occupied
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].lru_position == 0) {
			return i;
		}
	}
	return -1; // never gets here
}



void cache::evictFromL1AndPlaceInVictim(int idx) {
	cacheBlock to_evict = L1[idx];
	int victim_idx = firstFreeSpotInVictimCache(); // if this is -1, nothing is free
	if (victim_idx == -1) {
		victim_idx = leastRecentlyUsedIndexInVictimCache(); // get the LRU
	}
	int victim_tag = to_evict.tag << 4 | idx; // assemble the victim tag like before
	victim[victim_idx].valid = true;
	victim[victim_idx].tag = victim_tag;
	updateVictimCacheLRURanking_promote(victim_idx);
	L1[idx].valid = false;

}

