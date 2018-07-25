#include <QMouseEvent>
#include "labelClick.h"

//---------------------------------------------------------------------------

labelClick::labelClick(QWidget *_parent, Qt::WindowFlags f) : QLabel(_parent, f)
{
}

//---------------------------------------------------------------------------

labelClick::labelClick(QString s, QWidget *_parent, Qt::WindowFlags f) : QLabel(s, _parent, f)
{
}

//---------------------------------------------------------------------------

labelClick::~labelClick()
{
}

//---------------------------------------------------------------------------

void labelClick::mousePressEvent(QMouseEvent *_event)
{
  QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(_event);
  emit clicked(mouseEvent);
}

//---------------------------------------------------------------------------

void labelClick::mouseMoveEvent(QMouseEvent *_event)
{
  QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(_event);
  emit moved(mouseEvent);
}

//---------------------------------------------------------------------------

void labelClick::mouseReleaseEvent(QMouseEvent *_event)
{
  emit released(static_cast<QMouseEvent *>(_event));
}
