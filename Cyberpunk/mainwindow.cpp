#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    jugador   =   new Personaje(123,123);
    scena = new QGraphicsScene();
    timer= new QTimer();


    scena->addItem(jugador->getItem());
    ui->scene->setScene(scena);
    // scena->addItem(personaje);




    ui->scene->setScene(scena);


    connect(timer,SIGNAL(timeout()),this,SLOT(keyPressEvent()));
    connect(timer,SIGNAL(timeout()),this,SLOT(keyReleaseEvent()));
    timer->start(100);

}

MainWindow::~MainWindow()
{
    delete ui;
}

//-------------FUNCIONES PRUEBA-------------
void MainWindow::keyPressEvent(QKeyEvent* event)  {
    if (!jugador) return;
    switch(event->key()) {
    case Qt::Key_A:  jugador->mover(0, true); break;
    case Qt::Key_D: jugador->mover(1, true); break;
    case Qt::Key_W:    jugador->saltar(); break;
    case Qt::Key_S:  jugador->bajar(); break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (!jugador) return;
    switch(event->key()) {
    case Qt::Key_A:  jugador->mover(0, true); break;
    case Qt::Key_D: jugador->mover(1, true); break;
    case Qt::Key_W:    jugador->saltar(); break;
    case Qt::Key_S:  jugador->bajar(); break;
    }
}