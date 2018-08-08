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

#ifndef SNOOPCONFIGDIALOG_H
#define SNOOPCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class SnoopConfigDialog;
}

class CSnoopConfig;

class SnoopConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SnoopConfigDialog(CSnoopConfig *pAppConfig, QWidget *parent = 0);
  ~SnoopConfigDialog();

private slots:
  void onBrowse();
  void onDefault();
  void onReset();
  void onSave();
  void onCancel();

private:
  SnoopConfigDialog &operator = (const SnoopConfigDialog&);
  SnoopConfigDialog(SnoopConfigDialog&);
  
  Ui::SnoopConfigDialog *ui;

  CSnoopConfig *m_pAppConfig;

  QString dbPath;
  bool bAutoUpdate;
  bool bAutoReprocess;
  bool bOnlineDb;

  int32_t autoUpdateDays;
  int32_t maxDecodeErrors;

};

#endif // SNOOPCONFIGDIALOG_H
