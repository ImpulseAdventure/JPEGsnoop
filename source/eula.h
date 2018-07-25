#ifndef EULA_H
#define EULA_H

#include <QDialog>

namespace Ui {
class Q_Eula;
}

class Q_Eula : public QDialog
{
  Q_OBJECT

public:
  explicit Q_Eula(QWidget *parent = 0);
  ~Q_Eula();

  bool updateStatus();

private slots:
  void okClicked();

private:
  Ui::Q_Eula *ui;
};

#endif // EULA_H
