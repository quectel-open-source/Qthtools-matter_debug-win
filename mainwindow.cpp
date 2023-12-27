/*
Copyright (C) 2023  Quectel Wireless Solutions Co.,Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QAbstractItemView>
#include <QDebug>
#include <QListView>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QDateTime>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

MatterBurningInfo burningInfo, newBurningInfo;
MatterCertificateInfo matterCertificateInfo, newMatterCertificateInfo;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    httpClient = new HttpClient();

    qserial = new qSerial();
    qserial->portListPeriodSet(1000);
    connect(qserial, SIGNAL(portListNoticSignal(QList<QString>)), this, SLOT(port_list_noticSlot(QList<QString>)));
    connect(qserial, SIGNAL(dataReadNoticSignal(QByteArray)), this, SLOT(dataReadNoticSlot(QByteArray)));
    ui->comboBox_uartPort->setView(new QListView());
    ui->comboBox_uartBate->setView(new QListView());
    ui->comboBox_parity->setView(new QListView());
    ui->comboBox_stopBits->setView(new QListView());
    ui->comboBox_read->setView(new QListView());
    ui->textEdit_log->setReadOnly(true);
    ui->lineEdit_pk->setValidator(new QRegExpValidator(QRegExp("[0-9A-Za-z]{12}"),this));

    //初始化烧录定时器
    burnTime = new QTimer();
    connect(burnTime, SIGNAL(timeout()), this, SLOT(burnTimeOut()));
    //初始化自动发送定时器
    scheduledSending = new QTimer();
    connect(scheduledSending, SIGNAL(timeout()), this, SLOT(scheduledSendingNoticSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void Delay_MSec(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < _Timer )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

//串口刷新
void MainWindow::port_list_noticSlot(QList<QString> portList)
{
    qDebug()<<"串口列表发生变化"<<portList;
    if(portList.count() <= 0)
    {
        return;
    }

    QString currentPort = ui->comboBox_uartPort->currentText();
    ui->comboBox_uartPort->clear();
    ui->comboBox_uartPort->addItems(portList);

    if(this->qserial->serialIsOpen() && portList.contains(currentPort) == false)
    {
        this->qserial->SerialClose();
        ui->comboBox_uartPort->setEnabled(true);
        ui->comboBox_uartBate->setEnabled(true);
    }
    else if(this->qserial->serialIsOpen() == false)
    {
            ui->comboBox_uartPort->setCurrentText(portList.first());
    }
    else
    {
        ui->comboBox_uartPort->setCurrentText(currentPort);
    }

    //调整下拉列表宽度，完整显示
    int max_len = 0;
    for(int idx = 0; idx < ui->comboBox_uartPort->count(); idx ++)
    {
        if(max_len < ui->comboBox_uartPort->itemText(idx).toLocal8Bit().length())
        {
            max_len = ui->comboBox_uartPort->itemText(idx).toLocal8Bit().length();
        }
    }

    if (max_len * 12 * 0.75 < ui->comboBox_uartPort->width())
    {
        ui->comboBox_uartPort->view()->setFixedWidth(ui->comboBox_uartPort->width());
    }
    else
    {
        ui->comboBox_uartPort->view()->setFixedWidth(max_len * 12 * 0.75);
    }
}

//串口开关
void MainWindow::on_pushButton_clicked()
{
    if (ui->pushButton->text() == "打开")
    {
        QString portName = ui->comboBox_uartPort->currentText();
        int baundrate = ui->comboBox_uartBate->currentText().toInt();
        int parity = ui->comboBox_parity->currentText().toInt();
        int stopbate = ui->comboBox_stopBits->currentText().toInt();

        if (qserial->serialOpen(portName, baundrate, parity, stopbate, QSerialPort::NoFlowControl))
        {
            ui->pushButton->setText("关闭");
            qDebug()<<"串口已打开";
            ui->textEdit_log->append("串口已打开");
            ui->comboBox_uartPort->setEnabled(false);
            ui->comboBox_uartBate->setEnabled(false);
            ui->comboBox_parity->setEnabled(false);
            ui->comboBox_stopBits->setEnabled(false);
        }
        else
        {
            QMessageBox::critical(this, "错误", "串口打开失败", "确定");
        }
    }
    else
    {
        qserial->SerialClose();
        ui->pushButton->setText("打开");
        qDebug()<<"串口关闭";
        ui->textEdit_log->append("串口关闭");
        ui->comboBox_uartPort->setEnabled(true);
        ui->comboBox_uartBate->setEnabled(true);
        ui->comboBox_parity->setEnabled(true);
        ui->comboBox_stopBits->setEnabled(true);
        if (matterBurning)
        {
            stopBurning();
        }

        if (ui->checkBox_timing->checkState())
        {
            ui->checkBox_timing->setChecked(false);
            if (scheduledSending->isActive())
            {
                scheduledSending->stop();
            }
        }
    }
}

QString MainWindow::hexStrCheckSum(const QString &str)
{
    QByteArray senddata;
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len / 2);
    char lstr, hstr;

    for (int i = 0; i < len;) {
        hstr = str.at(i).toLatin1();
        if (hstr == ' ') {
            i++;
            continue;
        }

        i++;
        if (i >= len) {
            break;
        }

        lstr = str.at(i).toLatin1();
        hexdata = convertHexChar(hstr);
        lowhexdata = convertHexChar(lstr);

        if ((hexdata == 16) || (lowhexdata == 16)) {
            break;
        } else {
            hexdata = hexdata * 16 + lowhexdata;
        }

        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }

    senddata.resize(hexdatalen);

    uint8_t checkSum = 0;
    const char *a = senddata.data();
    for (int i = 0; i < senddata.length(); i++)
    {
        checkSum += a[i];
    }

    return QString::number(checkSum, 16);
}

char MainWindow::convertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return ch - 0x30;
    }
    else if ((ch >= 'A') && (ch <= 'F'))
    {
        return ch - 'A' + 10;
    }
    else if ((ch >= 'a') && (ch <= 'f'))
    {
        return ch - 'a' + 10;
    }
    else
    {
        return (-1);
    }
}

//串口接收处理
void MainWindow::dataReadNoticSlot(QByteArray recvData)
{
    addLogs(recvData, true);
    if (matterBurning)
    {
        recvAllDate.append(recvData);

        if (QString(recvData).contains("not found", Qt::CaseSensitive) || QString(recvData).contains("para err", Qt::CaseSensitive) || QString(recvData).contains("fail", Qt::CaseSensitive))
        {
            stopBurning();
            QMessageBox::critical(this, "错误", "烧录失败", "确定");
            return;
        }

        if(QString(recvData).contains("#", Qt::CaseSensitive))
        {
            if (QString(recvAllDate).contains("MAC address: ", Qt::CaseSensitive) && matterBurnMode == 1)
            {
                if (recvAllDate.mid(0, 3) != "mac" || recvAllDate.indexOf("#") < recvAllDate.lastIndexOf("mac"))
                {
                    recvAllDate = "";
                    return;
                }

                if (isFrist)
                {
                    isFrist = false;
                    burnTime->stop();
                    QString macAdress = recvAllDate.mid(recvAllDate.indexOf("MAC address: ") + 13, 17);
                    burningInfo.dks = macAdress.remove("-", Qt::CaseSensitive).toUpper();
                    qDebug()<<"macAddress:"<<burningInfo.dks;
                    if (burningInfo.dks.isEmpty())
                    {
                        stopBurning();
                        QMessageBox::critical(this, "错误", "烧录失败", "确定");
                        return;
                    }

                    QByteArray info  = QString("pk=" + burningInfo.pk + "&random=" + QString::number(burningInfo.random)).toUtf8();
                    burningInfo.sign = QMessageAuthenticationCode::hash(info, burningInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
                    qDebug()<<"sign:"<<burningInfo.sign;

                    QString url = "https://iot-api.quectelcn.com/v2/factory/devices/createOne";

                    QJsonObject jsonObject;
                    jsonObject.insert("mac", burningInfo.dks);
                    jsonObject.insert("pk", burningInfo.pk);
                    jsonObject.insert("ak", burningInfo.ak);
                    jsonObject.insert("random", burningInfo.random);
                    jsonObject.insert("sign", burningInfo.sign);

                    QJsonDocument document;
                    document.setObject(jsonObject);
                    QByteArray byteArray = document.toJson(QJsonDocument::Compact);
                    qDebug() <<byteArray;

                    QString strReply = httpClient->post_request(url, byteArray);
                    if (strReply.isNull())
                    {
                        stopBurning();
                        QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                        return;
                    }

                    bool isOK = false;
                    QJsonDocument tojson;
                    QJsonParseError ParseError;
                    tojson =QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
                    if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
                    {
                        if(tojson.isObject())
                        {
                            QJsonObject Object = tojson.object();
                            if(!Object.isEmpty())
                            {
                                if (Object.contains("code"))
                                {
                                    int code = Object.value("code").toInt();
                                    if (code == 200)
                                    {
                                        isOK = true;
                                    }
                                }
                            }
                        }
                    }

                    if (!isOK)
                    {
                        stopBurning();
                        QMessageBox::critical(this, "错误", "烧录失败，设备注册失败", "确定");
                        return;
                    }

                    QString sendDate = "hello\r\n";
                    Delay_MSec(300);
                    qserialSendDate(sendDate);
                    matterBurnMode = 2;
                }
            }
            else if (QString(recvAllDate).contains("be ready", Qt::CaseSensitive) && matterBurnMode == 2)
            {
                QString sendDate = "dacpub\r\n";
                qserialSendDate(sendDate);
                matterBurnMode = 3;
            }
            else if (matterBurnMode == 3)
            {
                QStringList list = QString(recvAllDate).split("\r\n");
                if (list.size() != 5)
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
                burningInfo.publicKey = "3059301306072a8648ce3d020106082a8648ce3d030107034200" + list[2];
                qDebug()<<"burningInfo.key"<<burningInfo.publicKey;

                QByteArray info  = QString("dks=[" + burningInfo.dks + "]&pk=" + burningInfo.pk + "&random=" + QString::number(burningInfo.random)).toUtf8();
                burningInfo.sign = QMessageAuthenticationCode::hash(info, burningInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
                qDebug()<<"sign:"<<burningInfo.sign;

                QString url = "https://iot-api.quectelcn.com/v2/factory/matter/device/genCertInfo";

                QJsonObject jsonObject;
                jsonObject.insert("pk", burningInfo.pk);
                QJsonArray jsonArray_dks;
                jsonArray_dks.append(burningInfo.dks);
                jsonObject.insert("dks", QJsonValue(jsonArray_dks));
                jsonObject.insert("ak", burningInfo.ak);
                jsonObject.insert("random", burningInfo.random);
                jsonObject.insert("sign", burningInfo.sign);
                QJsonObject jsonObject_dac;
                jsonObject_dac.insert("dk", burningInfo.dks);
                jsonObject_dac.insert("dacPublicKeyHexStr", burningInfo.publicKey);
                QJsonArray jsonArray_dac;
                jsonArray_dac.append(jsonObject_dac);
                jsonObject.insert("dacCommands", QJsonValue(jsonArray_dac));
                QJsonDocument document;
                document.setObject(jsonObject);
                QByteArray byteArray = document.toJson(QJsonDocument::Compact);
                qDebug() <<byteArray;

                QString strReply = httpClient->post_request(url, byteArray);
                if (strReply.isNull())
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                    return;
                }

                QJsonDocument tojson;
                QJsonParseError ParseError;
                tojson =QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
                if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
                {
                    if(tojson.isObject())
                    {
                        QJsonObject Object = tojson.object();
                        if(!Object.isEmpty())
                        {
                            if (Object.contains("code"))
                            {
                                int code = Object.value("code").toInt();
                                if (code == 200 && Object.contains("data"))
                                {
                                    QJsonObject JsonObject_data = Object.value("data").toObject();
                                    if (!JsonObject_data.isEmpty())
                                    {
                                        if (JsonObject_data.contains("devCertInfos"))
                                        {
                                            QJsonArray jsonArray_devCert = JsonObject_data.value("devCertInfos").toArray();

                                            if (jsonArray_devCert.count() > 0)
                                            {
                                                QJsonObject JsonObject_devCertInfos = jsonArray_devCert.at(0).toObject();
                                                if (!JsonObject_devCertInfos.isEmpty())
                                                {
                                                    if (JsonObject_devCertInfos.contains("discriminator"))
                                                    {
                                                        matterCertificateInfo.discriminator = JsonObject_devCertInfos.value("discriminator").toInt();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("iterationCount"))
                                                    {
                                                        matterCertificateInfo.iterationCount = JsonObject_devCertInfos.value("iterationCount").toInt();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("salt"))
                                                    {
                                                        matterCertificateInfo.salt = JsonObject_devCertInfos.value("salt").toString();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("verifier"))
                                                    {
                                                        matterCertificateInfo.verifier = JsonObject_devCertInfos.value("verifier").toString();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("passcode"))
                                                    {
                                                        matterCertificateInfo.passcode = JsonObject_devCertInfos.value("passcode").toInt();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("dacCertHexStr"))
                                                    {
                                                        matterCertificateInfo.dacCertHexStr = JsonObject_devCertInfos.value("dacCertHexStr").toString();
                                                    }
                                                    if (JsonObject_devCertInfos.contains("failDes"))
                                                    {
                                                        if(!JsonObject_devCertInfos.value("failDes").toString().isEmpty())
                                                        {
                                                            qDebug()<<"failDes:"<<JsonObject_devCertInfos.value("failDes").toString();
                                                            stopBurning();
                                                            QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                                                            return;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (JsonObject_data.contains("vendorId"))
                                        {
                                            matterCertificateInfo.vendorId = JsonObject_data.value("vendorId").toInt();
                                        }
                                        if (JsonObject_data.contains("productId"))
                                        {
                                            matterCertificateInfo.productId = JsonObject_data.value("productId").toInt();
                                        }
                                        if (JsonObject_data.contains("vendorName"))
                                        {
                                            matterCertificateInfo.vendorName = JsonObject_data.value("vendorName").toString();
                                        }
                                        if (JsonObject_data.contains("productName"))
                                        {
                                            matterCertificateInfo.productName = JsonObject_data.value("productName").toString();
                                        }
                                        if (JsonObject_data.contains("paiCerHexStr"))
                                        {
                                            matterCertificateInfo.paiCerHexStr = JsonObject_data.value("paiCerHexStr").toString();
                                        }
                                        if (JsonObject_data.contains("cdCertHexStr"))
                                        {
                                            matterCertificateInfo.cdCertHexStr = JsonObject_data.value("cdCertHexStr").toString();
                                        }
                                    }
                                }
                                else
                                {
                                    stopBurning();
                                    QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                                    return;
                                }
                            }
                        }
                    }
                }

                if (matterCertificateInfo.discriminator != 0)
                {
                    QString sendDate = "matterpara discriminator write " + QString::number(matterCertificateInfo.discriminator) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 4;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 4)
            {
                if (matterCertificateInfo.vendorId != 0)
                {
                    QString vendorId = QString::number(matterCertificateInfo.vendorId, 16);
                    if (vendorId.length() % 2)
                    {
                        vendorId = "0" + vendorId;
                    }
                    QString sendDate = "matterpara venid write " + vendorId + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 5;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 5)
            {
                if (matterCertificateInfo.productId != 0)
                {
                    QString productId = QString::number(matterCertificateInfo.productId, 16);
                    if (productId.length() % 2)
                    {
                        productId = "0" + productId;
                    }
                    QString sendDate = "matterpara proid write " + productId + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 6;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 6)
            {
                if (matterCertificateInfo.iterationCount != 0)
                {
                    QString sendDate = "matterpara itcount write " + QString::number(matterCertificateInfo.iterationCount) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 7;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 7)
            {
                if (matterCertificateInfo.salt != "")
                {
                    QString sendDate = "matterpara salt write " + matterCertificateInfo.salt + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 8;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 8)
            {
                if (matterCertificateInfo.verifier != "")
                {
                    QString sendDate = "matterpara verifier write " + matterCertificateInfo.verifier + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 9;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 9)
            {
                if (matterCertificateInfo.passcode != 0)
                {
                    QString sendDate = "matterpara pincode write " + QString::number(matterCertificateInfo.passcode) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 10;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 10)
            {
                if (matterCertificateInfo.vendorName != "")
                {
                    QString sendDate = "matterpara vename write " + matterCertificateInfo.vendorName + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 11;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 11)
            {
                if (matterCertificateInfo.productName != "")
                {
                    QString sendDate = "matterpara proname write " + matterCertificateInfo.productName + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 12;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 12)
            {
                if (matterCertificateInfo.paiCerHexStr != "")
                {
                    QString sendDate = "paicert write " + matterCertificateInfo.paiCerHexStr + " " + hexStrCheckSum(matterCertificateInfo.paiCerHexStr) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 13;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 13)
            {
                if (matterCertificateInfo.cdCertHexStr != "")
                {
                    QString sendDate = "cdcert write " + matterCertificateInfo.cdCertHexStr + " " + hexStrCheckSum(matterCertificateInfo.cdCertHexStr) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 14;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 14)
            {
                if (matterCertificateInfo.dacCertHexStr != "")
                {
                    QString sendDate = "daccert write " + matterCertificateInfo.dacCertHexStr + " " + hexStrCheckSum(matterCertificateInfo.dacCertHexStr) + "\r\n";
                    qserialSendDate(sendDate);
                    matterBurnMode = 15;
                }
                else
                {
                    stopBurning();
                    QMessageBox::critical(this, "错误", "烧录失败", "确定");
                    return;
                }
            }
            else if (QString(recvAllDate).contains("success", Qt::CaseSensitive) && matterBurnMode == 15)
            {
                if (ui->checkBox_free->isChecked() || ui->checkBox_prcode->isChecked())
                {
                    if (ui->checkBox_free->isChecked() && !freeSendOK)
                    {
                        QByteArray info  = QString("pk=" + burningInfo.pk + "&random=" + QString::number(burningInfo.random)).toUtf8();
                        burningInfo.sign = QMessageAuthenticationCode::hash(info, burningInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
                        qDebug()<<"sign:"<<burningInfo.sign;

                        QString url = "https://iot-api.quectelcn.com/v2/factory/product/deliverables/latest?pk="
                                + burningInfo.pk + "&ak=" + burningInfo.ak + "&random=" + QString::number(burningInfo.random)
                                + "&sign=" + burningInfo.sign;
        
                        QString strReply = httpClient->get_request(url);
                        if (strReply.isNull())
                        {
                            stopBurning();
                            QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                            return;
                        }

                        QJsonDocument tojson;
                        QJsonParseError ParseError;
                        tojson =QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
                        if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
                        {
                            if(tojson.isObject())
                            {
                                QJsonObject Object = tojson.object();
                                if(!Object.isEmpty())
                                {
                                    if (Object.contains("code"))
                                    {
                                        int code = Object.value("code").toInt();
                                        if (code == 200 && Object.contains("data"))
                                        {
                                            QJsonObject JsonObject_data = Object.value("data").toObject();
                                            if (!JsonObject_data.isEmpty())
                                            {
                                                if (JsonObject_data.contains("config"))
                                                {
                                                    matterCertificateInfo.config = JsonObject_data.value("config").toString();
                                                }
                                            }
                                        }
                                        else
                                        {
                                            stopBurning();
                                            QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                                            return;
                                        }
                                    }
                                }
                            }
                        }

                        if (matterCertificateInfo.config != "")
                        {
                            QString sendDate = "configpara write " + matterCertificateInfo.config + " " + hexStrCheckSum(matterCertificateInfo.config) + "\r\n";
                            qserialSendDate(sendDate);
                            freeSendOK = true;
                        }
                        else
                        {
                            stopBurning();
                            QMessageBox::critical(this, "错误", "烧录失败", "确定");
                            return;
                        }
                    }
                    else if (ui->checkBox_prcode->isChecked() && !prcodeSendOK)
                    {
                        QByteArray info  = QString("dks=[" + burningInfo.dks + "]&pk=" + burningInfo.pk + "&random=" + QString::number(burningInfo.random)).toUtf8();
                        burningInfo.sign = QMessageAuthenticationCode::hash(info, burningInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
                        qDebug()<<"sign:"<<burningInfo.sign;

                        QString url = "https://iot-api.quectelcn.com/v2/factory/matter/device/genQrCodeStr";

                        QJsonObject jsonObject;
                        jsonObject.insert("pk", burningInfo.pk);
                        QJsonArray jsonArray_dks;
                        jsonArray_dks.append(burningInfo.dks);
                        jsonObject.insert("dks", QJsonValue(jsonArray_dks));
                        jsonObject.insert("ak", burningInfo.ak);
                        jsonObject.insert("random", burningInfo.random);
                        jsonObject.insert("sign", burningInfo.sign);
                        QJsonDocument document;
                        document.setObject(jsonObject);
                        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
                        qDebug() <<byteArray;

                        QString strReply = httpClient->post_request(url, byteArray);
                        if (strReply.isNull())
                        {
                            stopBurning();
                            QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                            return;
                        }

                        QJsonDocument tojson;
                        QJsonParseError ParseError;
                        tojson =QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
                        if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
                        {
                            if(tojson.isObject())
                            {
                                QJsonObject Object = tojson.object();
                                if(!Object.isEmpty())
                                {
                                    if (Object.contains("code"))
                                    {
                                        int code = Object.value("code").toInt();
                                        if (code == 200 && Object.contains("data"))
                                        {
                                            QJsonObject JsonObject_data = Object.value("data").toObject();
                                            if (!JsonObject_data.isEmpty())
                                            {
                                                if (JsonObject_data.contains("detailDTOS"))
                                                {
                                                    QJsonArray jsonArray_detail = JsonObject_data.value("detailDTOS").toArray();
                                                    if (jsonArray_detail.count() > 0)
                                                    {
                                                        QJsonObject jsonArray_detailDTOS = jsonArray_detail.at(0).toObject();
                                                        if (!jsonArray_detailDTOS.isEmpty())
                                                        {
                                                            if (jsonArray_detailDTOS.contains("qrCodeStr"))
                                                            {
                                                                matterCertificateInfo.qrCodeStr = jsonArray_detailDTOS.value("qrCodeStr").toString();
                                                            }
                                                            if (jsonArray_detailDTOS.contains("manualPairCode"))
                                                            {
                                                                matterCertificateInfo.manualPairCode = jsonArray_detailDTOS.value("manualPairCode").toString();
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        stopBurning();
                                        QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                                        return;
                                    }
                                }
                            }
                        }

                        if (matterCertificateInfo.qrCodeStr != "" && matterCertificateInfo.manualPairCode != "")
                        {
                            QString sendDate = "matterpara qrcode write [" + matterCertificateInfo.qrCodeStr + "][" + matterCertificateInfo.manualPairCode + "]\r\n";
                            qserialSendDate(sendDate);
                            prcodeSendOK = true;
                        }
                        else
                        {
                            stopBurning();
                            QMessageBox::critical(this, "错误", "烧录失败", "确定");
                            return;
                        }
                    }
                    else
                    {
                        stopBurning();
                        ui->textEdit_log->append("烧录完成");
                    }
                }
                else
                {
                    stopBurning();
                    ui->textEdit_log->append("烧录完成");
                }

                return;
            }
            else
            {
                stopBurning();
                QMessageBox::critical(this, "错误", "烧录失败", "确定");
                return;
            }


            recvAllDate = "";
        }
    }

}
//串口发送
void MainWindow::qserialSendDate(QString sendDate)
{
    if (qserial->SerialSend(sendDate.toUtf8()) != -1)
    {
        addLogs(sendDate, false);
    }
}

void MainWindow::on_pushButton_clear_clicked()
{
    ui->textEdit_log->clear();
}


void MainWindow::on_checkBox_time_clicked(bool checked)
{
    timestamp = checked;
}

void MainWindow::addLogs(QString log, bool isrecv)
{
    QString dataHear = "";
    if (timestamp)
    {
        dataHear.append("["+QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss.zzz")+"]");
    }
    if (isrecv)
    {
        dataHear.append("[RX]");
    }
    else
    {
        dataHear.append("[TX]");
    }

    ui->textEdit_log->append(dataHear + log);
    qDebug()<<dataHear + log;
}

void MainWindow::on_pushButton_burning_clicked()
{
    if (ui->pushButton_burning->text() == "matter烧录")
    {
        if (ui->lineEdit_pk->text() == "")
        {
            QMessageBox::information(this, "提示", "请先输入PK", "确定");
            return;
        }
        if (qserial->serialIsOpen())
        {
            qDebug()<<"开始matter烧录";
            burningInfo = newBurningInfo;
            recvAllDate = "";
            ui->pushButton_burning->setText("停止烧录");
            isFrist = true;
            if (ui->checkBox_timing->checkState())
            {
                ui->checkBox_timing->setChecked(false);
                if (scheduledSending->isActive())
                {
                    scheduledSending->stop();
                }
            }
            ui->groupBox_send->setEnabled(false);
            ui->groupBox_key->setEnabled(false);
            ui->groupBox_file->setEnabled(false);
            ui->groupBox_check->setEnabled(false);
            ui->lineEdit_pk->setEnabled(false);
            ui->checkBox_free->setEnabled(false);
            ui->checkBox_prcode->setEnabled(false);
            burningInfo.pk = ui->lineEdit_pk->text();
            burningInfo.random = QRandomGenerator::global()->bounded(0, 999999);
            matterBurning = true;
            freeSendOK = false;
            prcodeSendOK = false;
            burnTime->start(500);
            matterBurnMode = 1;
        }
        else
        {
            QMessageBox::information(this, "提示", "请先打开串口", "确定");
            return;
        }
    }
    else
    {
        isFrist = false;
        qDebug()<<"停止matter烧录";
        ui->pushButton_burning->setText("matter烧录");
        matterBurning = false;
        ui->groupBox_send->setEnabled(true);
        ui->groupBox_key->setEnabled(true);
        ui->groupBox_file->setEnabled(true);
        ui->groupBox_check->setEnabled(true);
        ui->lineEdit_pk->setEnabled(true);
        ui->checkBox_free->setEnabled(true);
        ui->checkBox_prcode->setEnabled(true);
        if (burnTime->isActive())
        {
            burnTime->stop();
        }
        matterBurnMode = 0;
    }
}

void MainWindow::stopBurning()
{
    if (ui->pushButton_burning->text() == "停止烧录")
    {
        isFrist = false;
        qDebug()<<"停止matter烧录";
        ui->pushButton_burning->setText("matter烧录");
        matterBurning = false;
        ui->groupBox_send->setEnabled(true);
        ui->groupBox_key->setEnabled(true);
        ui->groupBox_file->setEnabled(true);
        ui->groupBox_check->setEnabled(true);
        ui->lineEdit_pk->setEnabled(true);
        ui->checkBox_free->setEnabled(true);
        ui->checkBox_prcode->setEnabled(true);
        if (burnTime->isActive())
        {
            burnTime->stop();
        }
        matterBurnMode = 0;
    }
}

void MainWindow::burnTimeOut()
{
    QString sendDate = "mac\r\n";
    qserialSendDate(sendDate);
}

void MainWindow::scheduledSendingNoticSlot()
{
    if (ui->textEdit_send->toPlainText() != "")
    {
        QString sendData = ui->textEdit_send->toPlainText() + "\r\n";
        qserialSendDate(sendData);
    }
}
void MainWindow::on_checkBox_timing_clicked(bool checked)
{
    if (!qserial->serialIsOpen())
    {
        ui->checkBox_timing->setChecked(false);
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }

    if (checked)
    {
        scheduledSending->start(ui->spinBox_time->value());
        ui->spinBox_time->setEnabled(false);
    }
    else
    {
        if (scheduledSending->isActive())
        {
            scheduledSending->stop();
        }
        ui->spinBox_time->setEnabled(true);
    }
}

void MainWindow::on_pushButton_send_clicked()
{
    if (!qserial->serialIsOpen())
    {
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }
    if (ui->textEdit_send->toPlainText() != "")
    {
        QString sendData = ui->textEdit_send->toPlainText() + "\r\n";
        qserialSendDate(sendData);
    }
}

void MainWindow::on_pushButton_burn_clicked()
{
    if (!qserial->serialIsOpen())
    {
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }
    if (ui->lineEdit_burnpk->text().isEmpty())
    {
        QMessageBox::information(this, "提示", "PK不能为空，请输入后再试", "确定");
        return;
    }
    int random = QRandomGenerator::global()->bounded(0, 999999);
    QString pk = ui->lineEdit_burnpk->text();
    QString oc = ui->lineEdit_burnoc->text();
    QByteArray info  = QString("pk=" + pk + "&random=" + QString::number(random)).toUtf8();
    QString sign = QMessageAuthenticationCode::hash(info, burningInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
    qDebug()<<"sign:"<<sign;
    QString url = "";
    if (ui->lineEdit_burnoc->text().isEmpty())
    {
        url = "https://iot-api.quectelcn.com/v2/factory/product/deliverables/latest?pk="
                + pk + "&ak=" + burningInfo.ak + "&random=" + QString::number(random)
                + "&sign=" + sign;
    }
    else
    {
        url = "https://iot-api.quectelcn.com/v2/factory/product/deliverables/latest?pk="
                + pk + "&ak=" + burningInfo.ak + "&random=" + QString::number(random)
                + "&sign=" + sign + "&moduleOc=" + oc;
    }

    QString strReply = httpClient->get_request(url);
    if (strReply.isNull())
    {
        QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
        return;
    }

    QJsonDocument tojson;
    QJsonParseError ParseError;
    tojson =QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
    if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
    {
        if(tojson.isObject())
        {
            QJsonObject Object = tojson.object();
            if(!Object.isEmpty())
            {
                if (Object.contains("code"))
                {
                    int code = Object.value("code").toInt();
                    if (code == 200 && Object.contains("data"))
                    {
                        QJsonObject JsonObject_data = Object.value("data").toObject();
                        if (!JsonObject_data.isEmpty())
                        {
                            if (JsonObject_data.contains("config"))
                            {
                                matterCertificateInfo.config = JsonObject_data.value("config").toString();
                            }
                        }
                    }
                    else
                    {
                        QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
                        return;
                    }
                }
            }
        }
    }

    if (matterCertificateInfo.config != "")
    {
        QString sendDate = "configpara write " + matterCertificateInfo.config + " " + hexStrCheckSum(matterCertificateInfo.config) + "\r\n";
        qserialSendDate(sendDate);
    }
    else
    {
        QMessageBox::critical(this, "错误", "烧录失败，平台返回错误", "确定");
    }
}

void MainWindow::on_pushButton_public_clicked()
{
    if (!qserial->serialIsOpen())
    {
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }

    if (ui->lineEdit_public->text().isEmpty())
    {
        QMessageBox::information(this, "提示", "输入框不能为空，请输入后再试", "确定");
        return;
    }
    else
    {
        QString sendDate = "testmatter write public " + ui->lineEdit_public->text() +"\r\n";
        qserialSendDate(sendDate);
    }
}

void MainWindow::on_pushButton_private_clicked()
{
    if (!qserial->serialIsOpen())
    {
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }

    if (ui->lineEdit_private->text().isEmpty())
    {
        QMessageBox::information(this, "提示", "输入框不能为空，请输入后再试", "确定");
        return;
    }
    else
    {
        QString sendDate = "testmatter write private " + ui->lineEdit_private->text() +"\r\n";
        qserialSendDate(sendDate);
    }
}

void MainWindow::on_pushButton_read_clicked()
{
    if (!qserial->serialIsOpen())
    {
        QMessageBox::information(this, "提示", "请先打开串口", "确定");
        return;
    }

    QString currentText = ui->comboBox_read->currentText();
    if (currentText == "private" || currentText == "public")
    {
        QString sendDate = "testmatter read " + currentText + "\r\n";
        qserialSendDate(sendDate);
    }
    else if (currentText == "cdcert" || currentText == "paicert" || currentText == "daccert")
    {
        QString sendDate = currentText + " read\r\n";
        qserialSendDate(sendDate);
    }
    else
    {
        QString sendDate = "matterpara " + currentText + " read\r\n";
        qserialSendDate(sendDate);
    }
}
