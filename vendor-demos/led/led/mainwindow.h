#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QDebug>


#define LEDDEV  "/dev/leds"

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
    void on_led1_clicked(bool checked);
    void on_led2_clicked(bool checked);
    void on_led3_clicked(bool checked);
    void on_led4_clicked(bool checked);
    void on_led5_clicked(bool checked);
    void on_ledall_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    int ledfd;

};
#endif // MAINWINDOW_H
