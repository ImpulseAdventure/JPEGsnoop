#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QActionGroup>
#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPainter>

class QPrinter;

class SnoopConfigDialog;
class Q_Viewer;
class CjfifDecode;
class CimgDecode;
class CwindowBuf;
class CDocLog;
class CSnoopConfig;
class CDbSigs;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  QPlainTextEdit *textEdit;

protected:
  void closeEvent(QCloseEvent *e) override;

signals:
  void ImgSrcChanged();

public slots:
  void updateImage();
  void reprocess();

private slots:
  void open();
  void saveLog();
  void onConfig();
  void filePrint();
  void filePrintPreview();
  void printPreview(QPrinter *);

private:
  MainWindow &operator = (const MainWindow&);
  MainWindow(MainWindow&);
  
  void createActions(void);
  void createMenus(void);
  void enableMenus();
  void AnalyzeFile();
  void AnalyzeFileDo();
  void AnalyzeClose();

  CDocLog *m_pDocLog;
  CDbSigs *m_pDbSigs;
  CSnoopConfig *m_pAppConfig;
  SnoopConfigDialog *configDialog;
  Q_Viewer *imgWindow;

  // Decoders and Buffers
  CjfifDecode *m_pJfifDec;
  CimgDecode *m_pImgDec;
  CwindowBuf *m_pWBuf;

  QToolBar *fileToolBar;

  QMenu *fileMenu;
  QMenu *editMenu;
  QMenu *viewMenu;
  QMenu *imageChanMenu;
  QMenu *imageZoomMenu;
  QMenu *overlayMenu;
  QMenu *toolsMenu;
  QMenu *optionsMenu;
  QMenu *scanSegMenu;
  QMenu *helpMenu;

  // File menu
  QAction *newAct;
  QAction *openAct;
  QAction *saveLogAct;
  QAction *reprocessAct;
  QAction *batchProcessAct;
  QAction *OffsetAct;
  QAction *printAct;
  QAction *printPreviewAct;
  QAction *printSetupAct;
  QAction *recentFileAct;
  QAction *exitAct;

  // Edit menu
  QAction *undoAct;
  QAction *copyAct;
  QAction *selectAllAct;
  QAction *findAct;
  QAction *findNextAct;

  // View menu
  QAction *adjustAct;
  QAction *toolbarAct;
  QAction *statusBarAct;

  // Image Chan menu
  QAction *rgbAct;
  QAction *yccAct;
  QAction *rAct;
  QAction *gAct;
  QAction *bAct;
  QAction *yAct;
  QAction *cbAct;
  QAction *crAct;
  QActionGroup *channelGroup;

  // Image Zoom menu
  QAction *zoomInAct;
  QAction *zoomOutAct;
  QAction *zoom12_5Act;
  QAction *zoom25Act;
  QAction *zoom50Act;
  QAction *zoom100Act;
  QAction *zoom150Act;
  QAction *zoom200Act;
  QAction *zoom300Act;
  QAction *zoom400Act;
  QAction *zoom500Act;
  QActionGroup *zoomGroup;
  QActionGroup *zoomInOutGroup;

  // Overlay menu
  QAction *mcuGridAct;

  // Tools menu
  QAction *imgSearchFwdAct;
  QAction *imgSearchRevAct;
  QAction *lookupMcuOffAct;
  QAction *fileOverlayAct;
  QAction *searchDqtAct;
  QAction *exportJpegAct;
  QAction *exportTiffAct;
  QAction *addToDbAct;
  QAction *manageLocalDbAct;

  // Options menu
  QAction *dhtExpandAct;
  QAction *hideUnknownTagsAct;
  QAction *makerNotesAct;
  QAction *sigSearchAct;
  QAction *relaxedParseAct;
  QAction *configAct;
  QAction *checkUpdatesAct;

  // Scan Segment menu
  QAction *decodeImageAct;
  QAction *fullIdtcAct;
  QAction *noIdtcAct;
  QAction *histogramRgbAct;
  QAction *histogramYAct;
  QAction *dumpAct;
  QAction *detailedDecodeAct;
  QActionGroup *idtGroup;

  // Help menu
  QAction *aboutAct;

  QStatusBar *m_StatusBar;

  QMessageBox msgBox;

  QString m_currentFile;

  // Input JPEG file
  QFile *m_pFile;
  qint64 m_lFileSize;

  bool m_bFileAnalyzed;         // Have we opened and analyzed a file?
  bool m_bFileOpened;           // Is a file currently opened?

  QLabel *rgbHisto;
  QLabel *yHisto;

  // Coach Messages
  QString strLoRes;
  QString strHiRes;
};

#endif // MAINWINDOW_H
