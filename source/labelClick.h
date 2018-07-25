#ifndef LABELCLICK_H
#define LABELCLICK_H

#include "QWidget"
#include "QLabel"

class QMouseEvent;

class labelClick : public QLabel
{
  Q_OBJECT

public:
  labelClick(QWidget * parent = 0, Qt::WindowFlags f = 0);
  labelClick(QString, QWidget * parent = 0, Qt::WindowFlags f = 0);
  virtual ~labelClick();

protected:
  virtual void  mousePressEvent(QMouseEvent *event);
  virtual void  mouseMoveEvent(QMouseEvent *event);
  virtual void  mouseReleaseEvent(QMouseEvent *event);

signals:
  void clicked(QMouseEvent *);
  void released(QMouseEvent *);
  void moved(QMouseEvent *);

};

#endif // LABELCLICK_H
