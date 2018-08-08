// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2018 - Calvin Hass
// http://www.impulseadventure.com/photo/jpeg-snoop.html
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

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
