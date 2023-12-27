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
#include "logthread.h"
#include "openlog.h"

#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QDateTime>

static QString logfile = "";
QString oldDate = "";

fileThread::fileThread()
{

}

timeThread::timeThread()
{

}

packThead::packThead()
{

}

void fileThread::getlogfile(QString file)
{
    logfile = file;
}

void fileThread::run()  //文件线程
{
    while (1)
    {
        QFileInfo info(logfile);
        if (info.exists())
        {
            if (info.size() > 1024 * 1024 * 50)
            {
                openlog_file = new OpenLog();
                openlog_file->openNewLog();
            }
        }
        sleep(60);
    }
}

void timeThread::run()  //时间判断线程
{
    QDateTime date_time = QDateTime::currentDateTime();
    oldDate = date_time.toString("yyyyMMdd");
    while(1)
    {
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyyMMdd");
        if (current_date != oldDate)
        {
            qDebug()<<"Tomorrow";
            openlog_time = new OpenLog();
            openlog_time->openNewLog();
            oldDate = current_date;
        }
        sleep(60*10);
    }
}

void packThead::run()  //打包线程
{
    openlog_pack = new OpenLog();
    openlog_pack->startDateDump();
}
