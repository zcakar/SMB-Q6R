#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    fd = open(DEVFILE, O_RDONLY|O_NONBLOCK);
    if(fd < 0)
    {
        qDebug()<<DEVFILE<<" open errer.";
        exit(0);
    }
    fd_sock = new QSocketNotifier(fd,QSocketNotifier::Read, this);

    connect(fd_sock, SIGNAL(activated(int)), this, SLOT(getValue()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getValue()
{
    char buttonValue[9]={'\0','\0','\0','\0','\0','\0','\0','\0','\0'};
    QPixmap pixmap;

    if(read(fd,buttonValue,sizeof(buttonValue)) != (sizeof(buttonValue)-1))
    {
        pixmap.load(":/icon/button_error.png");
        ui->label_5->setPixmap(pixmap);
        return;
    }

    ui->label_4->setText(QString(&buttonValue[0]));

    if(buttonValue[0] == '1' && buttonValue[1] == '1')
    {
        pixmap.load(":/icon/button_open.png");
        ui->label_5->setPixmap(pixmap);
    }
    else
    {
        pixmap.load(":/icon/button_close.png");
        ui->label_5->setPixmap(pixmap);
    }

}
