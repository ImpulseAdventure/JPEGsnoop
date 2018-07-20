/********************************************************************************
** Form generated from reading UI file 'snoopconfigdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SNOOPCONFIGDIALOG_H
#define UI_SNOOPCONFIGDIALOG_H

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
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SnoopConfigDialog
{
public:
    QDialogButtonBox *buttonBox;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout_3;
    QGroupBox *groupBox;
    QWidget *layoutWidget1;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *leDbPath;
    QVBoxLayout *verticalLayout;
    QPushButton *btnBrowse;
    QPushButton *btnDefault;
    QGroupBox *groupBox_2;
    QWidget *layoutWidget2;
    QHBoxLayout *horizontalLayout;
    QCheckBox *cbAutoUpdate;
    QLineEdit *leUpdateDays;
    QLabel *label_2;
    QGroupBox *groupBox_3;
    QWidget *layoutWidget3;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *cbAutoReprocess;
    QCheckBox *cbOblineDb;
    QGroupBox *groupBox_4;
    QWidget *layoutWidget4;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QLineEdit *leMaxErrors;
    QGroupBox *groupBox_5;
    QWidget *layoutWidget5;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_3;
    QPushButton *btnReset;

    void setupUi(QDialog *SnoopConfigDialog)
    {
        if (SnoopConfigDialog->objectName().isEmpty())
            SnoopConfigDialog->setObjectName(QStringLiteral("SnoopConfigDialog"));
        SnoopConfigDialog->resize(541, 426);
        SnoopConfigDialog->setModal(true);
        buttonBox = new QDialogButtonBox(SnoopConfigDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(440, 10, 81, 71));
        buttonBox->setOrientation(Qt::Vertical);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
        layoutWidget = new QWidget(SnoopConfigDialog);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(20, 10, 402, 406));
        verticalLayout_3 = new QVBoxLayout(layoutWidget);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(layoutWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setMinimumSize(QSize(400, 80));
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
        layoutWidget1 = new QWidget(groupBox);
        layoutWidget1->setObjectName(QStringLiteral("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(10, 10, 381, 71));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget1);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        leDbPath = new QLineEdit(layoutWidget1);
        leDbPath->setObjectName(QStringLiteral("leDbPath"));
        leDbPath->setMaximumSize(QSize(320, 20));

        horizontalLayout_2->addWidget(leDbPath);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        btnBrowse = new QPushButton(layoutWidget1);
        btnBrowse->setObjectName(QStringLiteral("btnBrowse"));

        verticalLayout->addWidget(btnBrowse);

        btnDefault = new QPushButton(layoutWidget1);
        btnDefault->setObjectName(QStringLiteral("btnDefault"));

        verticalLayout->addWidget(btnDefault);


        horizontalLayout_2->addLayout(verticalLayout);


        verticalLayout_3->addWidget(groupBox);

        groupBox_2 = new QGroupBox(layoutWidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setMinimumSize(QSize(400, 60));
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
        layoutWidget2 = new QWidget(groupBox_2);
        layoutWidget2->setObjectName(QStringLiteral("layoutWidget2"));
        layoutWidget2->setGeometry(QRect(10, 20, 381, 31));
        horizontalLayout = new QHBoxLayout(layoutWidget2);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        cbAutoUpdate = new QCheckBox(layoutWidget2);
        cbAutoUpdate->setObjectName(QStringLiteral("cbAutoUpdate"));
        cbAutoUpdate->setMaximumSize(QSize(260, 20));

        horizontalLayout->addWidget(cbAutoUpdate);

        leUpdateDays = new QLineEdit(layoutWidget2);
        leUpdateDays->setObjectName(QStringLiteral("leUpdateDays"));
        leUpdateDays->setMaximumSize(QSize(40, 20));

        horizontalLayout->addWidget(leUpdateDays);

        label_2 = new QLabel(layoutWidget2);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setMaximumSize(QSize(40, 20));

        horizontalLayout->addWidget(label_2);


        verticalLayout_3->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(layoutWidget);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        groupBox_3->setMinimumSize(QSize(400, 80));
        groupBox_3->setStyleSheet(QLatin1String("QGroupBox {\n"
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
        groupBox_3->setFlat(true);
        layoutWidget3 = new QWidget(groupBox_3);
        layoutWidget3->setObjectName(QStringLiteral("layoutWidget3"));
        layoutWidget3->setGeometry(QRect(10, 20, 335, 41));
        verticalLayout_2 = new QVBoxLayout(layoutWidget3);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        cbAutoReprocess = new QCheckBox(layoutWidget3);
        cbAutoReprocess->setObjectName(QStringLiteral("cbAutoReprocess"));

        verticalLayout_2->addWidget(cbAutoReprocess);

        cbOblineDb = new QCheckBox(layoutWidget3);
        cbOblineDb->setObjectName(QStringLiteral("cbOblineDb"));

        verticalLayout_2->addWidget(cbOblineDb);


        verticalLayout_3->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(layoutWidget);
        groupBox_4->setObjectName(QStringLiteral("groupBox_4"));
        groupBox_4->setMinimumSize(QSize(400, 60));
        groupBox_4->setMaximumSize(QSize(16777215, 60));
        groupBox_4->setStyleSheet(QLatin1String("QGroupBox {\n"
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
        groupBox_4->setFlat(true);
        layoutWidget4 = new QWidget(groupBox_4);
        layoutWidget4->setObjectName(QStringLiteral("layoutWidget4"));
        layoutWidget4->setGeometry(QRect(10, 20, 325, 31));
        horizontalLayout_3 = new QHBoxLayout(layoutWidget4);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(layoutWidget4);
        label->setObjectName(QStringLiteral("label"));
        label->setMinimumSize(QSize(240, 20));
        label->setMaximumSize(QSize(240, 20));

        horizontalLayout_3->addWidget(label);

        leMaxErrors = new QLineEdit(layoutWidget4);
        leMaxErrors->setObjectName(QStringLiteral("leMaxErrors"));
        leMaxErrors->setMaximumSize(QSize(40, 20));

        horizontalLayout_3->addWidget(leMaxErrors);


        verticalLayout_3->addWidget(groupBox_4);

        groupBox_5 = new QGroupBox(layoutWidget);
        groupBox_5->setObjectName(QStringLiteral("groupBox_5"));
        groupBox_5->setMinimumSize(QSize(400, 60));
        groupBox_5->setMaximumSize(QSize(16777215, 60));
        groupBox_5->setStyleSheet(QLatin1String("QGroupBox {\n"
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
        groupBox_5->setFlat(true);
        layoutWidget5 = new QWidget(groupBox_5);
        layoutWidget5->setObjectName(QStringLiteral("layoutWidget5"));
        layoutWidget5->setGeometry(QRect(10, 10, 311, 48));
        horizontalLayout_4 = new QHBoxLayout(layoutWidget5);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_3 = new QLabel(layoutWidget5);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setMaximumSize(QSize(230, 20));

        horizontalLayout_4->addWidget(label_3);

        btnReset = new QPushButton(layoutWidget5);
        btnReset->setObjectName(QStringLiteral("btnReset"));
        btnReset->setMinimumSize(QSize(80, 32));
        btnReset->setMaximumSize(QSize(80, 32));

        horizontalLayout_4->addWidget(btnReset);


        verticalLayout_3->addWidget(groupBox_5);


        retranslateUi(SnoopConfigDialog);

        QMetaObject::connectSlotsByName(SnoopConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *SnoopConfigDialog)
    {
        SnoopConfigDialog->setWindowTitle(QApplication::translate("SnoopConfigDialog", "JPEGsnoop Confguration", nullptr));
        groupBox->setTitle(QApplication::translate("SnoopConfigDialog", "Directory for User Database", nullptr));
        btnBrowse->setText(QApplication::translate("SnoopConfigDialog", "Browse", nullptr));
        btnDefault->setText(QApplication::translate("SnoopConfigDialog", "Default", nullptr));
        groupBox_2->setTitle(QApplication::translate("SnoopConfigDialog", "Software Update", nullptr));
        cbAutoUpdate->setText(QApplication::translate("SnoopConfigDialog", "Automatically check for updates every ", nullptr));
        label_2->setText(QApplication::translate("SnoopConfigDialog", "days", nullptr));
        groupBox_3->setTitle(QApplication::translate("SnoopConfigDialog", "Processing Options", nullptr));
        cbAutoReprocess->setText(QApplication::translate("SnoopConfigDialog", "Auto Reprocess files on change in Options (slower)", nullptr));
        cbOblineDb->setText(QApplication::translate("SnoopConfigDialog", "Submit signature to Online DB when User submit", nullptr));
        groupBox_4->setTitle(QApplication::translate("SnoopConfigDialog", "Report Settings", nullptr));
        label->setText(QApplication::translate("SnoopConfigDialog", "Max # errors to report in Scan Decode", nullptr));
        groupBox_5->setTitle(QApplication::translate("SnoopConfigDialog", "Help", nullptr));
        label_3->setText(QApplication::translate("SnoopConfigDialog", "Coach Messages can be re-enabled:", nullptr));
        btnReset->setText(QApplication::translate("SnoopConfigDialog", "Reset", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SnoopConfigDialog: public Ui_SnoopConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SNOOPCONFIGDIALOG_H
