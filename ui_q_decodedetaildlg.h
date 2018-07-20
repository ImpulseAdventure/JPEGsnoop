/********************************************************************************
** Form generated from reading UI file 'q_decodedetaildlg.ui'
**
** Created by: Qt User Interface Compiler version 5.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_Q_DECODEDETAILDLG_H
#define UI_Q_DECODEDETAILDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Q_DecodeDetailDlg
{
public:
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBox;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *leMcuX;
    QSpacerItem *horizontalSpacer;
    QLabel *label_2;
    QLineEdit *leMcuY;
    QGroupBox *groupBox_2;
    QWidget *widget1;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QLineEdit *leNumMcus;
    QCheckBox *cbEnableDec;
    QLabel *label_4;
    QWidget *widget2;
    QVBoxLayout *verticalLayout;
    QLabel *label_5;
    QPushButton *pbLoadCoords;

    void setupUi(QDialog *Q_DecodeDetailDlg)
    {
        if (Q_DecodeDetailDlg->objectName().isEmpty())
            Q_DecodeDetailDlg->setObjectName(QStringLiteral("Q_DecodeDetailDlg"));
        Q_DecodeDetailDlg->resize(462, 212);
        Q_DecodeDetailDlg->setModal(true);
        buttonBox = new QDialogButtonBox(Q_DecodeDetailDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(370, 10, 81, 61));
        buttonBox->setOrientation(Qt::Vertical);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        groupBox = new QGroupBox(Q_DecodeDetailDlg);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(10, 10, 301, 61));
        groupBox->setStyleSheet(QLatin1String("QGroupBox {\n"
"    border: 1px solid gray;\n"
"    border-radius: 9px;\n"
"    margin-top: 0.5em;\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        groupBox->setFlat(true);
        widget = new QWidget(groupBox);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(10, 30, 270, 23));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(label);

        leMcuX = new QLineEdit(widget);
        leMcuX->setObjectName(QStringLiteral("leMcuX"));
        leMcuX->setMinimumSize(QSize(61, 0));
        leMcuX->setMaximumSize(QSize(30, 21));

        horizontalLayout->addWidget(leMcuX);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        label_2 = new QLabel(widget);
        label_2->setObjectName(QStringLiteral("label_2"));
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(label_2);

        leMcuY = new QLineEdit(widget);
        leMcuY->setObjectName(QStringLiteral("leMcuY"));
        leMcuY->setMinimumSize(QSize(61, 0));
        leMcuY->setMaximumSize(QSize(61, 21));

        horizontalLayout->addWidget(leMcuY);

        groupBox_2 = new QGroupBox(Q_DecodeDetailDlg);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setGeometry(QRect(10, 80, 301, 61));
        groupBox_2->setStyleSheet(QLatin1String("QGroupBox {\n"
"    border: 1px solid gray;\n"
"    border-radius: 9px;\n"
"    margin-top: 0.5em;\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 3px 0 3px;\n"
"}"));
        groupBox_2->setFlat(true);
        widget1 = new QWidget(groupBox_2);
        widget1->setObjectName(QStringLiteral("widget1"));
        widget1->setGeometry(QRect(10, 30, 132, 23));
        horizontalLayout_2 = new QHBoxLayout(widget1);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_3 = new QLabel(widget1);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout_2->addWidget(label_3);

        leNumMcus = new QLineEdit(widget1);
        leNumMcus->setObjectName(QStringLiteral("leNumMcus"));
        sizePolicy.setHeightForWidth(leNumMcus->sizePolicy().hasHeightForWidth());
        leNumMcus->setSizePolicy(sizePolicy);
        leNumMcus->setMinimumSize(QSize(61, 0));
        leNumMcus->setMaximumSize(QSize(31, 16777215));

        horizontalLayout_2->addWidget(leNumMcus);

        cbEnableDec = new QCheckBox(Q_DecodeDetailDlg);
        cbEnableDec->setObjectName(QStringLiteral("cbEnableDec"));
        cbEnableDec->setGeometry(QRect(10, 150, 211, 20));
        label_4 = new QLabel(Q_DecodeDetailDlg);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(10, 180, 421, 16));
        widget2 = new QWidget(Q_DecodeDetailDlg);
        widget2->setObjectName(QStringLiteral("widget2"));
        widget2->setGeometry(QRect(329, 90, 120, 81));
        verticalLayout = new QVBoxLayout(widget2);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label_5 = new QLabel(widget2);
        label_5->setObjectName(QStringLiteral("label_5"));

        verticalLayout->addWidget(label_5);

        pbLoadCoords = new QPushButton(widget2);
        pbLoadCoords->setObjectName(QStringLiteral("pbLoadCoords"));

        verticalLayout->addWidget(pbLoadCoords);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(leMcuX);
        label_2->setBuddy(leMcuY);
        label_3->setBuddy(leNumMcus);
#endif // QT_NO_SHORTCUT

        retranslateUi(Q_DecodeDetailDlg);
        QObject::connect(buttonBox, SIGNAL(accepted()), Q_DecodeDetailDlg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Q_DecodeDetailDlg, SLOT(reject()));

        QMetaObject::connectSlotsByName(Q_DecodeDetailDlg);
    } // setupUi

    void retranslateUi(QDialog *Q_DecodeDetailDlg)
    {
        Q_DecodeDetailDlg->setWindowTitle(QApplication::translate("Q_DecodeDetailDlg", "Detailed Scan Decode Options", nullptr));
        groupBox->setTitle(QApplication::translate("Q_DecodeDetailDlg", "Starting MCU for Decode", nullptr));
        label->setText(QApplication::translate("Q_DecodeDetailDlg", "MCU X =", nullptr));
        label_2->setText(QApplication::translate("Q_DecodeDetailDlg", "MCU Y =", nullptr));
        groupBox_2->setTitle(QApplication::translate("Q_DecodeDetailDlg", "Length of Decode", nullptr));
        label_3->setText(QApplication::translate("Q_DecodeDetailDlg", "# MCUs =", nullptr));
        cbEnableDec->setText(QApplication::translate("Q_DecodeDetailDlg", "Enable detailed Scan Decode?", nullptr));
        label_4->setText(QApplication::translate("Q_DecodeDetailDlg", "Detailed Scan Decode will be reported in the next decode operation", nullptr));
        label_5->setText(QApplication::translate("Q_DecodeDetailDlg", "Load X/Y/Len\n"
"from last 2 clicks:", nullptr));
        pbLoadCoords->setText(QApplication::translate("Q_DecodeDetailDlg", "Load Coords", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Q_DecodeDetailDlg: public Ui_Q_DecodeDetailDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_Q_DECODEDETAILDLG_H
