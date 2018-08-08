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

#ifndef Q_DECODEDETAILDLG_H
#define Q_DECODEDETAILDLG_H

#include <QDialog>

namespace Ui {
class Q_DecodeDetailDlg;
}

class Q_DecodeDetailDlg : public QDialog
{
  Q_OBJECT

public:
  explicit Q_DecodeDetailDlg(QWidget *parent = 0);
  ~Q_DecodeDetailDlg();

private slots:
  void accept();
  void reject();

private:
  Ui::Q_DecodeDetailDlg *ui;

  uint32_t			m_nMcuX;
  uint32_t			m_nMcuY;
  uint32_t			m_nMcuLen;
  bool			m_bEn;

  uint32_t			m_nLoadMcuX;
  uint32_t			m_nLoadMcuY;
  uint32_t			m_nLoadMcuLen;

};

#endif // Q_DECODEDETAILDLG_H
