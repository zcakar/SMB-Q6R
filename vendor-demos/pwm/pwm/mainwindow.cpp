#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    fd=open(DEVFILE, O_RDONLY);
    if (fd < 0)
    {
        perror("open error");
        exit(-1);
    }

    connect(ui->pushButton_0, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_4, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_6, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_7, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_8, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_9, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_back, SIGNAL(clicked()), this, SLOT(btnClicked()));
    connect(ui->pushButton_clear, SIGNAL(clicked()), this, SLOT(btnClicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_button0_clicked()
{
    ioctl(fd, 0, 1);
}

void MainWindow::on_button1_no_clicked()
{
    ioctl(fd, 1, 1);
}

void MainWindow::on_button1_off_clicked()
{
    ioctl(fd, 1, 0);
}

void MainWindow::on_button3_clicked()
{
    int value=ui->lineEdit->text().toInt();
    ioctl(fd, 3, value);
}

void MainWindow::btnClicked()
{
    QPushButton *btn=qobject_cast <QPushButton *>(sender());

    QString string=btn->objectName();
    int ret=string.indexOf("back");

    if(ret > 0)
    {
        QString str=ui->lineEdit->text();

        int len=str.length();
        if(len > 0)
        {
            str.remove(len-1, 1);
            ui->lineEdit->setText(str);
        }
        return;
    }

    ret = 0;
    ret=string.indexOf("clear");

    if(ret > 0)
        {
            ui->lineEdit->clear();
            return;
        }

    ui->lineEdit->setText(ui->lineEdit->text().append(btn->text()));
}
