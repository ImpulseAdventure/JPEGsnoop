#include <QFileDialog>
#include <QStandardPaths>

#include "SnoopConfig.h"

#include "snoopconfigdialog.h"
#include "ui_snoopconfigdialog.h"

SnoopConfigDialog::SnoopConfigDialog(CSnoopConfig *pAppConfig, QWidget *parent) : QDialog(parent), ui(new Ui::SnoopConfigDialog), m_pAppConfig(pAppConfig)
{
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onSave()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));

  connect(ui->btnBrowse, SIGNAL(released()), this, SLOT(onBrowse()));
  connect(ui->btnDefault, SIGNAL(released()), this, SLOT(onDefault()));
  connect(ui->btnDefault, SIGNAL(released()), this, SLOT(onReset()));

  ui->leDbPath->setText(m_pAppConfig->dbPath());

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
  QFileDialog dialog(this);

  dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
  dialog.setOption(QFileDialog::ShowDirsOnly);

  if(dialog.exec())
  {
    dbPath = dialog.selectedFiles().at(0);
    ui->leDbPath->setText(dbPath);
  }
}

void SnoopConfigDialog::onDefault()
{
  ui->leDbPath->setText(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
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
