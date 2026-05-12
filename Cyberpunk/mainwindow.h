#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTimer>
#include <QElapsedTimer>
#include "personaje.h"
#include "nivel_1.h"
#include "nivel_2.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event)   override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void gameTick();

private:
    Ui::MainWindow *ui;

    Personaje*      jugador;
    QGraphicsScene* scena;
    QTimer*         timer;
    QElapsedTimer   reloj;      // Para calcular dt real entre ticks

    Nivel_1*        nivel1;
    Nivel_2*        nivel2;
    int             nivelActual;  // 1 o 2

    static constexpr int FPS_OBJETIVO = 60;
    static constexpr int MS_POR_TICK  = 1000 / FPS_OBJETIVO;  // ~16 ms
};

#endif // MAINWINDOW_H
