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

#include <QMessageBox>

#include "eula.h"
#include "ui_eula.h"

Q_Eula::Q_Eula(QWidget *parent) : QDialog(parent), ui(new Ui::Q_Eula)
{
  ui->setupUi(this);

  QFile eula(":/eula.txt");

  eula.open(QFile::ReadOnly);

  ui->plainTextEdit->setPlainText(QString(eula.readAll()));

  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(okClicked()));
}

Q_Eula::~Q_Eula()
{
  delete ui;
}

void Q_Eula::okClicked()
{
  if(!ui->cbAgreed->isChecked())
  {
    QMessageBox::warning(this, "JPEGsnoop", "You need to agree to the terms or else click Cancel", QMessageBox::Ok);
  }
  else
  {
    done(1);
  }
}

bool Q_Eula::updateStatus()
{
  return ui->cbUpdate->isChecked();
}
