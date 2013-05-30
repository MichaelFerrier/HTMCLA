#include "Classifier.h"

Classifier::Classifier(QString &_id, int _numItems, QString _regionID, QString _inputspaceID, QStringList &_labels)
	: DataSpace(_id), labels(_labels), regionID(_regionID), inputspaceID(_inputspaceID), numItems(_numItems)
{
}

int Classifier::GetSizeX()
{
	return 1;
}

int Classifier::GetSizeY()
{
	return numItems;
}

int Classifier::GetNumValues()
{
	return 1;
}

int Classifier::GetHypercolumnDiameter() 
{
	return 1;
}

bool Classifier::GetIsActive(int _x, int _y, int _index)
{
	_ASSERT(_x == 0);
	_ASSERT((_y >= 0) && (_y < numItems));
	_ASSERT(_index == 0);

	// TEMP
	return false;
}

