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
#ifndef MYTHEAD_H
#define MYTHEAD_H

#include <QObject>
#include <QThread>
#include <iostream>

class OpenLog;
class fileThread: public QThread
{
    Q_OBJECT
public:
    fileThread();
    OpenLog *openlog_file;
    void run();
    void getlogfile(QString file);
};

class timeThread: public QThread
{
    Q_OBJECT
public:
    timeThread();
    OpenLog *openlog_time;
    void run();
};

class packThead: public QThread
{
    Q_OBJECT
public:
    packThead();
    OpenLog *openlog_pack;
    void run();
    void getDumpfile(QString file);

signals:
    void cpDateDumpIsNullSignal();
};
#endif // MYTHEAD_H
