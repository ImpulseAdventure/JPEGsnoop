#include "JPEGsnoop.h"

#include "snoopconfigdialog.h"
#include "ui_snoopconfigdialog.h"

SnoopConfigDialog::SnoopConfigDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SnoopConfigDialog)
{
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onSave()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));

  connect(ui->btnBrowse, SIGNAL(released()), this, SLOT(onBrowse()));
  connect(ui->btnDefault, SIGNAL(released()), this, SLOT(onDefault()));
  connect(ui->btnDefault, SIGNAL(released()), this, SLOT(onReset()));

  dbPath = m_pAppConfig->dbPath();
  ui->leDbPath->setText(dbPath);

  bAutoUpdate = m_pAppConfig->swUpdateEnabled();
  ui->cbAutoUpdate->setChecked(bAutoUpdate);

  autoUpdateDays = m_pAppConfig->autoUpdateDays();
  ui->leUpdateDays->setText(QString("%1").arg(autoUpdateDays));
  bAutoReprocess = m_pAppConfig->autoReprocess();
  ui->cbAutoReprocess->setChecked(bAutoReprocess);
  bOnlineDb = m_pAppConfig->dbSubmitNet();
  ui->cbOblineDb->setChecked(bOnlineDb);
  maxDecodeErrors = m_pAppConfig->maxDecodeError();
  ui->leMaxErrors->setText(QString("%1").arg(maxDecodeErrors));
}

SnoopConfigDialog::~SnoopConfigDialog()
{
  delete ui;
}

void SnoopConfigDialog::onBrowse()
{

}

void SnoopConfigDialog::onDefault()
{

}

void SnoopConfigDialog::onReset()
{

}

void SnoopConfigDialog::onSave()
{

}

void SnoopConfigDialog::onCancel()
{
  close();
}
