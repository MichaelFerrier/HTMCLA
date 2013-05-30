#pragma once

#include "MemObject.h"

const int NUM_BUCKETS = 256;
const int KEY_MASK = 0x000000FF;

class FastHashTray
	: public MemObject
{
public:
	FastHashTray();

	virtual void Initialize();

	virtual MemObjectType GetMemObjectType() {return MOT_FAST_HASH_TRAY;}

	int key;
	void *pointer;
	FastHashTray *next;
};

class FastHashTable
{
public:
	FastHashTable();

	void Insert(int _key, void *_object);
	void Remove(int _key, void *_object);
	void Remove(int _key);
	void *Get(int _key);
	void Clear();
	unsigned int Count() {return count;}

	void* Iterator_Reset();
	void* Iterator_Reset_Reverse();
	void* Iterator_Set(int _key);
	void* Iterator_Advance();
	void* Iterator_AdvanceForKey();
	void* Iterator_Get();

	int count;
	FastHashTray* buckets[NUM_BUCKETS];
	FastHashTray *iterator_cur;
	int iterator_bucket_index;

	static void *BOOL_TRUE, *BOOL_FALSE;
};

