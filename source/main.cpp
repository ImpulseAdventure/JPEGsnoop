#include <QApplication>
#include <QDebug>

#include "DbSigs.h"
#include "DocLog.h"
#include "JPEGsnoop.h"
#include "JPEGsnoopCore.h"
#include "SnoopConfig.h"

#include "mainwindow.h"

CDocLog *glb_pDocLog;
CSnoopConfig*	m_pAppConfig;
CDbSigs *m_pDbSigs;
CJPEGsnoopCore *pJPEGsnoopCore;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("ImpulseAdventure");
    QCoreApplication::setOrganizationDomain("www.impulseadventure.com");
    QCoreApplication::setApplicationName("JPEGsnoopQt");

//    m_pAppConfig = new CSnoopConfig();
//    m_pAppConfig->UseDefaults();
    glb_pDocLog = new CDocLog();
    glb_pDocLog->Enable();
    m_pDbSigs = new CDbSigs();
//    pJPEGsnoopCore = new CJPEGsnoopCore();
//    m_pAppConfig->RegistryLoad();

    MainWindow w;
    w.show();

    return a.exec();
}
