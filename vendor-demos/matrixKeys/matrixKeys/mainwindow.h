#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QDebug>
#include <unistd.h>
#include <fcntl.h>
#include <QSocketNotifier>
#include <linux/input.h>

#define DEVFILE "/dev/input/event1"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void keyPressEvent(QKeyEvent  *event);


private slots:
    void getKeyMatrixValue();


private:
    Ui::MainWindow *ui;

    int fd;
    QSocketNotifier* matrix_sock;
};
#endif // MAINWINDOW_H
