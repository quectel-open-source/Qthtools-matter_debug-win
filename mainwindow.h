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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "serial/serial.h"
#include "http/httpclient.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef struct{
    QString pk;
    QString dks;
    QString ak = "24b9rucZxRLf2WaWBXagrvFh";
    QString as = "6A3XYyTqqfPKSSurG1fd6sYM7VQGpjcvkV1gMC9w";
    int random;
    QString sign;
    QString publicKey;
}MatterBurningInfo;

typedef struct{
    int discriminator;
    int vendorId;
    int productId;
    int iterationCount;
    QString salt;
    QString verifier;
    int passcode; //pincode
    QString vendorName;
    QString productName;
    QString paiCerHexStr;
    QString cdCertHexStr;
    QString dacCertHexStr;
    //免开发信息
    QString config;
    //二维码信息
    QString qrCodeStr;
    QString manualPairCode;
}MatterCertificateInfo;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void addLogs(QString log, bool isrecv);
    void qserialSendDate(QString sendDate);
    char convertHexChar(char ch);
    QString hexStrCheckSum(const QString &str);
    void stopBurning();

private slots:
    void on_pushButton_clicked();
    void port_list_noticSlot(QList<QString> portList);
    void dataReadNoticSlot(QByteArray recvData);
    void on_pushButton_clear_clicked();
    void on_checkBox_time_clicked(bool checked);
    void on_pushButton_burning_clicked();
    void burnTimeOut();
    void scheduledSendingNoticSlot();
    void on_checkBox_timing_clicked(bool checked);
    void on_pushButton_send_clicked();
    void on_pushButton_burn_clicked();
    void on_pushButton_public_clicked();
    void on_pushButton_private_clicked();
    void on_pushButton_read_clicked();

private:
    Ui::MainWindow *ui;
    qSerial *qserial = NULL;
    HttpClient *httpClient = NULL;
    bool timestamp = true;
    QTimer *burnTime = NULL;
    QTimer *scheduledSending = NULL;
    int matterBurnMode = 0;
    bool matterBurning = false;
    bool freeSendOK = false;
    bool prcodeSendOK = false;
    bool isFrist = false;
    QString recvAllDate = "";
};
#endif // MAINWINDOW_H
