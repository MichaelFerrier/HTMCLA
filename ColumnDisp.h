#pragma once

#include <QtGui/QColor>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include "DataSpace.h"
#include <map>

class QStyleOptionGraphicsItem;
class Region;
class Column;
class View;
class Synapse;
class SynapseRecord;

class ColumnDisp : public QGraphicsItem
{
public:
    ColumnDisp(View *_view, DataSpace *_dataSpace, int _x, int _y);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);

		int DetermineCellIndex(float _x, float _y);
		QColor DetermineSynapseColor(float _permanence, float _connectedPerm);

		void SetSelection(bool _colSelected, int _selCellIndex);

		void SelectSynapse(Synapse *_synapse, int _cellIndex);
		void ClearSelectedSynapses();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
		float imageVal;
		float *imageVals;
		int imageValCount;
private:
    int colX, colY, numItems, numRows;
		float itemDist, itemSize, itemSpacer, itemMargin, synapseInfoMargin, markMargin;
		View *view;
		DataSpace *dataSpace;
		Region *region;
		Column *column;
    QColor color;
    QVector<QPointF> stuff;
		bool colSelected;
		int selCellIndex;
		std::map<int,SynapseRecord> sel_synapses;
};

class SynapseRecord
{
public:
	SynapseRecord() {synapse = NULL; cellIndex = -1; permanence = 0; connectedPerm = 0;}
	SynapseRecord(Synapse *_synapse, int _cellIndex, float _permanence, float _connectedPerm) {synapse = _synapse; cellIndex = _cellIndex; permanence = _permanence; connectedPerm = _connectedPerm;}

	Synapse *synapse;
	int cellIndex;
	float permanence, connectedPerm;
};
