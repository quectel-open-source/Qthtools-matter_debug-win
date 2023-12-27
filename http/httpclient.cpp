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
#include "httpclient.h"

#include <QMessageBox>
#include <QTimer>

HttpClient::HttpClient()
{

}

QString HttpClient::post_request(QString url, QByteArray array)
{
    QNetworkRequest request;
    QNetworkAccessManager* naManager = new QNetworkAccessManager();
    QUrl strUrl = url;
    request.setUrl(strUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1SslV3);
    request.setSslConfiguration(conf);
    request.setRawHeader("Content-Type", "charset='utf-8'");
    request.setRawHeader("Content-Type", "application/json");
    QNetworkReply* reply = naManager->post(request, array);
    QEventLoop eventLoop;
    QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QString strReply = codec->toUnicode(reply->readAll());
    reply->deleteLater();
    qDebug()<<"POST平台应答："<<strReply;

    return strReply;
}

QString HttpClient::post_request(QNetworkRequest request, QByteArray array)
{
    QNetworkAccessManager* naManager = new QNetworkAccessManager();
    QNetworkReply* reply = naManager->post(request, array);
    QEventLoop eventLoop;
    QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QString strReply = codec->toUnicode(reply->readAll());
    reply->deleteLater();
    qDebug()<<"POST平台应答："<<strReply;

    return strReply;
}

QString HttpClient::get_request(QString url)
{
    QNetworkRequest request;
    QNetworkAccessManager* naManager = new QNetworkAccessManager();
    QUrl strUrl = url;
    request.setUrl(strUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1SslV3);
    request.setSslConfiguration(conf);
    request.setRawHeader("Content-Type", "charset='utf-8'");
    request.setRawHeader("Content-Type", "application/json");
    QNetworkReply *reply = naManager->get(request);
    QEventLoop eventLoop;
    QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QString strReply = codec->toUnicode(reply->readAll());
    reply->deleteLater();
    qDebug()<<"GET平台应答："<<strReply;

    return strReply;
}

QString HttpClient::get_request(QNetworkRequest request)
{
    QNetworkAccessManager* naManager = new QNetworkAccessManager();
    QNetworkReply *reply = naManager->get(request);
    QEventLoop eventLoop;
    QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QTextCodec* codec = QTextCodec::codecForName("utf8");
    QString strReply = codec->toUnicode(reply->readAll());
    reply->deleteLater();
    qDebug()<<"GET平台应答："<<strReply;

    return strReply;
}
