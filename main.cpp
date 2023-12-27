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
#include "LogModule/openlog.h"

#include <QApplication>

QString softwareName = "QthTools-Matter_debug-Win";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFont font("黑体", 10);
    a.setFont(font);

    //日志模块，开始日志储存
    OpenLog *openlog = new OpenLog();
    openlog->setLog();

    MainWindow w;
    w.show();
    return a.exec();
}
