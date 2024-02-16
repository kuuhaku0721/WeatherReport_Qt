#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}



/*
 * 样式表  主背景

QWidget#widget{
    border-image: url(:/weaUI/weaUI.png);
    color:rgb(255, 255, 255);
}

QLabel{
    font: 25 10pt "Microsoft YaHei";
    border-radius: 4px;
    background-color: argb(60, 60, 60, 130);
    color: rgb(255, 255, 255);
}

Line{
    background-color: rgb(0, 85, 0);
}




 */
