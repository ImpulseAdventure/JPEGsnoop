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
    QMessageBox::warning(this, "JPEGSnoop", "You need to agree to the terms or else click Cancel", QMessageBox::Ok);
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
