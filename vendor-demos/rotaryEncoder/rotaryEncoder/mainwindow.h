#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtDebug>
#include <QSocketNotifier>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>


#define DEVFILE     "/dev/buttons"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void getValue();

private:
    Ui::MainWindow *ui;

    int fd;

    QSocketNotifier* fd_sock;

};
#endif // MAINWINDOW_H
