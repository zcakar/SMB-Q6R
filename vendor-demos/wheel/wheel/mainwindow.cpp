#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    pixmap.load(":/icon/wheel_no.png");
    ui->label_4->setPixmap(pixmap);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if(event->delta() > 0){                    // 当滚轮远离使用者时,进行放大
        qDebug()<<"big";
        ui->label_3->setText("向上");
        pixmap.load(":/icon/wheel_up.png");
        ui->label_4->setPixmap(pixmap);
    }else{                                     // 当滚轮向使用者方向旋转时,进行缩小
        qDebug()<<"small";
        ui->label_3->setText("向下");
        pixmap.load(":/icon/wheel_down.png");
        ui->label_4->setPixmap(pixmap);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    ui->label_3->setText("按压");
    pixmap.load(":/icon/wheel_left.png");
    ui->label_4->setPixmap(pixmap);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    ui->label_3->setText("释放");
    pixmap.load(":/icon/wheel_no.png");
    ui->label_4->setPixmap(pixmap);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug()<<"双击";
}
