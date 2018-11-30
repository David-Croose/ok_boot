#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QFileDialog>
#include <QStandardPaths>
#include <qdebug.h>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

#include <QtSerialPort/QtSerialPort>

#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_4_clicked();
    void on_comboBox_activated(const QString &arg1);
    void comReadSlot(void);

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_lineEdit_2_editingFinished();

    void on_timerRefreshCOM();

private:
    Ui::MainWindow *ui;

    QString fileName;

    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

    QSerialPort comPort;
    quint16 frameSeq;
    bool gotACK;

    QStringListModel modelLClist;    // stores the connected lc
    quint8 currentLC;

    QTimer timerRefreshCOM;
};

#endif // MAINWINDOW_H
