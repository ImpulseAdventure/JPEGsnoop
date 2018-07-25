#ifndef NOTE_H
#define NOTE_H

#include <QDialog>

namespace Ui {
class Q_Note;
}

class Q_Note : public QDialog
{
  Q_OBJECT

public:
  explicit Q_Note(QString &msg, QWidget *parent = 0);
  ~Q_Note();
  bool hideStatus();

private:
  Ui::Q_Note *ui;
};

#endif // NOTE_H
