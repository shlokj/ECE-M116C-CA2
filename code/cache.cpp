#include "cache.h"

cache::cache()
{
	// initial setup
	
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++) {
		for (int j=0; j<L2_CACHE_WAYS; j++) {
			L2[i][j].valid = false;
			L2[i][j].lru_position = j;
		}
	}
	for (int i=0; i<VICTIM_SIZE; i++) {
		victim[i].valid = false;
		victim[i].lru_position = i;
	}
	// Do the same for Victim Cache ...

	this->myStat.missL1 =0;
	this->myStat.missVic =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accVic =0;
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
	// cout << "================= found a store =================" << endl;
	// cout << "data: " << data << " addr: " << addr << endl;
	int offset = addr & 0b11; // the last 2 bits
	int idx = (addr >> 2) & 0xF; // the next 4 bits to the left
	int tag = addr >> 6; // everything except the last 6 bits

	if (!(L1[idx].valid && L1[idx].tag == tag)) {
		// cout << "is not in L1 cache" << endl;
		int victim_idx = -1;
		if (isInVictimCache(addr, victim_idx)) { // if true, victim_idx has been set
			// update the LRU positions
			updateVictimCacheLRURanking_promote(victim_idx);
		}
		else {
			// check l2 cache and update the LRU ranking
			// cout << "is not in victim cache" << endl;
			int setNum = idx;
			int wayNum = -1;
			if (isInL2Cache(idx, tag, wayNum)) { // if true, wayNum has been set
				updateL2CacheLRURanking_promote(setNum, wayNum);
			}
			else {
				// cout << "is not in L2 cache" << endl;
			}
		}
	}
	// cout << "================= end store =================" << endl;
}

void cache::load(int& data, int addr, int* memory) {
	// cout << "================= found a load =================" << endl;
	// cout << "data: " << data << " addr: " << addr << endl;
	myStat.accL1++; // unconditionally increase the number of accesses on a load

	int offset = addr & 0b11; // the last 2 bits
	int idx = (addr >> 2) & 0xF; // the next 4 bits to the left
	int tag = addr >> 6; // everything except the last 6 bits
	// these values are coincidentally the same even for the L2 cache because of the way it is structured (8-way SA)
	// there are 16 sets in L2, and 8 ways within each set. when we do a lookup, we know which set to look in based on the index
	// but we need to go through each of the 8 ways iteratively

	// cout << "idx: " << idx << " tag: " << tag << endl;

	if (L1[idx].valid && L1[idx].tag == tag) {
		// cout << "HIT in L1 cache" << endl;
	}
	else {
		myStat.missL1++; // miss in L1, increment
		myStat.accVic++; // accessing victim cache
		// cout << "MISS in L1 cache, checking victim cache" << endl; // we don't actually need to fetch the data because we only need to output the time

		int victim_idx = -1;
		if (isInVictimCache(addr, victim_idx)) {
			// cout << "HIT in victim cache" << endl;
			// promote the block at victim_idx to L1 cache. this may involve removing a block from L1.
			promoteFromVictimToL1(victim_idx);
		}
		else {
			myStat.missVic++;
			myStat.accL2++;
			// cout << "MISS in victim cache, checking L2" << endl;
			// if L1 is valid, evict it and put the block in victim cache, and if there is no space in the victim cache, evict the LRU and put that in L2
			int wayNum = -1;
			if (isInL2Cache(idx, tag, wayNum)) { // promote it to L1 and bubble the others down
				// cout << "HIT in L2" << endl;
				// evict a block from L1 and place it in victim. if we evicted something from victim, we need to deal with that and place it in L2
				cacheBlock evicted_from_victim = evictFromL1AndPlaceInVictim(idx); // this function returns the block that was evicted from victim cache
				if (evicted_from_victim.valid) { // there was something in that spot, so now we have to place that in L2
					// getting here also implies that the victim cache was full, because we evicted a block that was valid
					int victim_idx_to_evict = leastRecentlyUsedIndexInVictimCache();
					// place this in L2 and give it the highest priority in its set
					placeInL2(evicted_from_victim);
				}
			}
			else {
				myStat.missL2++;
				// cout << "MISS in L2, retrieving from memory" << endl;
				if (L1[idx].valid) { // if the block in l1 was being used
					cacheBlock evicted_from_victim = evictFromL1AndPlaceInVictim(idx);
					if (evicted_from_victim.valid) { // if the block in victim was being used
						placeInL2(evicted_from_victim);
					}
				}
				L1[idx].valid = true;
				L1[idx].tag = tag;				
			}
		}
	}
	// cout << "================= end load =================" << endl;
}

bool cache::isInVictimCache(int addr, int& idx) {
	int offset = addr & 0b11;
	int tag = addr >> 2;
	// iteratively search for the tag in the victim cache
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].valid && victim[i].tag == tag) {
			idx = i;
			return true;
		}
	}
	return false;
}

bool cache::isInL2Cache(int setNum, int tag, int& wayNum) { // check only the set at setNum. similae to isInVictimCache
	for(int i = 0; i < L2_CACHE_WAYS; ++i) {
		if (L2[setNum][i].valid && L2[setNum][i].tag == tag) {
			wayNum = i;
			return true;
		}
	}
	return false;
}


void cache::updateVictimCacheLRURanking_promote(int idx_to_promote) { // the one at idx_to_promote becomes the most recently used, and all others are adjusted
	// everything with a higher lru position is decremented by 1
	int prev_lru_pos = victim[idx_to_promote].lru_position;
	victim[idx_to_promote].lru_position = 3; // because there are 4 blocks numbered from 0, so 3 is the greatest (MRU)
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].lru_position > prev_lru_pos) {
			victim[i].lru_position--;
		}
	}
}

void cache::updateL2CacheLRURanking_promote(int setNum, int wayNum) { // this block (setNum, wayNum) becomes the most recently used, and all others are adjusted
	// everything with a higher lru position is decremented by 1
	int prev_lru_pos = L2[setNum][wayNum].lru_position;
	L2[setNum][wayNum].lru_position = 7; // because there are 8 blocks numbered from 0, so 7 is the greatest (MRU)
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[setNum][i].lru_position > prev_lru_pos) {
			L2[setNum][i].lru_position--;
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

int cache::firstFreeSpotInVictimCache() { // first invalid (empty) spot
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (!victim[i].valid) {
			return i;
		}
	}
	return -1; // if all blocks are valid/occupied
}

int cache::leastRecentlyUsedIndexInVictimCache() { // assumes that all victim cache blocks are valid/occupied
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (victim[i].lru_position == 0) {
			return i;
		}
	}
	return -1; // never gets here
}

cacheBlock cache::evictFromL1AndPlaceInVictim(int idx) { // returns the cache block that was evicted from victim cache
	cacheBlock to_evict = L1[idx];
	int victim_idx = firstFreeSpotInVictimCache(); // if this is -1, nothing is free
	if (victim_idx == -1) {
		victim_idx = leastRecentlyUsedIndexInVictimCache(); // get the LRU
	}
	cacheBlock evicted_from_victim = victim[victim_idx];
	int victim_tag = to_evict.tag << 4 | idx; // assemble the victim tag like before
	// occupy the victim cache block
	victim[victim_idx].valid = true;
	victim[victim_idx].tag = victim_tag;
	updateVictimCacheLRURanking_promote(victim_idx); // make the block we just placed the most recently used
	L1[idx].valid = false; // free the L1 block
	return evicted_from_victim;
}

int cache::firstFreeSpotInL2BySet(int setNum) {
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (!L2[setNum][i].valid) {
			return i;
		}
	}
	return -1; // if all blocks are valid/occupied	
}

int cache::leastRecentlyUsedIndexInL2BySet(int setNum) { // assumes that all victim cache blocks are valid/occupied
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[setNum][i].lru_position == 0) {
			return i;
		}
	}
	return -1; // never gets here
}

void cache::placeInL2(cacheBlock evicted_from_victim) { // idx for the victim cache
	int l2_set_num = evicted_from_victim.tag & 0xF;
	int l2_tag = evicted_from_victim.tag >> 4;
	int l2_idx_in_set = firstFreeSpotInL2BySet(l2_set_num);
	if (l2_idx_in_set == -1) { // if no free spot in current set
		l2_idx_in_set = leastRecentlyUsedIndexInL2BySet(l2_set_num);
	}
	L2[l2_set_num][l2_idx_in_set].valid = true;
	L2[l2_set_num][l2_idx_in_set].tag = l2_tag;
	updateL2CacheLRURanking_promote(l2_set_num, l2_idx_in_set);
}

