#ifndef __HASHTABLE_H_INCLUDED__
#define __HASHTABLE_H_INCLUDED__

#include <stdlib.h>

#include "structs.hpp"

namespace HashTable {
	bool allocate(hashtable& h);
    bool reallocateRow(hashtable& h, int row);
	void deallocate(hashtable& h);
	unsigned int getHash(int* ptr);
    bool match(hashtable& h, database& d, clause& c, unsigned int hash, long& offset);
    bool insertOffset(hashtable& h, unsigned int hash, long offset);
    bool removeOffset(hashtable& h, unsigned int hash, long offset) ;
}

#endif
