#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QKeyEvent>
#include <algorithm>

// ============================================================
//  Constructor
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nivelActual(1)
{
    ui->setupUi(this);

    // ── Escena ───────────────────────────────────────────────
    scena = new QGraphicsScene(this);
    ui->scene->setScene(scena);
    ui->scene->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scene->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Importante: sin transformación de alineación extra
    ui->scene->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // ── Personaje ────────────────────────────────────────────
    jugador = new Personaje(100.f, 200.f);

    // ── Niveles ──────────────────────────────────────────────
    nivel1 = new Nivel_1();
    nivel2 = new Nivel_2();

    // ── Arrancar Nivel 1 ─────────────────────────────────────
    nivel1->setScene(scena, ui->scene);
    nivel1->inicializar(jugador);

    // ── Game loop ────────────────────────────────────────────
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

// ============================================================
//  gameTick
// ============================================================
void MainWindow::gameTick()
{
    float dt = static_cast<float>(reloj.restart()) / 1000.f;
    dt = std::clamp(dt, 0.001f, 0.1f);

    if (nivelActual == 1)
    {
        nivel1->actualizar(dt);

        if (nivel1->completado)
        {
            nivelActual = 2;

            nivel1->restaurarView();
            scena->clear();

            nivel2->setScene(scena);
            nivel2->inicializar(jugador);
            jugador->cargarSpritesNivel2();

            ui->scene->setAlignment(Qt::AlignCenter);
            ui->scene->fitInView(scena->sceneRect(), Qt::KeepAspectRatio);
        }
    }
    else if (nivelActual == 2)
    {
        nivel2->actualizar(dt);
    }

    scena->update();
}

// ============================================================
//  keyPressEvent
// ============================================================
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;

    switch (event->key())
    {
    case Qt::Key_A: case Qt::Key_Left:
        jugador->keyPressed(0); break;

    case Qt::Key_D: case Qt::Key_Right:
        jugador->keyPressed(1); break;

    case Qt::Key_W: case Qt::Key_Up:
        if (nivelActual == 1) jugador->saltar();
        else                  jugador->keyPressed(2);
        break;

    case Qt::Key_Space:
        if (nivelActual == 1) jugador->saltar();
        else                  jugador->activarBoost();
        break;

    case Qt::Key_S: case Qt::Key_Down:
        if (nivelActual == 2) jugador->keyPressed(3);
        break;

    case Qt::Key_Shift:
        if (nivelActual == 2) jugador->activarDesliz();
        break;

    default: break;
    }
}

// ============================================================
//  keyReleaseEvent
// ============================================================
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;

    switch (event->key())
    {
    case Qt::Key_A: case Qt::Key_Left:  jugador->keyReleased(0); break;
    case Qt::Key_D: case Qt::Key_Right: jugador->keyReleased(1); break;
    case Qt::Key_W: case Qt::Key_Up:    jugador->keyReleased(2); break;
    case Qt::Key_S: case Qt::Key_Down:  jugador->keyReleased(3); break;
    default: break;
    }
}

// ============================================================
//  resizeEvent
//  Nivel 1: recalcular escala para que los 800px llenen el ancho.
//  Nivel 2: fitInView normal (vista cenital completa).
// ============================================================
void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (!ui->scene || !ui->scene->scene()) return;

    if (nivelActual == 1)
        nivel1->aplicarEscalaView();
    else
        ui->scene->fitInView(ui->scene->scene()->sceneRect(), Qt::KeepAspectRatio);
}
