#include <QDebug>
#include <QtWidgets>

#include "ImgDecode.h"
#include "labelClick.h"
#include "SnoopConfig.h"

#include "Viewer.h"

Q_Viewer::Q_Viewer(CSnoopConfig *pAppConfig, CimgDecode *pImgDecoder, QWidget *_parent, Qt::WindowFlags f) :
  QWidget(_parent, f), m_pAppConfig(pAppConfig), imageLabel(0), scrollArea(0), m_pImgDecoder(pImgDecoder), scaleFactor(0.125), m_nZoomMode(PRV_ZOOM_12)
{
  setGeometry(200, 50, 400, 400);

  QFont font("Courier", 12);

  statusBar = new QStatusBar(this);

  mcuLabel = new QLabel(statusBar);
  mcuLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  mcuLabel->setFont(font);
  statusBar->addPermanentWidget(mcuLabel);

  fileLabel = new QLabel(statusBar);
  fileLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  fileLabel->setFont(font);
  statusBar->addPermanentWidget(fileLabel);

  yccLabel = new QLabel(statusBar);
  yccLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  yccLabel->setFont(font);
  statusBar->addPermanentWidget(yccLabel);

  imageLabel = new labelClick;
  imageLabel->setMouseTracking(true);
  imageLabel->setBackgroundRole(QPalette::Base);
  imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  imageLabel->setScaledContents(true);

  scrollArea = new QScrollArea(this);
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(imageLabel);
  scrollArea->setVisible(false);

  QGridLayout *loGrid = new QGridLayout(this);
  loGrid->addWidget(scrollArea, 0, 0);
  loGrid->addWidget(statusBar, 1, 0);
  loGrid->setContentsMargins(0, 0, 0, 0);
  setLayout(loGrid);

  m_strPreview = "RGB";
  setWindowTitle(m_strPreview + " " + m_strZoom);
  m_strZoom = QString("@ %1%").arg(scaleFactor * 100.0);

  setVisible(false);

  connect(imageLabel, SIGNAL(released(QMouseEvent *)), this, SLOT(mouseReleased(QMouseEvent *)));
  connect(imageLabel, SIGNAL(moved(QMouseEvent *)), this, SLOT(mouseMoved(QMouseEvent *)));
  connect(m_pAppConfig, SIGNAL(scanImageAc(bool)), this, SLOT(scanImageAc(bool)));
}

void Q_Viewer::mouseReleased(QMouseEvent *e)
{
 m_pImgDecoder->SetMarkerBlk(e->pos());
}

void Q_Viewer::mouseMoved(QMouseEvent *e)
{
  uint32_t nByte;
  uint32_t nBit;

  int32_t nY;
  int32_t nCb;
  int32_t nCr;

  QPoint p;
  p = m_pImgDecoder->PixelToMcu(e->pos());
  mcuLabel->setText(QString("MCU [%1,%2]")
                    .arg(p.x(), 4, 10, QChar('0'))
                    .arg(p.y(), 4, 10, QChar('0')));
  m_pImgDecoder->LookupFilePosMcu(e->pos(), nByte, nBit);
  p = m_pImgDecoder->PixelToBlk(e->pos());
  m_pImgDecoder->LookupBlkYCC(e->pos(), nY, nCb, nCr);
  fileLabel->setText(QString("File 0x%1:%2").arg(nByte, 8, 16, QChar('0')).arg(nBit));
  yccLabel->setText(QString("YCC DC=[%1,%2,%3]")
                    .arg(nY, 5, 10, QChar('0'))
                    .arg(nCb, 5, 10, QChar('0'))
                    .arg(nCr, 5, 10, QChar('0')));
}

void Q_Viewer::drawImage()
{
  qDebug() << "Q_Viewer::drawImage";
  setWindowTitle(m_strPreview + " " + m_strIdct + " " + m_strZoom);
  imageLabel->setPixmap(QPixmap::fromImage((*m_pImgDecoder->m_pDibTemp).scaledToWidth(static_cast<int32_t>(m_pImgDecoder->m_pDibTemp->width() * scaleFactor))));
  scrollArea->setVisible(true);
  setVisible(true);
}

void Q_Viewer::zoom(QAction* action)
{
  if(action->data().toInt() == PRV_ZOOM_IN)
  {
    if(m_nZoomMode + 1 < PRV_ZOOMEND)
    {
      m_nZoomMode++;

    }
  }
  else if(action->data().toInt() == PRV_ZOOM_OUT)
  {
    if(m_nZoomMode - 1 > PRV_ZOOMBEGIN)
    {
      m_nZoomMode--;
    }
  }
  else
  {
    m_nZoomMode = action->data().toInt();
  }

  switch(m_nZoomMode)
  {
    case PRV_ZOOM_12:
      scaleFactor = 0.125;
      break;

    case PRV_ZOOM_25:
      scaleFactor = 0.25;
      break;

    case PRV_ZOOM_50:
      scaleFactor = 0.5;
      break;

    case PRV_ZOOM_100:
      scaleFactor = 1.0;
      break;

    case PRV_ZOOM_150:
      scaleFactor = 1.5;
      break;

    case PRV_ZOOM_200:
      scaleFactor = 2.0;
      break;

    case PRV_ZOOM_300:
      scaleFactor = 3.0;
      break;

    case PRV_ZOOM_400:
      scaleFactor = 4.0;
      break;

    case PRV_ZOOM_800:
      scaleFactor = 8.0;
      break;

    default:
      scaleFactor = 1.0;
      break;
  }

  drawImage();
  m_strZoom = QString("@ %1%").arg(scaleFactor * 100.0);
}

void Q_Viewer::setPreviewTitle(QAction *action)
{
  switch(action->data().toInt())
  {
    case PREVIEW_RGB:
      m_strPreview += "RGB";
      break;

    case PREVIEW_YCC:
      m_strPreview += "YCC";
      break;

    case PREVIEW_R:
      m_strPreview += "R";
      break;

    case PREVIEW_G:
      m_strPreview += "G";
      break;

    case PREVIEW_B:
      m_strPreview += "B";
      break;

    case PREVIEW_Y:
      m_strPreview += "Y";
      break;

    case PREVIEW_CB:
      m_strPreview += "Cb";
      break;

    case PREVIEW_CR:
      m_strPreview += "Cr";
      break;

    default:
      m_strPreview += "???";
      break;
  }
}

void Q_Viewer::scanImageAc(bool b)
{
  qDebug() << "scanImageAc" << b;

  if(b)
  {
    m_strIdct = "DC+AC";
  }
  else
  {
    m_strIdct = "DC";
  }
}
