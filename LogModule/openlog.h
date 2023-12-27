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
#ifndef OPENLOG_H
#define OPENLOG_H

#include <QString>
#include <QObject>

#define SAVELOG    //日志保存宏开关
#define OPENDUMP    //dump日志宏开关

class OpenLog : public QObject
{
    Q_OBJECT
public:
    explicit OpenLog(QString logfolder = nullptr);
    ~OpenLog();
    void setLog();
    void openNewLog();
    bool startDateDump();
#ifdef SAVELOG
    void cpDateLog();
    void cpDateDump();
#endif

private slots:
    void cpDateDumpIsNullSlot();

private:
    int firsttime = 0;
};

#endif // OPENLOG_H
