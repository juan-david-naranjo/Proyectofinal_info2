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
    delete ui;
    // gameManager es hijo de this (QObject parent), Qt lo destruye solo
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