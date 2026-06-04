#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QKeyEvent>

// ============================================================
//  MainWindow — solo configura la ventana y delega a GameManager
//
//  Toda la lógica del juego (loop, niveles, menú, pausa)
//  vive en GameManager.
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ── Configurar la vista ───────────────────────────────────
    scena = new QGraphicsScene(this);
    ui->scene->setScene(scena);
    ui->scene->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scene->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scene->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // ── Crear el gestor y arrancar ────────────────────────────
    gameManager = new GameManager(scena, ui->scene, this);
    gameManager->iniciarJuego();   // ← única llamada al juego
}

MainWindow::~MainWindow()
{
    // 1. PRIMERO matamos al GameManager.
    // Como la escena todavía está viva, el GameManager puede limpiar
    // su overlay, sus robots y sus personajes de la escena sin provocar un Crash.
    if (gameManager != nullptr) {
        delete gameManager;
        gameManager = nullptr;
    }

    // 2. AHORA SÍ, con la escena completamente vacía y segura, la destruimos.
    if (scena != nullptr) {
        delete scena;
        scena = nullptr;
    }

    // 3. Por último, destruimos la interfaz visual.
    delete ui;
}

// ── Reenviar teclado al GameManager ──────────────────────────
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    gameManager->keyPressed(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    gameManager->keyReleased(event);
}

// ── Resize: delegar al GameManager según el estado ───────────
void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (gameManager) gameManager->aplicarEscala();
}