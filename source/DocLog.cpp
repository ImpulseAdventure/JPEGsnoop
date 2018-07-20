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

#include <QDebug>
#include "DocLog.h"

//
// Initialize the log
//
CDocLog::CDocLog(void)
{
  m_pDoc = NULL;
}

CDocLog::CDocLog(QPlainTextEdit *pDoc)
{
  m_pDoc = NULL;
  m_pDoc = pDoc;
}

CDocLog::~CDocLog(void)
{
}

void CDocLog::Clear()
{
  m_pDoc->clear();
}

void CDocLog::setDoc(QPlainTextEdit *pDoc)
{
  m_pDoc = pDoc;
}

void CDocLog::appendToLog(QString strTxt, QColor col)
{
  if(m_bEn)
  {
    QTextCharFormat format;
    format.setForeground(QBrush(col));
    m_pDoc->setCurrentCharFormat(format);
//    m_pDoc->appendPlainText(rTrim(strTxt));
    m_pDoc->appendPlainText(strTxt);
//    qDebug() << strTxt;
  }
}

// Add a basic text line to the log
void CDocLog::AddLine(QString strTxt)
{
  QColor sCol(1, 1, 1);
  appendToLog(strTxt, sCol);
}

// Add a header text line to the log
void CDocLog::AddLineHdr(QString strTxt)
{
  QColor sCol(1, 1, 255);
  appendToLog(strTxt, sCol);
}

// Add a header description text line to the log
void CDocLog::AddLineHdrDesc(QString strTxt)
{
  QColor sCol(32, 32, 255);
  appendToLog(strTxt, sCol);
}

// Add a warning text line to the log
void CDocLog::AddLineWarn(QString strTxt)
{
  QColor sCol(128, 1, 1);
  appendToLog(strTxt, sCol);
}

// Add an error text line to the log
void CDocLog::AddLineErr(QString strTxt)
{
  QColor sCol(255, 1, 1);
  appendToLog(strTxt, sCol);
}

// Add a "good" indicator text line to the log
void CDocLog::AddLineGood(QString strTxt)
{
  QColor sCol(16, 128, 16);
  appendToLog(strTxt, sCol);
}

QString CDocLog::rTrim(const QString& str)
{
  int n = str.size() - 1;

  for (; n >= 0; --n)
  {
    if (!str.at(n).isSpace())
    {
      return str.left(n + 1);
    }
  }

  return "";
}
