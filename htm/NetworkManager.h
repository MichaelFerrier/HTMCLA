#pragma once
#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>
#include <list>
#include "Region.h"
#include "InputSpace.h"
#include "Classifier.h"

const int INPUTSPACE_MAX_SIZE = 1000000;
const int INPUTSPACE_MAX_NUM_VALUES = 1000;

const int STRING_BUFFER_LEN = 100;
const int LINE_BUFFER_LEN = 10000;

class NetworkManager
{
public:
	NetworkManager(void);
	~NetworkManager(void);

	void ClearNetwork();

	bool LoadNetwork(QString &_filename, QXmlStreamReader &_xml, QString &_error_msg);
	bool ParseSynapseParams(QXmlStreamReader &_xml, SynapseParameters &_synapseParams, QString &_error_msg);
	Region *ParseRegion(QXmlStreamReader &_xml, SynapseParameters _proximalSynapseParams, SynapseParameters _distalSynapseParams, QString &_error_msg);
	InputSpace *ParseInputSpace(QXmlStreamReader &_xml, QString &_error_msg);
	PatternInfo *ParsePattern(QXmlStreamReader &_xml, QString &_error_msg, int _width, int _height);
	Classifier *ParseClassifier(QXmlStreamReader &_xml, QString &_error_msg);
	void ReadItem(char* &_linePos, char _separator, char *_stringBuffer, int _stringBufferLen);

	void ClearData();
	void ClearData_ProximalSegment(Segment *_segment);
	void ClearData_DistalSegment(Segment *_segment);

	bool LoadData(QString &_filename, QFile *_file, QString &_error_msg);
	bool LoadData_ProximalSegment(QDataStream &_stream, Region *_region, Segment *_segment, QString &_error_msg);
	bool LoadData_DistalSegment(QDataStream &_stream, Region *_region, Segment *_segment, QString &_error_msg);

	bool SaveData(QString &_filename, QFile *_file, QString &_error_msg);
	bool SaveData_ProximalSegment(QDataStream &_stream, Segment *_segment);
	bool SaveData_DistalSegment(QDataStream &_stream, Segment *_segment);

	const QString &GetFilename() {return filename;}
	int GetTime() {return time;}
	bool IsNetworkLoaded() {return networkLoaded;}

	DataSpace *GetDataSpace(const QString _id);
	InputSpace *GetInputSpace(const QString _id);
	Region *GetRegion(const QString _id);

	void Step();

	void WriteToLog(QString _text);

	std::vector<InputSpace*> inputSpaces;
	std::vector<Region*> regions;
	std::vector<Classifier*> classifiers;

	QString filename;
	int time;
	bool networkLoaded;
};

