#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTimer>
#include "personaje.h"
// #include <QKeyEvent>
// #include <qgraphicsview.h>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


protected slots:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

public:


    Personaje *jugador;
    QGraphicsScene *scena;

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    QTimer* timer;


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
