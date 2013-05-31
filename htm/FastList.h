#pragma once

#include "MemObject.h"

class FastListTray
	: public MemObject
{
public:
	FastListTray();

	virtual void Initialize();

	virtual MemObjectType GetMemObjectType() {return MOT_FAST_LIST_TRAY;}

	void *pointer;
	FastListTray *prev, *next;
};

class FastList
{
public:
	FastList();

	void Initialize();
	void Clear();
	int Count() {return count;}

	void InsertAtStart(void *_new);
	void InsertAtEnd(void *_new);

	void* RemoveFirst();
	void Remove(void *_item_to_remove, bool _multiple);

	void TransferContentsTo(FastList &_destination_list);
	void CopyContentsTo(FastList &_destination_list);

	void *GetFirst();
	void *GetLast();
	void *GetByIndex(int _index);
	bool IsInList(void *_item);

	static bool ListsAreIdentical(FastList &_list_A, FastList &_list_B);

	int count;
	FastListTray *first, *last;
	FastListTray *iterator_cur;
};

class FastListIter
{
public:
	FastListIter(FastList *_list = NULL);
	FastListIter(FastList &_list);

	void* Reset();
	void* Reset_Reverse();
	void* Prev();
	void* Advance();
	void* Get();
	void* Duplicate(FastListIter &_original);
	bool IsFirst();
	bool IsLast();
	int GetIndex();
	void Insert(void *_new);
	void Remove();
	void SetList(FastList &_list);
	void SetList(FastList *_list);

	FastList *list;
	FastListTray *iterator_cur;
};
