#include "botonmenu.h"
#include <QCursor>
#include <QFont>
#include <QFontMetricsF>

BotonMenu::BotonMenu(const QString& texto,
                     float          ancho,
                     float          alto,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , texto(texto)
    , ancho(ancho)
    , alto(alto)
    , hovered(false)
    , presionado(false)
{
    // Imprescindible para recibir hoverEnter/Leave
    setAcceptHoverEvents(true);


    // Cursor de mano al pasar encima (feedback visual de que es clickeable)
    setCursor(QCursor(Qt::PointingHandCursor));
}

bool BotonMenu::operator==(const BotonMenu& otro) const
{
    // Dos botones son iguales si tienen el mismo texto y el mismo tamaño
    return texto == otro.texto && ancho == otro.ancho && alto == otro.alto;
}





QRectF BotonMenu::boundingRect() const
{
    return QRectF(0, 0, ancho, alto);
}

void BotonMenu::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem*,
                      QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect(0, 0, ancho, alto);

    // ── Colores según estado ──────────────────────────────────
    QColor colorFondo, colorBorde, colorTexto;

    if (presionado)
    {
        // Click: fondo más claro, borde brillante
        colorFondo  = QColor(0, 80, 60, 230);
        colorBorde  = QColor(0, 255, 150);
        colorTexto  = QColor(0, 255, 150);
    }
    else if (hovered)
    {
        // Hover: fondo cian oscuro, borde verde brillante
        colorFondo  = QColor(0, 60, 50, 210);
        colorBorde  = QColor(0, 220, 120);
        colorTexto  = QColor(0, 255, 130);
    }
    else
    {
        // Normal: fondo casi negro, borde sutil
        colorFondo  = QColor(10, 20, 30, 200);
        colorBorde  = QColor(0, 140, 90, 180);
        colorTexto  = QColor(180, 220, 200);
    }

    // ── Fondo del botón ───────────────────────────────────────
    painter->setBrush(QBrush(colorFondo));
    painter->setPen(QPen(colorBorde, hovered ? 2.0 : 1.2));
    painter->drawRoundedRect(rect, 6, 6);

    // ── Línea decorativa izquierda (solo en hover) ────────────
    if (hovered)
    {
        painter->setPen(QPen(QColor(0, 255, 130), 3));
        painter->drawLine(QPointF(0, alto * 0.2f),
                          QPointF(0, alto * 0.8f));
    }

    // ── Texto centrado ────────────────────────────────────────
    painter->setPen(colorTexto);
    QFont font("Consolas", 16, hovered ? QFont::Bold : QFont::Normal);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, texto);
}

// ── Eventos del ratón ─────────────────────────────────────────
void BotonMenu::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        presionado = true;
        update();   // redibujar para mostrar estado "presionado"
    }
    e->accept();
}

void BotonMenu::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && presionado)
    {
        presionado = false;
        update();

        // Solo emitir si el ratón sigue dentro del botón
        if (boundingRect().contains(e->pos()))
            emit clicked();
    }
    e->accept();

}

void BotonMenu::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    hovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(e);
}

void BotonMenu::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    hovered    = false;
    presionado = false;   // por si arrastra el ratón fuera
    update();
    QGraphicsObject::hoverLeaveEvent(e);
}
