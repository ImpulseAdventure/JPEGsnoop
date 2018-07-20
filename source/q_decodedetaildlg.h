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
