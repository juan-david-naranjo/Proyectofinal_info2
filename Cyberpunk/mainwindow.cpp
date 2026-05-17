#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QKeyEvent>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nivelActual(2)
{
    ui->setupUi(this);

    // ── Escena gráfica ──
    scena = new QGraphicsScene(this);
    ui->scene->setScene(scena);



    ui->scene->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scene->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // ── Personaje ──
    jugador = new Personaje(100.f, 200.f);
    scena->addItem(jugador->getItem());


    // ── Niveles ──
    nivel1 = new Nivel_1();
    nivel2 = new Nivel_2();



     // nivel1->setScene(scena);

    // Iniciar nivel 1

    //nivel1->inicializar(jugador);



    // Inicializar nivel 2

    nivel2->setScene(scena);
    nivel2->inicializar(jugador);



    // ── Timer del game loop (60 fps objetivo) ──
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameTick);
    timer->start(MS_POR_TICK);
    reloj.start();
}

MainWindow::~MainWindow()
{
    delete nivel1;
    delete nivel2;
    delete jugador;
    delete ui;
}

// ── Game loop ─────────────────────────────────────────────────────────────────
void MainWindow::gameTick()
{
    // dt real en segundos (entre 1 ms y 100 ms para evitar saltos enormes)
    float dt = static_cast<float>(reloj.restart()) / 1000.f;
    dt = std::clamp(dt, 0.001f, 0.1f);

    // Actualizar nivel activo
    // if (nivelActual == 1)
    // {
    //     nivel1->actualizar(dt);

    //     // Transición al nivel 2 cuando se complete
    //     if (nivel1->completado)
    //     {
    //         nivelActual = 2;
    //         nivel2->inicializar(jugador);
    //     }
    // }
    if (nivelActual == 2)
    {
        nivel2->actualizar(dt);
    }

    scena->update();
}

// ── Input ─────────────────────────────────────────────────────────────────────
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;

    switch (event->key())
    {
    case Qt::Key_A: case Qt::Key_Left:  jugador->keyPressed(0); break;
    case Qt::Key_D: case Qt::Key_Right: jugador->keyPressed(1); break;
    case Qt::Key_W: case Qt::Key_Up:
        if (nivelActual == 1) jugador->saltar();
        else                   jugador->keyPressed(2);
        break;
    case Qt::Key_S: case Qt::Key_Down:
        if (nivelActual == 2) jugador->keyPressed(3);
        break;
    case Qt::Key_Space:
        if (nivelActual == 1) jugador->saltar();         // Doble salto
        if (nivelActual == 2) jugador->activarBoost();   // Boost de velocidad
        break;
    case Qt::Key_Shift:
        if (nivelActual == 2) jugador->activarDesliz();  // Deslizamiento
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;

    switch (event->key())
    {
    case Qt::Key_A: case Qt::Key_Left:  jugador->keyReleased(0); break;
    case Qt::Key_D: case Qt::Key_Right: jugador->keyReleased(1); break;
    case Qt::Key_W: case Qt::Key_Up:    jugador->keyReleased(2); break;
    case Qt::Key_S: case Qt::Key_Down:  jugador->keyReleased(3); break;
    }
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);  // primero el comportamiento base

    if (ui->scene && ui->scene->scene())
    {
        ui->scene->fitInView(
            ui->scene->scene()->sceneRect(),
            Qt::KeepAspectRatio   // mantiene proporciones, deja barras negras si es necesario
            );
    }
}
