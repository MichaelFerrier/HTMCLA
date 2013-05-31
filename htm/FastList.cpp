#include <crtdbg.h>
#include "MemManager.h"
#include "FastList.h"
#include <algorithm>

extern MemManager mem_manager;

FastListTray::FastListTray()
: pointer(NULL), prev(NULL), next(NULL)
{
}

void FastListTray::Initialize()
{
	pointer = NULL;
	prev = NULL;
	next = NULL;
}

FastList::FastList()
{
	Initialize();
}

void FastList::Initialize()
{
	count = 0;
	iterator_cur = NULL;
	first = NULL;
	last = NULL;
}

void FastList::Clear()
{
	FastListTray *cur;

	// Release all trays in the list
	while ((cur = first) != NULL)
	{
		// Record pointer to next tray in list
		first = cur->next;

		// Release the current tray
		mem_manager.ReleaseObject(cur);
	}

	// Reset variables
	last = NULL;
	count = 0;
	iterator_cur = NULL;
}

void FastList::InsertAtStart(void *_new)
{
	_ASSERT(_new != NULL);

	// Get new tray to represent the new item
	FastListTray *new_tray = (FastListTray*)(mem_manager.GetObject(MOT_FAST_LIST_TRAY));
	new_tray->pointer = _new;

	// Insert the new tray at the start of the list
	new_tray->next = first;
	new_tray->prev = NULL;

	if (first != NULL) {
		first->prev = new_tray;
	}

	first = new_tray;

	if (last == NULL) {
		last = new_tray;
	}

	// Increment the count
	count++;
}

void FastList::InsertAtEnd(void *_new)
{
	_ASSERT(_new != NULL);

	// Get new tray to represent the new item
	FastListTray *new_tray = (FastListTray*)(mem_manager.GetObject(MOT_FAST_LIST_TRAY));
	new_tray->pointer = _new;

	// Insert the new tray at the end of the list
	new_tray->next = NULL;
	new_tray->prev = last;

	if (last != NULL) {
		last->next = new_tray;
	}

	last = new_tray;

	if (first == NULL) {
		first = new_tray;
	}

	// Increment the count
	count++;
}

void* FastList::RemoveFirst()
{
	// If there are no items in the list, return NULL.
	if (first == NULL) {
		return NULL;
	}

	// Record pointer to removed tray
	FastListTray *removed_tray = first;

	// Record pointer to removed object
	void *removed_item = removed_tray->pointer;

	// Advance the first pointer
	first = first->next;

	if (first == NULL) {
		last = NULL;
	} else {
		first->prev = NULL;
	}

	// Release the removd tray
	mem_manager.ReleaseObject(removed_tray);

	// Decrease the count
	count--;

	return removed_item;
}

void FastList::Remove(void *_item_to_remove, bool _multiple)
{
	FastListIter iter(this);
	iter.Reset();

	while (iter.Get() != NULL)
	{
		if (iter.Get() == _item_to_remove) 
		{
			iter.Remove();

			if (!_multiple) {
				return;
			}
		} 
		else 
		{
			iter.Advance();
		}
	}
}

void FastList::TransferContentsTo(FastList &_destination_list)
{
	// The given _destinaton_list must be empty.
	_ASSERT(_destination_list.Count() == 0);

	// Transfer this list's member values to the given _destination_list.
	_destination_list.count = count;
	_destination_list.first = first;
	_destination_list.last  = last;
	_destination_list.iterator_cur = iterator_cur;

	// Clear this list's members.
	count = 0;
	first = last = NULL;
	iterator_cur = NULL;
}

void FastList::CopyContentsTo(FastList &_destination_list)
{
	// Copy the contents of this list to the given _destination_list.
	FastListIter iter(this);
	for (void *cur = iter.Reset(); cur != NULL; cur = iter.Advance()) {
		_destination_list.InsertAtEnd(cur);
	}
}

void *FastList::GetFirst()
{
	return (first == NULL) ? NULL : first->pointer;
}

void *FastList::GetLast()
{
	return (last == NULL) ? NULL : last->pointer;
}

void *FastList::GetByIndex(int _index)
{	
	FastListTray *cur;

	// Iterate through the trays, counting down _index, until _index is 0.
	for (cur = first; _index > 0; _index--)
	{
		// If there is no current tray, return NULL.
		if (cur == NULL) {
			return NULL;
		}

		// Advance to the next tray.
		cur = cur->next;
	}

	// Return the current tray's pointer, or NULL if there is no current tray.
	return (cur == NULL) ? NULL : cur->pointer;
}

bool FastList::IsInList(void *_item)
{
	FastListIter iter(this);
	for (void *cur = iter.Reset(); cur != NULL; cur = iter.Advance())
	{
		if (cur == _item) {
			return true;
		}
	}

	return false;
}

bool FastList::ListsAreIdentical(FastList &_list_A, FastList &_list_B)
{
	// If the two lists don't have an identical number of items, return false.
	if (_list_A.Count() != _list_B.Count()) {
		return false;
	}

	FastListIter iter_A(&_list_A), iter_B(&_list_B);

	iter_A.Reset();
	iter_B.Reset();

	// Iterate through all corresponding items in both lists...
	while (iter_A.Get() != NULL)
	{
		// If the current pair of corresponding items are not identical, return false.
		if (iter_A.Get() != iter_B.Get()) {
			return false;
		}

		// Advance both iterators.
		iter_A.Advance();
		iter_B.Advance();
	}

	// The two lists are identical; return true.
	return true;
}

FastListIter::FastListIter(FastList *_list)
: list(_list)
{
}

FastListIter::FastListIter(FastList &_list)
{
	list = &_list;
}

void* FastListIter::Reset()
{
	_ASSERT(list != NULL);

	// Reset the iterator to point to the first item in the list
	iterator_cur = list->first;

	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastListIter::Reset_Reverse()
{
	_ASSERT(list != NULL);

	// Reset the iterator to point to the last item in the list
	iterator_cur = list->last;

	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastListIter::Prev()
{
	_ASSERT(list != NULL);

	if (iterator_cur == NULL) {
		return NULL;
	}

	// Advance the iterator to the previous item in the list
	iterator_cur = iterator_cur->prev;

  return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastListIter::Advance()
{
	_ASSERT(list != NULL);

	if (iterator_cur == NULL) {
		return NULL;
	}

	// Advance the iterator to the next item in the list
	iterator_cur = iterator_cur->next;

  return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastListIter::Get()
{
	_ASSERT(list != NULL);

	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

void* FastListIter::Duplicate(FastListIter &_original)
{
	_ASSERT(list == _original.list);

	iterator_cur = _original.iterator_cur;

	return (iterator_cur == NULL) ? NULL : iterator_cur->pointer;
}

bool FastListIter::IsFirst()
{
	_ASSERT(list != NULL);

	return (iterator_cur == NULL) ? false : (iterator_cur == list->first);
}

bool FastListIter::IsLast()
{
	_ASSERT(list != NULL);

	return (iterator_cur == NULL) ? false : (iterator_cur == list->last);
}

int FastListIter::GetIndex()
{
	_ASSERT(list != NULL);

	if (iterator_cur == NULL) {
		return -1;
	}

	int index = 0;

	for (FastListTray *cur = list->first; cur != NULL; cur = cur->next)
	{
		// If the current tray is this iterator's current tray, return the current index.
		if (cur == iterator_cur) {
			return index;
		}

		// Increment the index.
		index++;
	}

	_ASSERT(false); // This point should never be reached.
	return -1;
}

void FastListIter::Insert(void *_new)
{
	_ASSERT(list != NULL);

	// If the iterator isn't valid, insert at the end of the list.
	if (iterator_cur == NULL) 
	{
    list->InsertAtEnd(_new);
		return;
	}

	// Insert the given item at the current position of the iterator.

	// Get new tray to represent the new item
	FastListTray *new_tray = (FastListTray*)(mem_manager.GetObject(MOT_FAST_LIST_TRAY));
	new_tray->pointer = _new;

	new_tray->prev = iterator_cur->prev;
	new_tray->next = iterator_cur;

	if (new_tray->prev == NULL) {
		list->first = new_tray; 
	} else {
		new_tray->prev->next = new_tray;
	}

	iterator_cur->prev = new_tray;

	// Increment the count
	list->count++;
}

void FastListIter::Remove()
{
	_ASSERT(list != NULL);

	// If the iterator isn't valid, do nothing -- just return.
	if (iterator_cur == NULL) {
		return;
	}

	if (iterator_cur->prev == NULL) {
		list->first = iterator_cur->next; 
	} else {
		iterator_cur->prev->next = iterator_cur->next;
	}

	if (iterator_cur->next == NULL) {
		list->last = iterator_cur->prev; 
	} else {
		iterator_cur->next->prev = iterator_cur->prev;
	}

	// Record pointer to removed tray
	FastListTray *removed_tray = iterator_cur;

	// Advance the iterator to the next tray
	iterator_cur = iterator_cur->next;

	// Release the removed tray
	mem_manager.ReleaseObject(removed_tray);

	// Decrease the count
	list->count--;
}

void FastListIter::SetList(FastList &_list)
{
	list = &_list;

	if (list != NULL) {
		Reset();
	}
}

void FastListIter::SetList(FastList *_list)
{
	list = _list;

	if (list != NULL) {
		Reset();
	}
}