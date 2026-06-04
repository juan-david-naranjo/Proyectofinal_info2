#include "plataforma.h"
#include <cmath>
#include <algorithm>   // std::min

Plataforma::Plataforma() : EntidadJuego(0, 0)
{
    ancho = 100.f;
    alto  = 20.f;
    esMovil = false;
    amplitudMovimiento = 0.f;
    velocidadMovimiento = 60.f;
    origenX = 0.f;
    tipoMuro            = TipoMuro::SIN_SPRITE;
}

Plataforma::Plataforma(float px, float py, float w, float h, bool movil,TipoMuro tipo)
    : EntidadJuego(px, py)
{
    ancho = w;
    alto  = h;
    esMovil = movil;
    tipoMuro            = tipo;
    amplitudMovimiento = movil ? 80.f : 0.f;
    velocidadMovimiento = 60.f;
    origenX = px;


    // El item gráfico lo crea el nivel (puede ser un QGraphicsRectItem)
    // Aquí lo dejamos nulo para que el nivel lo gestione
    itemGrafico = nullptr;
}

//----------------- Sobrecarga Obligatoria ----------------------
Plataforma::Plataforma(const Plataforma& otro)
    : EntidadJuego(otro)
    , ancho(otro.ancho)
    , alto(otro.alto)
    , esMovil(otro.esMovil)
    , tipoMuro(otro.tipoMuro)
    , amplitudMovimiento(otro.amplitudMovimiento)
    , velocidadMovimiento(otro.velocidadMovimiento)
    , origenX(otro.origenX)
    , laberintos(otro.laberintos)    // QPixmap usa copy-on-write, es seguro
// itemGrafico = nullptr (herencia)
{}

bool Plataforma::operator==(const Plataforma& otro) const
{
    // Iguales si tienen la misma posición y dimensiones
    return x == otro.x && y == otro.y
           && ancho == otro.ancho && alto == otro.alto;
}

//--------------------------------------------------------------
// getHitbox() está definido inline en plataforma.h — devuelve Hitbox(x, y, ancho, alto).
// No se necesita getBoundingBox() con QRectF: el sistema de físicas usa Hitbox directamente.

// Movimiento oscilatorio de plataforma móvil
// x(t) = origenX + A·sin(velocidad·t)   [aquí simplificado con acumulación de Vx]
void Plataforma::actualizar(float dt)
{
    if (!esMovil) return;

    // Movimiento usando la Vx heredada como "tiempo acumulado del seno"
    Vx += velocidadMovimiento * dt;   // Vx actúa como fase acumulada
    x = origenX + amplitudMovimiento * std::sin(Vx / velocidadMovimiento * M_PI);

    if (itemGrafico)
        itemGrafico->setPos(x, y);
}


// ── cargarSprite ──────────────────────────────────────────────────────────────
// Misma lógica que cargarSpritesNivel2 en Personaje:
// recibe el sprite base, lo tilea al tamaño exacto del muro
// y crea el itemGrafico listo para la escena.
void Plataforma::cargarSprite(const QPixmap& hoja,
                              int srcX, int srcY,
                              int srcW, int srcH)
{

    if (tipoMuro == TipoMuro::SIN_SPRITE || hoja.isNull()) return;

    // Recortar el sprite de la hoja — igual que en personaje.cpp
    QPixmap spriteBase = hoja.copy(srcX, srcY, srcW, srcH);
    if (spriteBase.isNull()) return;

    int w = static_cast<int>(ancho);
    int h = static_cast<int>(alto);

    QPixmap tileado = tilearSprite(spriteBase, w, h);
    if (tileado.isNull()) return;

    if (!itemGrafico)
        itemGrafico = new QGraphicsPixmapItem();

    itemGrafico->setPixmap(tileado);
    itemGrafico->setPos(x, y);
    itemGrafico->setZValue(1.0);
}



// ── tilearSprite ──────────────────────────────────────────────────────────────
// Repite fuente en una cuadrícula hasta cubrir anchoFinal × altoFinal.
// Los tiles del borde se recortan si no caben completos.
// No usa QVector ni QList — solo QPixmap y QPainter.
QPixmap Plataforma::tilearSprite(const QPixmap& fuente,
                                 int anchoFinal, int altoFinal)
{
    if (fuente.isNull() || anchoFinal <= 0 || altoFinal <= 0)
        return QPixmap();

    QPixmap resultado(anchoFinal, altoFinal);
    resultado.fill(Qt::transparent);

    QPainter p(&resultado);
    int tw = fuente.width();
    int th = fuente.height();

    for (int gy = 0; gy < altoFinal; gy += th)
    {
        for (int gx = 0; gx < anchoFinal; gx += tw)
        {
            // Recortar el tile si se sale del borde derecho o inferior
            int drawW = std::min(tw, anchoFinal - gx);
            int drawH = std::min(th, altoFinal  - gy);

            p.drawPixmap(gx, gy,          // destino en el resultado
                         fuente,           // fuente
                         0, 0,             // origen dentro del tile
                         drawW, drawH);    // tamaño a copiar
        }
    }
    p.end();
    return resultado;
}