#include "note.h"
#include "ui_note.h"

Q_Note::Q_Note(QString &msg, QWidget *parent) : QDialog(parent),
  ui(new Ui::Q_Note)
{
  ui->setupUi(this);

  ui->lbCoachMsg->setText(msg);
}

Q_Note::~Q_Note()
{
  delete ui;
}

bool Q_Note::hideStatus()
{
  return ui->cbHideMsg->isChecked();
}
