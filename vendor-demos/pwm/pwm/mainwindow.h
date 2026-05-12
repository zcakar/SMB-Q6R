#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define DEVFILE "/dev/pwm"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_button0_clicked();

    void on_button1_no_clicked();

    void on_button1_off_clicked();

    void on_button3_clicked();

    void btnClicked();

private:
    Ui::MainWindow *ui;

    int fd;
};
#endif // MAINWINDOW_H
