#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    a.setApplicationName("校园活动管理系统");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("校园活动管理");
    
    // 设置样式
    a.setStyle(QStyleFactory::create("Fusion"));
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
