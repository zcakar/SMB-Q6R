#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    ledfd = open(LEDDEV, O_WRONLY);
    if (ledfd < 1)
    {
        qDebug()<<"/dev/leds open error.";
        exit(-1);
    }
}

MainWindow::~MainWindow()
{
    ::close(ledfd);
    delete ui;
}



void MainWindow::on_led1_clicked(bool checked)
{
    if(checked)
    {
        ioctl(ledfd, 1, 3);
    }
    else
    {
        ioctl(ledfd, 0, 3);
    }
}

void MainWindow::on_led2_clicked(bool checked)
{
    if(checked)
    {
        ioctl(ledfd, 1, 4);
    }
    else
    {
        ioctl(ledfd, 0, 4);
    }
}

void MainWindow::on_led3_clicked(bool checked)
{
    if(checked)
    {
        ioctl(ledfd, 1, 2);
    }
    else
    {
        ioctl(ledfd, 0, 2);
    }
}

void MainWindow::on_led4_clicked(bool checked)
{
    if(checked)
    {
        ioctl(ledfd, 1, 1);
    }
    else
    {
        ioctl(ledfd, 0, 1);
    }
}

void MainWindow::on_led5_clicked(bool checked)
{
    if(checked)
    {
        ioctl(ledfd, 1, 0);
    }
    else
    {
        ioctl(ledfd, 0, 0);
    }
}

void MainWindow::on_ledall_clicked(bool checked)
{
    on_led1_clicked(checked);
    on_led2_clicked(checked);
    on_led3_clicked(checked);
    on_led4_clicked(checked);
    on_led5_clicked(checked);
}
