#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAcceptDrops(true);
    ui->statusBar->close();
    ui->mainToolBar->close();
    ui->progressBar->setValue(0);
    ui->textEdit->setReadOnly(true);
    ui->textEdit->setAcceptDrops(false);
    ui->textEdit_2->setReadOnly(true);

    QStringList coms;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        coms += info.portName();
    }
    ui->comboBox->addItems(coms);

    currentLC = 0xFF;

    QSettings lastBaseWriteAddr("sam", "application");
    quint32 val = lastBaseWriteAddr.value("baseAddress").toInt();
    if(val)
    {
       ui->lineEdit_2->setText("0x" + QString::number(val, 16));
    }
    else
    {
        ui->lineEdit_2->setPlaceholderText("0x8008000");
    }

    connect(&timerRefreshCOM, SIGNAL(timeout()), this, SLOT(on_timerRefreshCOM()));
    timerRefreshCOM.start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}


// CRC16
//====================================================================================
/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
quint16 const crc16_table[256] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

quint16 crc16_byte(quint16 crc, const quint8 data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

/**
 * crc16 - compute the CRC-16 for the data buffer
 * @crc:	previous CRC value
 * @buffer:	data pointer
 * @len:	number of bytes in the buffer
 *
 * Returns the updated CRC value.
 */
quint16 crc16(quint16 crc, quint8 const *buffer, quint32 len)
{
    while (len--)
        crc = crc16_byte(crc, *buffer++);
    return crc;
}
//------------------------------------------------------------------------------------


// the "open file" button
void MainWindow::on_pushButton_4_clicked()
{
    fileName = QFileDialog::getOpenFileName(this, tr("open file"), QStandardPaths::locate(QStandardPaths::DesktopLocation, NULL, QStandardPaths::LocateDirectory), tr("*.bin"));
    ui->lineEdit->setText(fileName);

    QFile *file = new QFile;
    file->setFileName(fileName);
    bool ok = file->open(QIODevice::ReadOnly);
    if(ok)
    {
        QString str1;
        QByteArray str2 = file->readAll();
        QString str3 = str2.toHex().data();

        for(int i = 0; i < str3.length(); i += 2)
        {
           QString str = str3.mid(i, 2);
           str1 += str;
           str1 += " ";
        }
        ui->textEdit->append(str1);

        QString strDisp;
        strDisp = fileName;
        strDisp.insert(0, "load file : ");
        strDisp += ", ";
        strDisp += QString::number(str2.length(), 10);
        strDisp += "B";
        strDisp += "(";
        strDisp += QString::number(str2.length() / 1024.0, 'f', 1);
        strDisp += "KB)\n";
        ui->textEdit_2->append(strDisp);
    }
}





void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();

    if(urls.isEmpty())
    {
        return;
    }

    fileName = urls.first().toLocalFile();

    QFile *file = new QFile;
    file->setFileName(fileName);
    bool ok = file->open(QIODevice::ReadOnly);
    if(ok)
    {
        QString str1;
        QByteArray str2 = file->readAll();
        QString str3 = str2.toHex().data();

        for(int i = 0; i < str3.length(); i += 2)
        {
           QString str = str3.mid(i, 2);
           str1 += str;
           str1 += " ";
        }
        ui->textEdit->append(str1);

        ui->lineEdit->setText(fileName);

        QString strDisp;
        strDisp = fileName;
        strDisp.insert(0, "load file : ");
        strDisp += ", ";
        strDisp += QString::number(str2.length(), 10);
        strDisp += "B";
        strDisp += "(";
        strDisp += QString::number(str2.length() / 1024.0, 'f', 1);
        strDisp += "KB)\n";
        ui->textEdit_2->append(strDisp);
    }
}




// select the com port
void MainWindow::on_comboBox_activated(const QString &arg1)
{
    QString comName;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        if(info.portName() == arg1)
        {
            comName = info.portName();
            comPort.setPort(info);
            break;
        }
    }

    if(comPort.open(QIODevice::ReadWrite))
    {
        comPort.setBaudRate(QSerialPort::Baud115200);
        comPort.setParity(QSerialPort::NoParity);
        comPort.setDataBits(QSerialPort::Data8);
        comPort.setStopBits(QSerialPort::OneStop);
        comPort.setFlowControl(QSerialPort::NoFlowControl);
        comPort.clearError();
        comPort.clear();
        connect(&comPort, SIGNAL(readyRead()), this, SLOT(comReadSlot()));

        comName += " open success\n";
        ui->textEdit_2->append(comName);
    }
    else
    {
        comName += " open failed\n";
        ui->textEdit_2->append(comName);
    }
}


void MainWindow::comReadSlot(void)
{
    static QByteArray arr;
    QByteArray arr_tmp = comPort.readAll();
    quint8 recvFrame[50] = { 0 };

    arr += arr_tmp;

    for(quint16 i = 0; i < arr.length(); i ++)
    {
        recvFrame[i] = arr[i];
    }

    if(recvFrame[0] != 0xEE
       || recvFrame[1] != 0xEE
       || recvFrame[2] != 0xEE
       || recvFrame[3] != 0xEE)
    {
        return;
    }

    quint16 len = recvFrame[5];
    len <<= 8;
    len |= recvFrame[4];
    if(len + 6 != 25)
    {
        return;
    }

    quint16 crc16_recv, crc16_calc;
    crc16_recv = recvFrame[len + 6 - 1];
    crc16_recv <<= 8;
    crc16_recv |= recvFrame[len + 6 - 1 - 1];

    crc16_calc = crc16(0, &recvFrame[6], len - 2);
    if(crc16_calc != crc16_recv)
    {
        return;
    }

    quint16 seq = recvFrame[9];
    seq <<= 8;
    seq |= recvFrame[8];
    if(seq + 1 != frameSeq)
    {
        return;
    }

    quint8 addr = recvFrame[6];
    quint8 cmd = recvFrame[7];
    quint8 ackcmd = recvFrame[10];
    if(cmd != 'C' || addr == 0xFF)
    {
        return;
    }

    arr.clear();    // if the whole frame receive done

    if(ackcmd == 'A')
    {
        quint8 version1, version2, version3, version4;
        QString tmp, ver;
        tmp += "lc ";
        tmp += QString::number(addr, 10);

        version1 = recvFrame[11];
        version2 = recvFrame[12];
        version3 = recvFrame[13];
        version4 = recvFrame[14];
        ver += " V";
        ver += QString::number(version1, 10);
        ver += ".";
        ver += QString::number(version2, 10);
        ver += ".";
        ver += QString::number(version3, 10);
        ver += ".";
        ver += QString::number(version4, 10);
        while(ver.endsWith(".0"))
        {
            ver.remove(ver.length() - 1 - 1, 2);
        }

        tmp += ver;
        tmp += " connected";

        QStringList lcAddr(tmp);
        modelLClist.setStringList(lcAddr);
        ui->listView->setModel(&modelLClist);

        currentLC = addr;
    }
    else if(ackcmd == 'M')
    {
        quint8 res = recvFrame[11];
        if(res == 1)
        {
            gotACK = true;
        }
        else
        {
            gotACK = false;
        }
    }
    else if(ackcmd == 'R')
    {
        quint8 res = recvFrame[11];
        if(res == 1)
        {
            gotACK = true;
        }
        else
        {
            gotACK = false;
        }
    }
    else if(ackcmd == 'S')
    {
        quint8 res = recvFrame[11];
        if(res == 1)
        {
            gotACK = true;
        }
        else
        {
            gotACK = false;
        }
    }
}


// the "connect" button
void MainWindow::on_pushButton_3_clicked()
{
    quint8 frame[16];
    quint16 frameLen = 10;
    quint16 crc16_calc;

    frame[0] = 0xFF;
    frame[1] = 0xFF;
    frame[2] = 0xFF;
    frame[3] = 0xFF;

    frame[4] = frameLen;
    frame[5] = frameLen >> 8;

    frame[6] = 0xFF;

    frame[7] = 'A';

    frame[8] = frameSeq;
    frame[9] = frameSeq >> 8;

    frame[10] = 0;
    frame[11] = 0;
    frame[12] = 0;
    frame[13] = 0;

    frame[14] = 0;
    frame[15] = 0;

    frameSeq ++;
    crc16_calc = crc16(0, &frame[6], frameLen - 2);
    frame[14] = crc16_calc;
    frame[15] = crc16_calc >> 8;
    comPort.write((const char *)frame, frameLen + 6);

    ui->listView->setModel(NULL);    // clear the connected lc list
}



// the "update" button
void MainWindow::on_pushButton_2_clicked()
{
    ui->progressBar->setValue(0);

    QFile *file = new QFile;
    file->setFileName(fileName);
    bool ok = file->open(QIODevice::ReadOnly);
    if(ok)
    {
        QByteArray arr = file->readAll();
        quint16 sendTotalCnt = (arr.length() % 1024) ? (arr.length() / 1024 + 1) : (arr.length() / 1024);
        ui->progressBar->setMaximum(sendTotalCnt);

        if(currentLC == 0xFF)
        {
            ui->textEdit_2->append("no lc connected!");
            return;
        }

        QByteArray data(ui->lineEdit_2->text().toLatin1());
        bool ok;
        qint32 writeAddr = data.toInt(&ok, 16);
        quint8 errCnt = 0;
        quint16 sendCnt = 0;
        while(1)
        {
            quint8 frame[1044];
            quint16 frameLen = 1038;

            frame[0] = 0xFF;
            frame[1] = 0xFF;
            frame[2] = 0xFF;
            frame[3] = 0xFF;

            frame[4] = frameLen;
            frame[5] = frameLen >> 8;

            frame[6] = currentLC;

            frame[7] = 'M';

            frame[8] = frameSeq;
            frame[9] = frameSeq >> 8;

            frame[10] = writeAddr;
            frame[11] = writeAddr >> 8;
            frame[12] = writeAddr >> 16;
            frame[13] = writeAddr >> 24;

            if(sendCnt == sendTotalCnt - 1)    // if it is the last frame, tell lc
            {
                frame[14] = 1;
                frame[15] = 0;
                frame[16] = 0;
                frame[17] = 0;
            }
            else
            {
                frame[14] = 0;
                frame[15] = 0;
                frame[16] = 0;
                frame[17] = 0;
            }

            // load the .bin data
            for(quint32 i = 0; i < 1024; i ++)
            {
                qint32 index = sendCnt * 1024 + i;

                if(index < arr.length())
                {
                    frame[18 + i] = arr[index];
                }
                else
                {
                    frame[18 + i] = 0xFF;
                }
            }

            gotACK = false;
            frameSeq ++;
            quint16 crc16_calc = crc16(0, &frame[6], frameLen - 2);
            frame[1042] = crc16_calc;
            frame[1043] = crc16_calc >> 8;
            comPort.write((const char *)frame, frameLen + 6);

            // after sneding a frame, we should wait for the ack frame from lc
            //==============================================================================
            QElapsedTimer et;
            et.start();
            while(et.elapsed() < 300)
            {
                QCoreApplication::processEvents();
            }
            if(gotACK == true)
            {
                writeAddr += 1024;
                sendCnt ++;
                ui->progressBar->setValue(sendCnt);
                if(sendCnt >= sendTotalCnt)
                {
                    ui->textEdit_2->append("transmit ok");
                    break;
                }
            }
            else
            {
                if(++ errCnt > 10)
                {
                    ui->textEdit_2->append("sending time out!");
                    return;
                }
            }
            //------------------------------------------------------------------------------
        }
    }
    else
    {
        QString tmp = "open file : ";
        tmp += fileName;
        tmp += " failed";
        ui->textEdit_2->append(tmp);
    }

}


// the "run" button
void MainWindow::on_pushButton_clicked()
{
    quint8 frame[16];
    quint16 frameLen = 10;
    quint16 crc16_calc;

    frame[0] = 0xFF;
    frame[1] = 0xFF;
    frame[2] = 0xFF;
    frame[3] = 0xFF;

    frame[4] = frameLen;
    frame[5] = frameLen >> 8;

    frame[6] = 0xFF;

    frame[7] = 'R';

    frame[8] = frameSeq;
    frame[9] = frameSeq >> 8;

    frame[10] = 0;
    frame[11] = 0;
    frame[12] = 0;
    frame[13] = 0;

    frame[14] = 0;
    frame[15] = 0;

    gotACK = false;
    frameSeq ++;
    crc16_calc = crc16(0, &frame[6], frameLen - 2);
    frame[14] = crc16_calc;
    frame[15] = crc16_calc >> 8;
    comPort.write((const char *)frame, frameLen + 6);

    // after sneding a frame, we should wait for the ack frame from lc
    //==============================================================================
    QElapsedTimer et;
    et.start();
    while(et.elapsed() < 300)
    {
        QCoreApplication::processEvents();
    }
    if(gotACK == true)
    {
        ui->textEdit_2->append("run ok");
    }
    else
    {
        ui->textEdit_2->append("run failed");
    }

    // close the com port to avoid this routine crashed
    comPort.close();
    //------------------------------------------------------------------------------
}

// the "base address" enter line editor
void MainWindow::on_lineEdit_2_editingFinished()
{
    QByteArray data(ui->lineEdit_2->text().toLatin1());
    bool ok;
    qint32 writeAddr = data.toInt(&ok, 16);
    if(ok == false)
    {
        return;
    }

    QSettings lastBaseWriteAddr("sam", "application");
    lastBaseWriteAddr.setValue("baseAddress", writeAddr);
}


void MainWindow::on_timerRefreshCOM()
{
    QStringList coms_old, coms;
    for(quint8 i = 0; i < ui->comboBox->count(); i ++)
    {
        coms_old += ui->comboBox->itemText(i);
    }

    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        coms += info.portName();
    }
    if(coms != coms_old)
    {
        quint16 n = ui->comboBox->count();
        for(quint16 i = 0; i < n; i ++)
        {
            ui->comboBox->removeItem(0);
        }
        ui->comboBox->addItems(coms);
    }
}






