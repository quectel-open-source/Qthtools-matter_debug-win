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
#include "openlog.h"
#include "ccrashstack.h"
#include "logthread.h"

#include <QDir>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDirIterator>
#include <QMessageBox>
#include "QtGui/private/qzipreader_p.h"
#include "QtGui/private/qzipwriter_p.h"
#include "QMessageBox"

QFile *newFile1 = nullptr;
QFile *newFile2 = nullptr;
static int fileCutting = 0;
static QString dateLog = "";
static QString dateDump = "";
static QString logFile = "";
static QString newLogFolder = "";
extern QString softwareName;
//线程类
fileThread *filethread;
timeThread *timethread;
packThead *packthead;

OpenLog::OpenLog(QString logfolder)
{
    if (logfolder != "")
    {
        newLogFolder = logfolder;
    }
    if (newLogFolder == "")
    {
        newLogFolder = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/" + softwareName;
    }

    filethread = new fileThread();
    timethread = new timeThread();
    packthead = new packThead();
}

OpenLog::~OpenLog()
{

}

void printfLogToFile1(QtMsgType type,const QMessageLogContext &context,const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    static QMutex mutex;
    mutex.lock();
    QString current_date_time = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss zzz");
    QString message = QString("[%1] %2 %3").arg(current_date_time).arg(msg).arg("\r\n");
    QTextStream text_stream(newFile1);
    text_stream << message;
    newFile1->flush();
    mutex.unlock();
}

void printfLogToFile2(QtMsgType type,const QMessageLogContext &context,const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    static QMutex mutex2;
    mutex2.lock();
    QString current_date_time = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss zzz");
    QString message = QString("[%1] %2 %3").arg(current_date_time).arg(msg).arg("\r\n");
    QTextStream text_stream(newFile2);
    text_stream << message;
    newFile2->flush();
    mutex2.unlock();
}

long __stdcall   callback(_EXCEPTION_POINTERS*   excp)
{
    CCrashStack crashStack(excp);
    QString sCrashInfo = crashStack.GetExceptionInfo();
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyyMMddhhmmsszzz");
    QString sFileName = dateDump + "/dump_" + current_date + ".log";

    QFile file(sFileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        file.write(sCrashInfo.toUtf8());
        file.close();
    }

    QMessageBox::critical(NULL,"Dump",QObject::tr("<FONT size=4><div><b>对于发生的错误，表示诚挚的歉意</b><br/></div>"),QObject::tr("确定"));
    return   EXCEPTION_EXECUTE_HANDLER;
}
/**************************************************************************
** 功能	@brief : 日志保存
** 输入	@param :
** 输出	@retval:
** 注意  @note: dump日志需要开启宏
**      @example:
***************************************************************************/
void OpenLog::setLog()
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QDateTime expirationTime = current_date_time.addDays(-15);

    QDir *folder = new QDir;
    bool exist = folder->exists(newLogFolder);
    if (!exist)
    {
        folder->mkdir(newLogFolder);
    }

    QString softwareLog = newLogFolder + "/SoftwareLog";
    if (!folder->exists(softwareLog))
    {
        folder->mkdir(softwareLog);
    }

    QDir *old_dir = new QDir(softwareLog);
    QStringList filter;
    QList<QFileInfo> *fileInfo = new QList<QFileInfo>(old_dir->entryInfoList(filter));
    for (int i = 2; i < fileInfo->count(); i++)
    {
        if (fileInfo->at(i).fileName() != "")
        {
            QDateTime filDdateTime = QDateTime::fromString(fileInfo->at(i).fileName(), "yyyyMMdd");
            if (filDdateTime <= expirationTime)
            {
                QDir del;
                del.setPath(fileInfo->at(i).filePath());
                del.removeRecursively();
            }
        }
    }

    QString dateFile = softwareLog + "/" + current_date_time.toString("yyyyMMdd");
    if (!folder->exists(dateFile))
    {
        folder->mkdir(dateFile);
    }

    dateLog = dateFile + "/log";
    if (!folder->exists(dateLog))
    {
        folder->mkdir(dateLog);
    }

    QString current_date = current_date_time.toString("yyyyMMddhhmmsszzz");
    logFile = dateLog + "/log_" + current_date + ".txt";
    newFile1 = new QFile(logFile);
    newFile1->open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Truncate);
    qInstallMessageHandler(printfLogToFile1);
    fileCutting = 1;
    filethread->getlogfile(logFile);

#ifdef OPENDUMP
    dateDump = dateFile + "/dump";
    if (!folder->exists(dateDump))
    {
        folder->mkdir(dateDump);
    }
    SetUnhandledExceptionFilter(callback);
#endif

    filethread->start();
    timethread->start();
}

void OpenLog::openNewLog()
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QDateTime expirationTime = current_date_time.addDays(-15);

    QDir *folder = new QDir;
    bool exist = folder->exists(newLogFolder);
    if (!exist)
    {
        folder->mkdir(newLogFolder);
    }

    QString softwareLog = newLogFolder + "/SoftwareLog";
    if (!folder->exists(softwareLog))
    {
        folder->mkdir(softwareLog);
    }

    QDir *old_dir = new QDir(softwareLog);
    QStringList filter;
    QList<QFileInfo> *fileInfo = new QList<QFileInfo>(old_dir->entryInfoList(filter));
    for (int i = 2; i < fileInfo->count(); i++)
    {
        if (fileInfo->at(i).fileName() != "")
        {
            QDateTime filDdateTime = QDateTime::fromString(fileInfo->at(i).fileName(), "yyyyMMdd");
            if (filDdateTime < expirationTime)
            {
                QDir del;
                del.setPath(fileInfo->at(i).filePath());
                del.removeRecursively();
            }
        }
    }

    QString dateFile = softwareLog + "/" + current_date_time.toString("yyyyMMdd");
    if (!folder->exists(dateFile))
    {
        folder->mkdir(dateFile);
    }

    dateLog = dateFile + "/log";
    if (!folder->exists(dateLog))
    {
        folder->mkdir(dateLog);
    }

#ifdef OPENDUMP
    dateDump = dateFile + "/dump";
    if (!folder->exists(dateDump))
    {
        folder->mkdir(dateDump);
    }
    SetUnhandledExceptionFilter(callback);
#endif

    QString current_date = current_date_time.toString("yyyyMMddhhmmsszzz");
    logFile = dateLog + "/log_" + current_date + ".txt";
    if (fileCutting == 0)
    {
        newFile1 = new QFile(logFile);
        newFile1->open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Truncate);
        qInstallMessageHandler(printfLogToFile1);
        filethread->getlogfile(logFile);
        newFile2->close();
        fileCutting = 1;
        return;
    }
    else if (fileCutting == 1)
    {
        newFile2 = new QFile(logFile);
        newFile2->open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Truncate);
        qInstallMessageHandler(printfLogToFile2);
        filethread->getlogfile(logFile);
        newFile1->close();
        fileCutting = 0;
        return;
    }
}

#ifdef SAVELOG
/**********************************************************************************************
** 功能	@brief : 导出log日志文件
** 输入	@param :
** 输出	@retval: bool类型，false导出失败，true，true导出成功
** 注意  @note:
**      @example:
**********************************************************************************************/
void OpenLog::cpDateLog()
{
    QString filePath = QFileDialog::getSaveFileName(0, tr("选择文件存储路径"), "" , "*.txt");
    if (filePath == NULL)
    {
        return;
    }

    QFile::remove(filePath);
    if((QFile::copy(logFile, filePath)))
    {
        return;
    }
    return;
}

void OpenLog::cpDateDumpIsNullSlot()
{
    QMessageBox::information(0, tr("提示"), tr("最近15天未出现异常日志"), tr("确定"));
}

#ifdef OPENDUMP
static QString latestdate = "";
static QString filePath = "";
/**********************************************************************************************
** 功能	@brief : 导出dump日志文件
** 输入	@param :
** 输出	@retval: bool类型，false导出失败，true导出成功
** 注意  @note:
**      @example:
**********************************************************************************************/
void OpenLog::cpDateDump()  //打包线程
{
    QDirIterator it(newLogFolder + "/SoftwareLog", QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QString latestfolder = "";

    //校验dump日志文件名格式
    while (it.hasNext())
    {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        //找出存在dump日志的文件路径（路径格式：.../dump/dump_20220930141111222.log）
        if (fileInfo.absoluteFilePath().indexOf("/dump/dump_") > 0 && fileInfo.absoluteFilePath().indexOf("log") > 0)
        {
            QString datefile = fileInfo.absoluteFilePath();
            QStringList fileList = datefile.split('/');
            if (fileList.size() > 0)
            {
                QString dumpname = fileList[fileList.size() - 1];
                if (dumpname.size() == 26)
                {
                    QString dumptime = dumpname.mid(5, 17);    //截取dump文件名，保留中间部分
                    const char *s = dumptime.toLatin1().data();
                    while (*s)
                    {
                        if (*s >= '0' && *s <= '9');
                        else
                        {
                            continue;
                        }
                        s ++;
                    }

                    QDateTime current_date_time = QDateTime::currentDateTime();
                    QDateTime expirationTime = current_date_time.addDays(-15);
                    QDateTime filDdateTime = QDateTime::fromString(dumptime, "yyyyMMddhhmmsszzz");
                    if (filDdateTime.toString() != NULL)    //判断中间部分是否为时间格式
                    {
                        if (filDdateTime < expirationTime)
                        {
                            continue;
                        }
                        else
                        {
                            QString dumpdate = dumptime.mid(0, 8);
                            if (latestdate != "")
                            {
                                if (QDateTime::fromString(latestdate, "yyyyMMdd") < QDateTime::fromString(dumpdate, "yyyyMMdd"))    //存在多个dump日志时，对比生成的时间，取最新dump日志
                                {
                                    if (fileList[fileList.size() - 3] == dumpdate)    //判断dump日志的时间是否与所在的日期文件夹一致
                                    {
                                        latestdate = dumpdate;
                                        latestfolder = datefile;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                            }
                            else
                            {
                                if (fileList[fileList.size() - 3] == dumpdate)    //判断dump日志的时间是否与所在的日期文件夹一致
                                {
                                    latestdate = dumpdate;
                                    latestfolder = datefile;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                        }
                    }
                    else
                    {
                        qDebug()<<"名称非时间格式";
                        continue;
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }
    if (latestdate != NULL)
    {
        filePath = QFileDialog::getSaveFileName(0, tr("选择文件存储路径"), "" , "*.zip");
        if (filePath == NULL)
        {
            return;
        }
        packthead->start();
    }
    else
    {
        QMessageBox::information(0, tr("提示"), tr("最近15天未出现异常日志"), tr("确定"));
    }
}

bool OpenLog::startDateDump()  //打包主流程
{
//        qDebug()<<"最新dump日志路劲："<<latestfolder;
    QString zipfolder = newLogFolder + "/SoftwareLog/" + latestdate;
    QDir *folder = new QDir;
    if (!folder->exists(zipfolder))
    {
        return false;
    }
    else
    {
//            qDebug()<<"开始打包压缩";
        QZipWriter *writer = new QZipWriter(filePath);
        QDir log_dir(newLogFolder + "/SoftwareLog/" + latestdate + "/log");
        QFileInfoList logList = log_dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        qDebug()<<"生成log日志文件";
        for (int i = 0; i < logList.size(); i ++)
        {
            if (logList.at(i).filePath().indexOf("log") > 0 && logList.at(i).filePath().indexOf(".txt") > 0)
            {
//                    qDebug()<<logList.at(i).filePath();
                QFile file_log(logList.at(i).absoluteFilePath());
                if (file_log.open(QIODevice::ReadOnly))
                {
                    writer->addFile(latestdate + "/log/" + logList.at(i).fileName(), file_log.readAll());
                }
            }
        }

        QDir dump_dir(newLogFolder + "/SoftwareLog/" + latestdate + "/dump");
        QFileInfoList dumpList = dump_dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        qDebug()<<"生成dump日志文件";
        for (int i = 0; i < dumpList.size(); i++)
        {
            if (dumpList.at(i).filePath().indexOf("dump") > 0 && dumpList.at(i).filePath().indexOf(".log") > 0)
            {
//                    qDebug()<<dumpList.at(i).filePath();
                QFile file_log(dumpList.at(i).absoluteFilePath());
                if (file_log.open(QIODevice::ReadOnly))
                {
                    writer->addFile(latestdate + "/dump/" + dumpList.at(i).fileName(), file_log.readAll());
                }
            }
        }

        delete writer;
        writer = nullptr;
        return true;
    }

    return false;
}
#endif
#endif
