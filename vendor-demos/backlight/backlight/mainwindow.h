#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <QMouseEvent>


#define DEVFILE "/sys/class/backlight/backlight/brightness"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void mousePressEvent(QMouseEvent *event);


private slots:
    void btn_slot();

private:
    Ui::MainWindow *ui;
    int fd;
    char buffer[8];
    int value;
};

#endif // MAINWINDOW_H
