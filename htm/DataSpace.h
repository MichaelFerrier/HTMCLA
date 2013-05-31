#pragma once
#include <QtCore/qstring.h>

typedef int DataSpaceType;
const DataSpaceType DATASPACE_TYPE_INPUTSPACE = 0;
const DataSpaceType DATASPACE_TYPE_REGION     = 1;
const DataSpaceType DATASPACE_TYPE_CLASSIFIER = 2;

class DataSpace
{
public:

	DataSpace(QString &_id) {id = _id; index = -1;}

	const QString &GetID() {return id;}
	void SetID(QString &_id) {id = _id;}

	void SetIndex(int _index) {index = _index;}
	int GetIndex() {return index;}

	virtual DataSpaceType GetDataSpaceType()=0; 

	virtual int GetSizeX()=0;
	virtual int GetSizeY()=0;
	virtual int GetNumValues()=0;
	virtual int GetHypercolumnDiameter()=0;


	virtual bool GetIsActive(int _x, int _y, int _index)=0;

	QString id;
	int index;
};

