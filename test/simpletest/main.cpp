#include <QtTest>
#include <QApplication>  // 添加这行
#include "simpletest.h"

// 修改为使用 GUI 测试主函数
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // 创建 QApplication 实例
    SimpleTest test;
    return QTest::qExec(&test, argc, argv);
}
