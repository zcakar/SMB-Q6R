#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    fd = open(DEVFILE, O_RDWR|O_CREAT);
    if(fd < 0)
    {
        qDebug()<<"open /sys/class/backlight/backlight/brightness err.";
        exit(-1);
    }

    memset(&buffer, '\0', 8*sizeof(char));

    int i=0,len=0;
    len=read(fd, &buffer, 8*sizeof(char));
    value =0;

    for(i=0;i<len-1;i++)
    {
        value =10*value+int(buffer[i] - '0');
    }

    ui->spinBox->setValue(value);

    connect(ui->button_0, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_1, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_2, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_3, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_4, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_5, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_6, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_7, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_8, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_9, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_clear, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));
    connect(ui->button_det, SIGNAL(clicked(bool)), this, SLOT(btn_slot()));

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::btn_slot()
{
    QPushButton *btn=qobject_cast <QPushButton *>(sender());

    QString val=(btn->objectName()).right(1);


    if(val == "r")
    {
        ui->spinBox->clear();
    }
    else if(val == "t")
    {
        QString str=ui->spinBox->text();
        write(fd, str.toLatin1().data(), str.length());
    }
    else
    {
        QString tmp = ui->spinBox->text(); 
        int tmpval = tmp.toInt() *10 + val.toInt();
        ui->spinBox->setValue(tmpval);
        value = tmpval;
    }

}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (value == 0)
    {
        ui->spinBox->setValue(100);
        value = 100;
        ui->button_det->click();
    }
}
