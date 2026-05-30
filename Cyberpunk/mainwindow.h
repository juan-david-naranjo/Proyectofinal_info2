#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "gamemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ============================================================
//  MainWindow — responsabilidad única: ser la ventana
//
//  No sabe nada del juego. Solo:
//    1. Crea la escena Qt
//    2. Crea GameManager y llama iniciarJuego()
//    3. Reenvía eventos de teclado y resize a GameManager
// ============================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent  (QKeyEvent*   event) override;
    void keyReleaseEvent(QKeyEvent*   event) override;
    void resizeEvent    (QResizeEvent* event) override;

private:
    Ui::MainWindow* ui;
    QGraphicsScene* scena;
    GameManager*    gameManager;
};

#endif // MAINWINDOW_H
