#include "q_decodedetaildlg.h"
#include "ui_q_decodedetaildlg.h"

Q_DecodeDetailDlg::Q_DecodeDetailDlg(QWidget *parent) :
  QDialog(parent), ui(new Ui::Q_DecodeDetailDlg), m_nMcuX(0), m_nMcuY(0), m_nMcuLen(0), m_bEn(false)
{
  ui->setupUi(this);

  connect(this, SIGNAL(accepted()), this, SLOT(accept()));
  connect(this, SIGNAL(rejected()), this, SLOT(reject()));

  ui->leMcuX->setText("1");
}

Q_DecodeDetailDlg::~Q_DecodeDetailDlg()
{
  delete ui;
}

void Q_DecodeDetailDlg::accept()
{

}

void Q_DecodeDetailDlg::reject()
{
  return;
}
