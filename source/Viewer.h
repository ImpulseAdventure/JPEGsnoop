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
