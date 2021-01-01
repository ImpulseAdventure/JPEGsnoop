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

#ifndef Q_VIEWER_H
#define Q_VIEWER_H

#include <QPainter>
#include <QPaintEvent>
#include <QString>
#include <QWidget>

#include "snoop.h"

class CimgDecode;
class QLabel;
class labelClick;
class QScrollArea;
class QStatusBar;
class CSnoopConfig;

class Q_Viewer : public QWidget
{
  Q_OBJECT
public:
  explicit Q_Viewer(CSnoopConfig *pAppConfig, CimgDecode *pImgDecoder, QWidget *parent = nullptr, Qt::WindowFlags f = 0);
  void drawImage();

public slots:
  void mouseReleased(QMouseEvent *);
  void mouseMoved(QMouseEvent *);
  void zoom(QAction *action);
  void setPreviewTitle(QAction *preview);
  void scanImageAc(bool);

private:
  CSnoopConfig *m_pAppConfig;
  CimgDecode *m_pImgDecoder;

  labelClick *imageLabel;
  QScrollArea *scrollArea;
  double scaleFactor;
  int32_t m_nZoomMode;

  QString m_strPreview;
  QString m_strZoom;
  QString m_strIdct;

  QStatusBar *statusBar;
  QLabel *mcuLabel;
  QLabel *fileLabel;
  QLabel *yccLabel;
};

#endif // Q_VIEWER_H
