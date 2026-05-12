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
    ::close(fd);
    delete fd_sock;
}


void MainWindow::getValue()
{
    char buttonValue[8]={'0','0','0','0','0','0','0','0'};
    unsigned int i = 0;
    QPixmap pixmap;

    if(read(fd,buttonValue,sizeof(buttonValue)) != sizeof(buttonValue))
    {
        return;
    }

    for(i = 0; i < sizeof(buttonValue); i++)
    {
        if(buttonValue[i]=='1')
        {
            switch(i)
            {
            case 3:
                ui->label_3->setText("右档");
                pixmap.load(":/icon/knob_right.png");
                ui->label_5->setPixmap(pixmap);
                break;
            case 4:
                ui->label_3->setText("左档");
                pixmap.load(":/icon/knob_left.png");
                ui->label_5->setPixmap(pixmap);
                break;
            case 5:
                ui->label_3->setText("中档");
                pixmap.load(":/icon/knob_center.png");
                ui->label_5->setPixmap(pixmap);
                break;
            default:
                ui->label_3->setText("错误");
                pixmap.load(":/icon/knob_error.png");
                ui->label_5->setPixmap(pixmap);
                break;
            }
        }
    }

}
