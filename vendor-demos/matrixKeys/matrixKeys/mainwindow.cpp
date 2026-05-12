#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    fd=open(DEVFILE, O_RDONLY|O_NONBLOCK);
    if(fd < 0)
    {
        qDebug()<<"/dev/input/event1 open fail.";
        exit(0);
    }
    matrix_sock = new QSocketNotifier(fd,QSocketNotifier::Read, this);

    connect(matrix_sock, SIGNAL(activated(int)), this, SLOT(getKeyMatrixValue()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent  *event)
{
    //qDebug()<<event->key();
    //qDebug() << event->text();
    ui->label_5->setText(QString::number(event->key()));

}

void MainWindow::getKeyMatrixValue()
{
    struct input_event inputEvent;
    unsigned int bytesRead = read(fd, &inputEvent, sizeof(inputEvent));

    if(bytesRead < sizeof(input_event))
    {
        return;
    }

    if((__u16)1 == inputEvent.type)
    {
        ui->label_4->setText(QString::number(inputEvent.code));
    }

}
