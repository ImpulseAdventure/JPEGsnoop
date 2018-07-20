#include <QtWidgets>

#include "SnoopConfig.h"
#include "snoopconfigdialog.h"
#include "JPEGsnoop.h"
//#include "Viewer.h"
#include "JfifDecode.h"
#include "ImgDecode.h"
#include "WindowBuf.h"

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("MainWindow::MainWindow() Begin");

  QWidget *widget = new QWidget;
  setCentralWidget(widget);
  setGeometry(100, 100, 800, 700);

  // Allocate the file window buffer
  m_pWBuf = new CwindowBuf();

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for File Buffer");
    exit(1);
  }

  // Allocate the JPEG decoder
  m_pImgDec = new CimgDecode(glb_pDocLog, m_pWBuf, this);

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for Image Decoder");
    exit(1);
  }

  m_pJfifDec = new CjfifDecode(glb_pDocLog, m_pWBuf, m_pImgDec);

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for JFIF Decoder");
    exit(1);
  }

  m_pAppConfig = new CSnoopConfig(m_pJfifDec);
  m_pAppConfig->RegistryLoad();

  menuBar()->setNativeMenuBar(false);
  
  createActions();
  createMenus();

//  QScrollArea *scrollArea = new QScrollArea(this);
//  scrollArea->setWidget(m_pImgDec);
//  scrollArea->setBackgroundRole(QPalette::Dark);
//  scrollArea->setVisible(false);

  QSplitter *splitter = new QSplitter(Qt::Vertical);
  doc = new QPlainTextEdit;
  doc->setMaximumBlockCount(0);
  QFont f;
  f.setFamily("Courier");
  doc->setFont(f);
  doc->appendPlainText("");

//  v = new Q_Viewer(this);
  splitter->addWidget(doc);
  splitter->addWidget(m_pImgDec);
//  splitter->addWidget(scrollArea);
  setCentralWidget(splitter);

  m_StatusBar = statusBar();
  QLabel *st1 = new QLabel("     ");
  st1->setAlignment(Qt::AlignHCenter);
  st1->setMinimumSize(st1->sizeHint());

  m_StatusBar->addPermanentWidget(st1);
  st1->setText("1234");
  m_pJfifDec->SetStatusBar(m_StatusBar);
  connect(m_pJfifDec, SIGNAL(updateStatus(QString, int)), m_StatusBar, SLOT(showMessage(QString,int)));
  connect(m_pImgDec, SIGNAL(updateStatus(QString, int)), m_StatusBar, SLOT(showMessage(QString,int)));

  glb_pDocLog->setDoc(doc);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createActions(void)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("MainWindow::createActions() Begin");

//  newAct = new QAction(tr("&New"), this);
//  newAct->setShortcuts(QKeySequence::New);
//   newAct->setStatusTip(tr("Create a new file"));
//   connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

  openAct = new QAction(tr("&Open Image..."), this);
  openAct->setShortcuts(QKeySequence::Open);
//   openAct->setStatusTip(tr("Open an existing file"));
  connect(openAct, &QAction::triggered, this, &MainWindow::open);

  saveLogAct = new QAction(tr("&Save Log..."), this);
  saveLogAct->setShortcuts(QKeySequence::Save);
  saveLogAct->setEnabled(false);
//   saveAct->setStatusTip(tr("Save the document to disk"));
  connect(saveLogAct, &QAction::triggered, this, &MainWindow::saveLog);

  reprocessAct = new QAction(tr("Reprocess File"), this);
  reprocessAct->setShortcut(tr("Ctrl+R"));
  reprocessAct->setEnabled(false);
//   saveAct->setStatusTip(tr("Save the document to disk"));
//   connect(saveAct, &QAction::triggered, this, &MainWindow::save);

  batchProcessAct = new QAction(tr("Batch Process..."), this);
  OffsetAct = new QAction(tr("Offset"), this);

  printAct = new QAction(tr("&Print..."), this);
  printAct->setShortcuts(QKeySequence::Print);
//   printAct->setStatusTip(tr("Print the document"));
//   connect(printAct, &QAction::triggered, this, &MainWindow::print);

  printPreviewAct = new QAction(tr("Print Preview"), this);
//   printAct->setStatusTip(tr("Print the document"));
//   connect(printAct, &QAction::triggered, this, &MainWindow::print);

  printSetupAct = new QAction(tr("Print Setup..."), this);
//   printAct->setStatusTip(tr("Print the document"));
//   connect(printAct, &QAction::triggered, this, &MainWindow::print);

  recentFileAct = new QAction(tr("Recent File"), this);
  recentFileAct->setEnabled(false);

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcuts(QKeySequence::Quit);
//   exitAct->setStatusTip(tr("Exit the application"));
  connect(exitAct, &QAction::triggered, this, &MainWindow::Quit);

  undoAct = new QAction(tr("Undo"), this);
  undoAct->setShortcuts(QKeySequence::Undo);
  undoAct->setEnabled(false);

  copyAct = new QAction(tr("&Copy"), this);
  copyAct->setShortcuts(QKeySequence::Copy);
  copyAct->setEnabled(false);

  selectAllAct = new QAction(tr("Select All"), this);
  selectAllAct->setShortcuts(QKeySequence::SelectAll);
  selectAllAct->setEnabled(false);

  findAct = new QAction(tr("Find..."), this);
  findAct->setShortcuts(QKeySequence::Find);
  findAct->setEnabled(false);

  findNextAct = new QAction(tr("Find Next"), this);
  findNextAct->setShortcuts(QKeySequence::FindNext);
  findNextAct->setEnabled(false);

  adjustAct = new QAction(tr("Adjust..."), this);
  adjustAct->setEnabled(false);

  // Image Channel
  rgbAct = new QAction(tr("RGB"), this);
  rgbAct->setShortcut(tr("Alt+1"));
  rgbAct->setCheckable(true);
  rgbAct->setChecked(true);

  yccAct = new QAction(tr("YCC"), this);
  yccAct->setShortcut(tr("Alt+2"));
  yccAct->setCheckable(true);

  rAct = new QAction(tr("R"), this);
  rAct->setShortcut(tr("Alt+3"));
  rAct->setCheckable(true);

  gAct = new QAction(tr("G"), this);
  gAct->setShortcut(tr("Alt+4"));
  gAct->setCheckable(true);

  bAct = new QAction(tr("B"), this);
  bAct->setShortcut(tr("Alt+5"));
  bAct->setCheckable(true);

  yAct = new QAction(tr("Y (Greyscale)"), this);
  yAct->setShortcut(tr("Alt+6"));
  yAct->setCheckable(true);

  cbAct = new QAction(tr("Cb"), this);
  cbAct->setShortcut(tr("Alt+7"));
  cbAct->setCheckable(true);

  crAct = new QAction(tr("Cr"), this);
  crAct->setShortcut(tr("Alt+8"));
  crAct->setCheckable(true);

  channelGroup = new QActionGroup(this);
  channelGroup->addAction(rgbAct);
  channelGroup->addAction(yccAct);
  channelGroup->addAction(rAct);
  channelGroup->addAction(gAct);
  channelGroup->addAction(bAct);
  channelGroup->addAction(yAct);
  channelGroup->addAction(cbAct);
  channelGroup->addAction(crAct);

  // Image Zoom
  zoomInAct = new QAction(tr("Zoom In"));
  zoomInAct->setShortcuts(QKeySequence::ZoomIn);

  zoomOutAct = new QAction(tr("Zoom Out"));
  zoomOutAct->setShortcuts(QKeySequence::ZoomOut);

  zoom12_5Act = new QAction(tr("12.5%"), this);
  zoom12_5Act->setCheckable(true);
  zoom12_5Act->setChecked(true);

  zoom25Act = new QAction(tr("25%"), this);
  zoom25Act->setCheckable(true);

  zoom50Act = new QAction(tr("50%"), this);
  zoom50Act->setCheckable(true);

  zoom100Act = new QAction(tr("100%"), this);
  zoom100Act->setCheckable(true);

  zoom150Act = new QAction(tr("150%"), this);
  zoom150Act->setCheckable(true);

  zoom200Act = new QAction(tr("200%"), this);
  zoom200Act->setCheckable(true);

  zoom300Act = new QAction(tr("300%"), this);
  zoom300Act->setCheckable(true);

  zoom400Act = new QAction(tr("400%"), this);
  zoom400Act->setCheckable(true);

  zoom500Act = new QAction(tr("500%"), this);
  zoom500Act->setCheckable(true);

  zoomGroup = new QActionGroup(this);
  zoomGroup->addAction(zoom12_5Act);
  zoomGroup->addAction(zoom25Act);
  zoomGroup->addAction(zoom50Act);
  zoomGroup->addAction(zoom100Act);
  zoomGroup->addAction(zoom150Act);
  zoomGroup->addAction(zoom200Act);
  zoomGroup->addAction(zoom300Act);
  zoomGroup->addAction(zoom400Act);
  zoomGroup->addAction(zoom500Act);

  mcuGridAct = new QAction(tr("MCU Grid"), this);
  mcuGridAct->setShortcut(tr("Ctrl+G"));

  toolbarAct = new QAction(tr("&Toolbar"), this);
  toolbarAct->setCheckable(true);
  toolbarAct->setChecked(true);

  statusBarAct = new QAction(tr("&Status Bar"), this);
  statusBarAct->setCheckable(true);
  statusBarAct->setChecked(true);

  // Tools
  imgSearchFwdAct = new QAction(tr("Image Search Fwd"), this);
  imgSearchFwdAct->setShortcut(tr("Ctrl+2"));
  imgSearchFwdAct->setEnabled(false);

  imgSearchRevAct = new QAction(tr("Image Search Rev"), this);
  imgSearchRevAct->setShortcut(tr("Ctrl+1"));
  imgSearchRevAct->setEnabled(false);

  lookupMcuOffAct = new QAction(tr("Lookup MCU Offset..."), this);
  lookupMcuOffAct->setEnabled(false);

  fileOverlayAct = new QAction(tr("File Overlay..."), this);
  fileOverlayAct->setEnabled(false);

  searchDqtAct = new QAction(tr("Search Executable for DQT..."), this);

  exportJpegAct = new QAction(tr("Export JPEG..."), this);
  exportJpegAct->setEnabled(false);

  exportTiffAct = new QAction(tr("Export TIFF..."), this);
  exportTiffAct->setEnabled(false);

  addToDbAct = new QAction(tr("Add Camera/SW to DB..."), this);
  addToDbAct->setShortcut(tr("Alt+S"));
  addToDbAct->setEnabled(false);

  manageLocalDbAct = new QAction(tr("Manage Local DB..."), this);

  // Options
  dhtExpandAct = new QAction(tr("DHT Expand"), this);
  dhtExpandAct->setCheckable(true);
  dhtExpandAct->setChecked(m_pAppConfig->bOutputDHTexpand);
  connect(dhtExpandAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onOptionsDhtexpand()));

  hideUnknownTagsAct = new QAction(tr("Hide Unknown EXIF Tags"), this);
  hideUnknownTagsAct->setCheckable(true);
  hideUnknownTagsAct->setChecked(m_pAppConfig->bExifHideUnknown);
  connect(hideUnknownTagsAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onOptionsHideuknownexiftags()));

  makerNotesAct = new QAction(tr("Maker Notes"), this);
  makerNotesAct->setCheckable(true);
  hideUnknownTagsAct->setChecked(m_pAppConfig->bDecodeMaker);
  connect(makerNotesAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onOptionsMakernotes()));

  sigSearchAct = new QAction(tr("Signature Search"), this);
  sigSearchAct->setCheckable(true);
  sigSearchAct->setChecked(m_pAppConfig->bSigSearch);
  connect(sigSearchAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onOptionsSignaturesearch()));

  relaxedParseAct = new QAction(tr("Relaxed Parsing"), this);
  relaxedParseAct->setCheckable(true);
  relaxedParseAct->setChecked(m_pAppConfig->bRelaxedParsing);
  connect(relaxedParseAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onOptionsRelaxedparsing()));

  configAct = new QAction(tr("Configuration..."), this);
  connect(configAct, &QAction::triggered, this, &MainWindow::onConfig);

  checkUpdatesAct = new QAction(tr("Check for Updates..."), this);

  // Scan Segment menu
  decodeImageAct = new QAction(tr("Decode Image"), this);
  decodeImageAct->setCheckable(true);
  decodeImageAct->setChecked(m_pAppConfig->bDecodeScanImg);
  connect(decodeImageAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentDecodeImage()));

  fullIdtcAct = new QAction(tr("Full IDCT (AC+DC - slow)"), this);;
  fullIdtcAct->setCheckable(true);
  connect(fullIdtcAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentFullidct()));

  noIdtcAct = new QAction(tr("No IDCT (DC only - fast)"), this);;
  noIdtcAct->setCheckable(true);
  connect(noIdtcAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentNoidct()));

  qDebug() << "Action" << m_pAppConfig->bDecodeScanImgAc;

  if(m_pAppConfig->bDecodeScanImgAc)
  {
    fullIdtcAct->setChecked(true);
  }
  else
  {
    noIdtcAct->setChecked(true);
  }

  idtGroup = new QActionGroup(this);
  idtGroup->addAction(noIdtcAct);
  idtGroup->addAction(fullIdtcAct);

  histogramRgbAct = new QAction(tr("Histogram RGB"), this);;
  histogramRgbAct->setCheckable(true);
  histogramRgbAct->setChecked(m_pAppConfig->bHistoEn);
  connect(histogramRgbAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentHistogram()));

  histogramYAct = new QAction(tr("Histogram Y"), this);;
  histogramYAct->setCheckable(true);
  histogramYAct->setChecked(m_pAppConfig->bDumpHistoY);
  connect(histogramYAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentHistogramy()));

  dumpAct = new QAction(tr("Dump"), this);;
  dumpAct->setCheckable(true);
  dumpAct->setChecked(m_pAppConfig->bOutputScanDump);
  connect(dumpAct, SIGNAL(triggered()), m_pAppConfig, SLOT(onScansegmentDump()));

  detailedDecodeAct = new QAction(tr("Detailed Decode..."), this);

  // Help menu
  aboutAct = new QAction(tr("&About JPEGsnoop..."));

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("MainWindow::createActions() End");
}

void MainWindow::createMenus(void)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("MainWindow::createMenus() Begin");

  fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveLogAct);
  fileMenu->addAction(reprocessAct);
  fileMenu->addSeparator();
  fileMenu->addAction(OffsetAct);
  fileMenu->addSeparator();
  fileMenu->addAction(printAct);
  fileMenu->addAction(printPreviewAct);
  fileMenu->addAction(printSetupAct);
  fileMenu->addSeparator();
  fileMenu->addAction(recentFileAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  editMenu = menuBar()->addMenu("&Edit");
  editMenu->addAction(undoAct);
  editMenu->addSeparator();
  editMenu->addAction(copyAct);
  editMenu->addAction(selectAllAct);
  editMenu->addSeparator();
  editMenu->addAction(findAct);
  editMenu->addAction(findNextAct);

  viewMenu = menuBar()->addMenu("&View");
  imageChanMenu = viewMenu->addMenu(tr("Image Chan"));
  imageZoomMenu = viewMenu->addMenu(tr("Image Zoom"));
  overlayMenu = viewMenu->addMenu(tr("Overlays"));
  viewMenu->addAction(adjustAct);
  viewMenu->addSeparator();
  viewMenu->addAction(toolbarAct);
  viewMenu->addAction(statusBarAct);

  imageChanMenu->addAction(rgbAct);
  imageChanMenu->addAction(yccAct);
  imageChanMenu->addSeparator();
  imageChanMenu->addAction(rAct);
  imageChanMenu->addAction(gAct);
  imageChanMenu->addAction(bAct);
  imageChanMenu->addSeparator();
  imageChanMenu->addAction(yAct);
  imageChanMenu->addAction(cbAct);
  imageChanMenu->addAction(crAct);

  imageZoomMenu->addAction(zoomInAct);
  imageZoomMenu->addAction(zoomOutAct);
  imageZoomMenu->addSeparator();
  imageZoomMenu->addAction(zoom12_5Act);
  imageZoomMenu->addAction(zoom25Act);
  imageZoomMenu->addAction(zoom50Act);
  imageZoomMenu->addAction(zoom100Act);
  imageZoomMenu->addAction(zoom150Act);
  imageZoomMenu->addAction(zoom200Act);
  imageZoomMenu->addAction(zoom300Act);
  imageZoomMenu->addAction(zoom400Act);
  imageZoomMenu->addAction(zoom500Act);

  overlayMenu->addAction(mcuGridAct);

  toolsMenu = menuBar()->addMenu("&Tools");
  toolsMenu->addAction(imgSearchFwdAct);
  toolsMenu->addAction(imgSearchRevAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(lookupMcuOffAct);
  toolsMenu->addAction(fileOverlayAct);
  toolsMenu->addAction(searchDqtAct);
  toolsMenu->addAction(exportJpegAct);
  toolsMenu->addAction(exportTiffAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(addToDbAct);
  toolsMenu->addAction(manageLocalDbAct);

  optionsMenu = menuBar()->addMenu("&Options");
  optionsMenu->addAction(dhtExpandAct);
  optionsMenu->addAction(hideUnknownTagsAct);
  optionsMenu->addAction(makerNotesAct);
  optionsMenu->addAction(sigSearchAct);
  optionsMenu->addAction(relaxedParseAct);
  optionsMenu->addSeparator();
  scanSegMenu = optionsMenu->addMenu(tr("Scan Segment"));
  optionsMenu->addSeparator();
  optionsMenu->addAction(configAct);
  optionsMenu->addAction(checkUpdatesAct);

  // Scan Segment menu
  scanSegMenu->addAction(decodeImageAct);
  scanSegMenu->addSeparator();
  scanSegMenu->addAction(fullIdtcAct);
  scanSegMenu->addAction(noIdtcAct);
  scanSegMenu->addSeparator();
  scanSegMenu->addAction(histogramRgbAct);
  scanSegMenu->addAction(histogramYAct);
  scanSegMenu->addSeparator();
  scanSegMenu->addAction(dumpAct);
  scanSegMenu->addAction(detailedDecodeAct);

  // Help menu
  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutAct);
}

void MainWindow::open()
{
  QFileDialog dialog(this);

  QStringList filters;
  filters << "JPEG Image (*.jpg *.jpeg)"
          << "Thumbnail (*.thm)"
          << "QuickTime Movie (*.mov)"
          << "Digital Negative (*.dng)"
          << "RAW Image (*.crw *.cr2 *.nef *.orf *.pef)"
          << "PDF Files (*.pdf)"
          << "Photoshop Files (*.psd)"
          << "All Files (*)";
  dialog.setNameFilters(filters);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setOption(QFileDialog::ReadOnly);
  dialog.setDirectory("/Users/bob/Downloads");

  if(dialog.exec())
  {
    QStringList files = dialog.selectedFiles();

    m_currentFile = files.at(0);

    m_pFile = new QFile(m_currentFile);

    if(m_pFile->open(QIODevice::ReadOnly) == false)
    {
      QString strError;

      // Note: msg includes m_strPathName
      strError = QString("ERROR: Couldn't open file: [%1]").arg(m_currentFile);
      glb_pDocLog->AddLineErr(strError);

      if(m_pAppConfig->bInteractive)
      {
        msgBox.setText(strError);
        msgBox.exec();
      }

      m_pFile = NULL;
    }
    else
    {
      // Set the file size variable
      m_lFileSize = m_pFile->size();

      // Don't attempt to load buffer with zero length file!
      if(m_lFileSize > 0)
      {
        // Open up the buffer
        m_pWBuf->BufFileSet(m_pFile);
        m_pWBuf->BufLoadWindow(0);

        // Mark file as opened
        m_bFileOpened = true;

        enableMenus();
        AnalyzeFileDo();
        AnalyzeClose();
      }
      else
      {
        m_pFile->close();
      }
    }
  }
}

void MainWindow::AnalyzeFileDo()
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("MainWindow::AnalyzeFileDo() Start");

  // Ensure file was already opened
  Q_ASSERT(m_pFile);
  Q_ASSERT(m_bFileOpened);

  // Reset the analyzed state in case this file is invalid (eg. zero length)
  m_bFileAnalyzed = false;

  // Clear the document log
  glb_pDocLog->Clear();

  glb_pDocLog->AddLine("");
  glb_pDocLog->AddLine(QString("JPEGsnoop %1 by Calvin Hass").arg(QString(VERSION_STR)));
  glb_pDocLog->AddLine("  http://www.impulseadventure.com/photo/");
  glb_pDocLog->AddLine("  -------------------------------------");
  glb_pDocLog->AddLine("");
  glb_pDocLog->AddLine(QString("  Filename: [%1]").arg(m_currentFile));
  glb_pDocLog->AddLine(QString("  Filesize: [%1] bytes").arg(m_lFileSize));
  glb_pDocLog->AddLine("");

  // Perform the actual decoding
  if(m_lFileSize > 0xFFFFFFFF)
  {
    glb_pDocLog->AddLineErr(QString("ERROR: File too large for this version of JPEGsnoop. [Size=0x%1]").arg(m_lFileSize));
  }
  else if(m_lFileSize == 0)
  {
    glb_pDocLog->AddLineErr("ERROR: File length is zero, no decoding done.");
  }
  else
  {
    if(DEBUG_EN)
      m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeFileDo() Checkpoint 2");

    m_pJfifDec->ProcessFile(m_pFile);

    // Now indicate that the file has been processed
    m_bFileAnalyzed = true;
  }
}


// Close the current file
// - Invalidate the buffer
//
// POST:
// - m_bFileOpened
//
void MainWindow::AnalyzeClose()
{
  // Indicate no file is currently opened
  // Note that we don't clear m_bFileAnalyzed as the last
  // decoder state is still available
  m_bFileOpened = false;

  // Close the buffer window
  m_pWBuf->BufFileUnset();

  m_pFile->close();
}

void MainWindow::saveLog()
{
  QFileDialog dialog(this);

  QStringList filters;
  filters << "Text File (*.txt)";

  QFileInfo fileInfo(m_currentFile);

  dialog.setWindowModality(Qt::WindowModal);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setNameFilters(filters);
//  dialog.setDirectory("/Users/bob/Downloads");
  dialog.selectFile(fileInfo.absoluteFilePath() + "/" + fileInfo.baseName() + ".txt");

  if (dialog.exec() != QDialog::Accepted)
      return;

//  return saveFile(dialog.selectedFiles().first());
  QFile file(dialog.selectedFiles().first());

     if (!file.open(QFile::WriteOnly | QFile::Text))
     {
         QMessageBox::warning(this, tr("Application"),
                              tr("Cannot write file %1:\n%2.")
                              .arg(QDir::toNativeSeparators(dialog.selectedFiles().first()),
                                   file.errorString()));
         return;
     }

     QTextStream out(&file);
     out << doc->toPlainText();

  file.close();
}

void MainWindow::enableMenus()
{
  saveLogAct->setEnabled(true);
  reprocessAct->setEnabled(true);
  selectAllAct->setEnabled(true);
  findAct->setEnabled(true);
  imgSearchFwdAct->setEnabled(true);
  imgSearchRevAct->setEnabled(true);
  lookupMcuOffAct->setEnabled(true);
  fileOverlayAct->setEnabled(true);
  searchDqtAct->setEnabled(true);
  exportJpegAct->setEnabled(true);
  exportTiffAct->setEnabled(true);
  addToDbAct->setEnabled(true);
  manageLocalDbAct->setEnabled(true);
}

void MainWindow::onConfig()
{
  configDialog = new SnoopConfigDialog();
  configDialog->show();
}

void MainWindow::Quit()
{
  m_pAppConfig->RegistryStore();
  QApplication::quit();
}
