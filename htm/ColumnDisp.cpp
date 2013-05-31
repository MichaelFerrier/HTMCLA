#include "ColumnDisp.h"
#include "Region.h"
#include "Column.h"
#include "Cell.h"
#include "View.h"
#include "Synapse.h"
#include <QtGui/QBrush>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionGraphicsItem>

ColumnDisp::ColumnDisp(View *_view, DataSpace *_dataSpace, int _x, int _y)
{
	view = _view;
	colX = _x;
	colY = _y;
	dataSpace = _dataSpace;
	region = (dataSpace->GetDataSpaceType() == DATASPACE_TYPE_REGION) ? (Region*)dataSpace : NULL;
	column = (region == NULL) ? NULL : region->GetColumn(colX, colY);
	numItems = (region != NULL) ? region->GetCellsPerCol() : dataSpace->GetNumValues();
	numRows = (int)ceil(sqrt((float)numItems));
	itemMargin = 0.1f;
	itemDist = (1.0f - (2.0f * itemMargin)) / (float)numRows;
	itemSize = itemDist * 0.85f;
	itemSpacer = (itemDist - itemSize) / 2.0f;
	synapseInfoMargin = itemSize * 0.15f;
	markMargin = itemSize * 0.35f;
	colSelected = false;
	selCellIndex = -1;
	imageVal = 0;
	imageVals = NULL;
	imageValCount = 0;
		
	setFlags(ItemIsSelectable | ItemIsMovable);
	setAcceptHoverEvents(true);
}

QRectF ColumnDisp::boundingRect() const
{
    return QRectF(-0.1, -0.1, 1.1, 1.1);
}

void ColumnDisp::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

		// Determine level of detail
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

		// If level of detail warrents it, prepare to draw text.
		if (lod >= 90)
		{
			QFont font("Times", 10);
			font.setStyleStrategy(QFont::ForceOutline);
			painter->setFont(font);
		}

		// Column fill color
		QColor fillColor;
		if (view->GetViewBoost())
		{
			// Use this column's predetermined imageVal.
			int color_val = imageVal * 221;
			fillColor = QColor(221, 221 - color_val, 221 - color_val);
		}
		else
		{
			fillColor = QColor(221,221,221);
		}

		// If viewing connections, and displaying a synapse to/from the column itself, determine column fill color.
		if (((view->GetViewConnectionsIn()) || (view->GetViewConnectionsOut())) && (sel_synapses.size() > 0))
		{
			std::map<int,SynapseRecord>::iterator iter = sel_synapses.find(numItems);
			if (iter != sel_synapses.end())
			{
				fillColor = DetermineSynapseColor(iter->second.permanence, iter->second.connectedPerm); 
			}
		}

		// Save the brush and pen states.
		QBrush b = painter->brush();
		QPen p = painter->pen();

		// If the whole column is selected, first draw a larger black outline.
		if (colSelected) 
		{
			// Set thick black pen, no brush.
			painter->setPen(QPen(QBrush(QColor(0,0,0)), 0.13));
			painter->setBrush(Qt::NoBrush);

			// Draw black outline, slightly thicker than the selection outline.
			painter->drawRoundedRect(QRectF(0.05, 0.05, 0.9, 0.9), 0.25, 0.25);
		}

		// Set the brush to the coumn fill color.
    painter->setBrush(fillColor);

		// If the column is selected, use the selection color for the outline pen. Otherwise, no pen.
		if (colSelected) {
			painter->setPen(QPen(QBrush(QColor(255,0,0)), 0.1));
		} else {
			painter->setPen(Qt::NoPen);
		}

		// Draw the rounded rectangle representing the whole column.
		painter->drawRoundedRect(QRectF(0.05, 0.05, 0.9, 0.9), 0.25, 0.25);

		// Turn off the pen for drawing individual cells in the column.
		painter->setPen(Qt::NoPen);

		// Define colors for inactive, active, and predicted cells.
		QColor colorInactive(255,255,255), colorActive(247,186,23), colorPredicted(255,230,230), colorPredicted1Step(255,251,141);

		// Loop through all cells in this column...
		bool active = false, predicted = false, predicted_1step = false, learning = false;
		int x = 0, y = 0;
		Cell *cell;
		for (int i = 0; i < numItems; i++)
		{
			// If this cell is selected, first draw thicker black outline.
			if (i == selCellIndex) 
			{
				// Set pen to thicker black outline, no brush.
				painter->setPen(QPen(QBrush(QColor(0,0,0)), 0.105));
				painter->setBrush(Qt::NoBrush);

				// Draw thicker black outline.
				painter->drawEllipse(QRectF(itemMargin + itemSpacer + ((float)x * itemDist), itemMargin + itemSpacer + ((float)y * itemDist), itemSize, itemSize));

				// Set the pen to the color of the selection outline.
				painter->setPen(QPen(QBrush(QColor(255,0,0)), 0.08));
			}

			if (view->GetViewActivity())
			{
				// Determine whether the current cell is active.
				active = (region == NULL) ? dataSpace->GetIsActive(colX, colY, i) : region->IsCellActive(colX, colY, i);

				// If not active, determine whether the current cell is predicted to be active in the future.
				if (!active) 
				{
					predicted = (region == NULL) ? false : region->IsCellPredicted(colX, colY, i);
					predicted_1step = (predicted == false) ? false : (region->GetCell(colX, colY, i)->GetNumPredictionSteps() == 1);
				}

				learning = (region == NULL) ? false : region->IsCellLearning(colX, colY, i);

				// Set the brush to the color of this cell, depending on whether it is inactive, active, or predicted, or displaying a synapse permanence.
				painter->setBrush(active ? colorActive : (predicted ? (predicted_1step ? colorPredicted1Step : colorPredicted) : colorInactive));
			}
			else if (view->GetViewReconstruction() || view->GetViewPrediction())
			{
				// Use this cell's predetermined imageVal.
				int color_val = imageVals[i] * 255;
				painter->setBrush(QColor(255 - color_val, 255 - color_val, 255));
			}
			else
			{
				// Set the brush to the color of this cell, depending on whether it is inactive, active, or predicted, or displaying a synapse permanence.
				painter->setBrush(colorInactive);
			}

			// Draw this cell.
			QRectF cellRect(itemMargin + itemSpacer + ((float)x * itemDist), itemMargin + itemSpacer + ((float)y * itemDist), itemSize, itemSize);
			painter->drawEllipse(cellRect);

			// If this cell is selected, turn off the pen.
			if (i == selCellIndex) {
				painter->setPen(Qt::NoPen);
			}

			// If appropriate, display predicted time number (and learning state) on this cell.
			if (view->GetViewActivity() && (lod >= 50) && (predicted || learning))
			{
				cell = column->GetCellByIndex(i);
				_ASSERT(cell != NULL);

				QString str;

				if (learning) 
				{
					painter->setPen(QPen(QBrush(QColor(0,0,0)), 0.1));
					str = "L";
				} 
				else 
				{
					painter->setPen(QPen(QBrush(colorActive), 0.1));
					str.setNum(cell->GetNumPredictionSteps());
				}

				float scale = 50.0f * numRows;
				QTransform original_transform = painter->transform();
				painter->scale(1.0f / scale, 1.0f / scale);
				painter->drawText(QRectF(cellRect.x() * scale, cellRect.y() * scale, cellRect.width() * scale, cellRect.height() * scale), Qt::AlignCenter | Qt::TextDontClip, str);
				painter->setPen(Qt::NoPen);
				painter->setTransform(original_transform);
			}
			
			// If viewing connections, and displaying a synapse to/from the current cell, draw representaton of synapse.
			if (((view->GetViewConnectionsIn()) || (view->GetViewConnectionsOut())) && (sel_synapses.size() > 0))
			{
				std::map<int,SynapseRecord>::iterator iter = sel_synapses.find(i);
				if (iter != sel_synapses.end())
				{
					QRectF synapseRect(itemMargin + itemSpacer + ((float)x * itemDist) + synapseInfoMargin, itemMargin + itemSpacer + ((float)y * itemDist) + synapseInfoMargin, itemSize - (2 * synapseInfoMargin), itemSize - (2 * synapseInfoMargin));

					painter->setBrush(DetermineSynapseColor(iter->second.permanence, iter->second.connectedPerm));
					painter->drawEllipse(synapseRect);

					// If the level of detail is sufficient to warrent it, draw text of synapse info.
					if (lod >= 50)
					{
						QString str;
						str.setNum(iter->second.permanence, 'g', 2);
						float scale = 50.0f * numRows;
						QTransform original_transform = painter->transform();
						painter->scale(1.0f / scale, 1.0f / scale);
						painter->setPen(QPen(QBrush(QColor(255,255,255)), 0.1));
						painter->drawText(QRectF(synapseRect.x() * scale, synapseRect.y() * scale, synapseRect.width() * scale, synapseRect.height() * scale), Qt::AlignCenter | Qt::TextDontClip, str);
						painter->setPen(Qt::NoPen);
						painter->setTransform(original_transform);
					}
				}
			}

			// If viewing marked cells, and this cell is marked, draw a mark on the current cell.
			if (view->GetViewMarkedCells())
			{
				int index = (colY * view->sceneWidth * numItems) + (colX * numItems) + i;
				std::map<int,int>::iterator iter = view->marked_cells.find(index);
				if (iter != view->marked_cells.end())
				{
					QRectF synapseRect(itemMargin + itemSpacer + ((float)x * itemDist) + markMargin, itemMargin + itemSpacer + ((float)y * itemDist) + markMargin, itemSize - (2 * markMargin), itemSize - (2 * markMargin));

					painter->setBrush(QColor(0,0,0));
					painter->drawEllipse(synapseRect);
				}
			}

			// Determine next position.
			if (++x == numRows)
			{
				x = 0;
				y++;
			}
		}

		// If the level of detail is sufficient to warrent it, draw text of column coordinates.
		if (lod >= 90)
		{
			float scale = 175.0f;
			QTransform original_transform = painter->transform();
      painter->scale(1.0f /scale, 1.0f / scale);
			painter->setPen(QPen(QBrush(QColor(0,0,0)), 0.1));
			painter->drawText(QRectF(0.0 * scale, 0.05 * scale, 1.0 * scale, 0.1 * scale), Qt::AlignCenter | Qt::TextDontClip, QString("%1,%2").arg(colX).arg(colY));
			painter->setTransform(original_transform);
		}

		// Restore the brush and pen states.
		painter->setBrush(b);
		painter->setPen(p);
}

int ColumnDisp::DetermineCellIndex(float _x, float _y)
{
	int x = 0, y = 0;
	QRectF rect;
	for (int i = 0; i < numItems; i++)
	{
		rect.setRect(itemMargin + itemSpacer + ((float)x * itemDist), itemMargin + itemSpacer + ((float)y * itemDist), itemSize, itemSize);

		// If the rect for the current cell contains the given coordinates, return this cell index.
		if (rect.contains(_x, _y)) {
			return i;
		}

		// Determine next position.
		if (++x == numRows)
		{
			x = 0;
			y++;
		}
	}

	// The given coords are contained by no cell's rect. Return -1.
	return -1;
}

QColor ColumnDisp::DetermineSynapseColor(float _permanence, float _connectedPerm)
{
	float degree;
	QColor nearColor, farColor;

	if (_permanence < _connectedPerm)
	{
		degree = 1.0f - (_permanence / _connectedPerm);
		nearColor = QColor(170,130,130);
		farColor = QColor(255,0,0);
	}
	else
	{
		degree = (_permanence - _connectedPerm) / (1.0 - _connectedPerm);
		nearColor = QColor(130,170,130);
		farColor = QColor(0,255,0);
	}

	return QColor((1.0 - degree) * nearColor.red() + degree * farColor.red(), 
								(1.0 - degree) * nearColor.green() + degree * farColor.green(),
								(1.0 - degree) * nearColor.blue() + degree * farColor.blue());
}

void ColumnDisp::SetSelection(bool _colSelected, int _selCellIndex)
{
	colSelected = _colSelected;
	selCellIndex = _selCellIndex;

	// Redraw this ColumnDisp.
	update();
}

void ColumnDisp::SelectSynapse(Synapse *_synapse, int _cellIndex)
{
	if ((sel_synapses[_cellIndex].synapse == NULL) || (sel_synapses[_cellIndex].permanence < _synapse->GetPermanence())) {
		sel_synapses[_cellIndex] = SynapseRecord(_synapse, _cellIndex, _synapse->GetPermanence(), _synapse->Params->ConnectedPerm);
	}

	// Redraw ths ColumnDisp.
	update();
}

void ColumnDisp::ClearSelectedSynapses()
{
	sel_synapses.clear();

	// Redraw this ColumnDisp.
	update();
}

void ColumnDisp::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
    update();
}

void ColumnDisp::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        stuff << event->pos();
        update();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void ColumnDisp::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    update();
}

