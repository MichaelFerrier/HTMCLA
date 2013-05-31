#include <crtdbg.h>
#include "MemManager.h"
#include "FastHash.h"

extern MemManager mem_manager;

// For recording boolean values in a FastHashTable
void* FastHashTable::BOOL_FALSE = (void*)(0x00000001);
void* FastHashTable::BOOL_TRUE = (void*)(0x00000002);

FastHashTray::FastHashTray()
: key(0), pointer(NULL), next(NULL)
{
}

void FastHashTray::Initialize()
{
	key = 0;
	pointer = NULL;
	next = NULL;
}

FastHashTable::FastHashTable()
: count(0), iterator_cur(NULL), iterator_bucket_index(NUM_BUCKETS)
{
	// Initialize all bucket pointers to NULL
	for (int i = 0; i < NUM_BUCKETS; i++) {
		buckets[i] = NULL;
	}
}

void FastHashTable::Insert(int _key, void *_object)
{
	int bucket_index = _key & KEY_MASK;

	// Get pointer to first tray pointer for this bucket.
	FastHashTray **tray_ptr = &(buckets[bucket_index]);

	// Iterate through all trays in this bucket, up to the point where the given _object should be inserted.
	// If the given _object is already in the table, return.
	while (((*tray_ptr) != NULL) && ((*tray_ptr)->key <= _key))
	{
		if (((*tray_ptr)->key == _key) && ((*tray_ptr)->pointer == _object))
		{
			// The given _object is already in the table, under the given _key. Do not add it, just return.
			return;
		}

		// Advance to the next tray
		tray_ptr = &((*tray_ptr)->next);
	}

	// Get a new tray to represent the given object
	FastHashTray *new_tray = (FastHashTray*)(mem_manager.GetObject(MOT_FAST_HASH_TRAY));

	// Initialize the new tray
	new_tray->key = _key;
	new_tray->pointer = _object;

	// Insert the new tray in the bucket
	new_tray->next = (*tray_ptr);
	(*tray_ptr) = new_tray;

	// Increment the count
	count++;
}

void FastHashTable::Remove(int _key, void *_object)
{
	int bucket_index = _key & KEY_MASK;
	FastHashTray *tray_to_remove;

	// Get pointer to first tray pointer for this bucket.
	FastHashTray **tray_ptr = &(buckets[bucket_index]);

	// Iterate through all trays in this bucket, up to the point where the given _object is found.
	while (((*tray_ptr) != NULL) && ((*tray_ptr)->key <= _key))
	{
		if (((*tray_ptr)->key == _key) && ((*tray_ptr)->pointer == _object))
		{
			// Record pointer to this tray to be removed.
			tray_to_remove = (*tray_ptr);

			// Set the pointer that points to the tray to remove, to point to the next tray instead.
			(*tray_ptr) = (*tray_ptr)->next;

			// Release the tray that's been removed.
			mem_manager.ReleaseObject(tray_to_remove);

			// Decrement the count
			count--;

			// The objecthas been removed; return.
			return;
		}

		// Advance to the next tray
		tray_ptr = &((*tray_ptr)->next);
	}
}

void FastHashTable::Remove(int _key)
{
	int bucket_index = _key & KEY_MASK;
	FastHashTray *tray_to_remove;

	// Get pointer to first tray pointer for this bucket.
	FastHashTray **tray_ptr = &(buckets[bucket_index]);

	// Iterate through all trays in this bucket, up to the point where the given _object is found.
	while (((*tray_ptr) != NULL) && ((*tray_ptr)->key < _key))
	{
		// Advance to the next tray
		tray_ptr = &((*tray_ptr)->next);
	}

	while (((*tray_ptr) != NULL) && ((*tray_ptr)->key == _key))
	{
		// Record pointer to this tray to be removed.
		tray_to_remove = (*tray_ptr);

		// Set the pointer that points to the tray to remove, to point to the next tray instead.
		(*tray_ptr) = (*tray_ptr)->next;

		// Release the tray that's been removed.
		mem_manager.ReleaseObject(tray_to_remove);

		// Decrement the count
		count--;
	}
}

void *FastHashTable::Get(int _key)
{
	// Determine the bucket index for the given _key
	int bucket_index = _key & KEY_MASK;

  // Iterate through all trays in the given _key's bucket, up to and including those with the given _key.
	for (FastHashTray *cur_tray = buckets[bucket_index]; ((cur_tray != NULL) && (cur_tray->key <= _key)); cur_tray = cur_tray->next)
	{
		if (cur_tray->key == _key)
		{
			// The first tray with the given _key has been found. Return its pointer.
			return cur_tray->pointer;
		}
	}

	// There is no tray with the given _key. Return NULL.
	return NULL;
}

void FastHashTable::Clear()
{
	FastHashTray *cur_tray, *next_tray;

	// If the table is already empty, just return.
	if (count == 0) {
		return;
	}

	for (int i = 0; i < NUM_BUCKETS; i++) 
	{
		// Release all trays in the current bucket
		cur_tray = buckets[i];
		while (cur_tray != NULL)
		{
			next_tray = cur_tray->next;
			mem_manager.ReleaseObject(cur_tray);
			cur_tray = next_tray;
		}

		// Set the current bucket pointer to NULL
		buckets[i] = NULL;
	}

	// Reset the count
	count = 0;

	// Reset the iterator
	iterator_cur = NULL;
	iterator_bucket_index = NUM_BUCKETS;
}

void* FastHashTable::Iterator_Reset()
{
	// Reset the iterator
	iterator_bucket_index = 0;
	iterator_cur = NULL;

	// Advance to the first object
	return Iterator_Advance();
}

void* FastHashTable::Iterator_Set(int _key)
{
	// Determine the bucket index for the given _key
	int bucket_index = _key & KEY_MASK;

	// Set the iterator_cur to the first tray in the given _key's bucket
	iterator_cur = buckets[bucket_index];

	// Advance to the first tray for the given _key, if one exists.
	while ((iterator_cur != NULL) && (iterator_cur->key < _key)) {
		iterator_cur = iterator_cur->next;
	}

	if ((iterator_cur == NULL) || (iterator_cur->key > _key))
	{
		// There are no trays for the given key; the iterator is invalid. Return NULL.
		iterator_bucket_index = NUM_BUCKETS;
		iterator_cur = NULL;
		return NULL;
	}

	// A tray for the given _key has been found. The iterator is valid.
	iterator_bucket_index = bucket_index;
	
  // Return the current object, the first object with the given _key.
	return iterator_cur->pointer;
}

void* FastHashTable::Iterator_Advance()
{
	// If there is a current tray...
	if (iterator_cur != NULL) 
	{
		// Advance to the next tray in this bucket
		iterator_cur = iterator_cur->next;

		if (iterator_cur == NULL)
		{
			// There is no next tray in this bucket, so advance to the next bucket.
			iterator_bucket_index++;
		}
	}

	// If there is no current tray...
	if (iterator_cur == NULL)
	{
		// Find the current or next bucket that has any trays in it.
		while ((iterator_bucket_index < NUM_BUCKETS) && (buckets[iterator_bucket_index] == NULL))
		{
			// There is no tray in this bucket, so advance to the next bucket.
			iterator_bucket_index++;
		}

		// If a bucket with trays has been found, set the iterator_cur to that bucket's first tray. 
		if (iterator_bucket_index < NUM_BUCKETS)
		{
			iterator_cur = buckets[iterator_bucket_index];
		}
	}

	// Return the current object
	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastHashTable::Iterator_AdvanceForKey()
{
	_ASSERT(iterator_cur != NULL);

	if ((iterator_cur->next != NULL) && (iterator_cur->next->key == iterator_cur->key))
	{
		iterator_cur = iterator_cur->next;
		return iterator_cur->pointer;
	}
	else
	{
		// There are no more trays for the current key; the iterator is invalid. Return NULL.
		iterator_bucket_index = NUM_BUCKETS;
		iterator_cur = NULL;
		return NULL;
	}
}

void* FastHashTable::Iterator_Get()
{
	// Return the current object
	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}