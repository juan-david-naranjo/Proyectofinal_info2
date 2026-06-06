#ifndef BOTONMENU_H
#define BOTONMENU_H

#include <QGraphicsObject>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QString>


#include <QGraphicsSceneHoverEvent>
#include <QBrush>

// ============================================================
//  BotonMenu
//
//  Botón interactivo para usar dentro de QGraphicsScene.
//  Extiende QGraphicsObject para poder emitir señales Qt.
//
//  Características:
//    - Efecto hover: cambia color al pasar el ratón encima
//    - Emite clicked() al hacer clic
//    - Totalmente reutilizable (menú, pausa, victoria, etc.)
//
//  Uso:
//    BotonMenu* btn = new BotonMenu("CONTINUAR", 300, 50);
//    btn->setPos(x, y);
//    escena->addItem(btn);
//    connect(btn, &BotonMenu::clicked, this, &GameManager::reanudar);
// ============================================================
class BotonMenu : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit BotonMenu(const QString& texto,
                       float          ancho  = 320.f,
                       float          alto   = 52.f,
                       QGraphicsItem* parent = nullptr);

    // ── QGraphicsItem obligatorios ────────────────────────────
    QRectF boundingRect() const override;
    void   paint(QPainter* painter,
                 const QStyleOptionGraphicsItem*,
                 QWidget*) override;
    bool operator==(const BotonMenu& otro) const;   // -> Sobrecarga Obligatoria

signals:
    void clicked();   // emitida al soltar el botón con el ratón encima

protected:
    void mousePressEvent   (QGraphicsSceneMouseEvent* e) override;
    void mouseReleaseEvent (QGraphicsSceneMouseEvent* e) override;
    void hoverEnterEvent   (QGraphicsSceneHoverEvent* e) override;          //estas funciones son propias de qt las sobreescribimos para actualizar las respuestas
    void hoverLeaveEvent   (QGraphicsSceneHoverEvent* e) override;




private:
    QString texto;
    float   ancho;
    float   alto;
    bool    hovered;    // ratón encima
    bool    presionado; // botón siendo pulsado
    BotonMenu(const BotonMenu&)            = delete;  // -> Sobrecarga Obligatoria QObject impide copiar
    BotonMenu& operator=(const BotonMenu&) = delete;  // -> Regla de los Tres: bloquear asignación
};

#endif // BOTONMENU_H
