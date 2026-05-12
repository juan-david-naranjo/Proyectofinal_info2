#include "plataforma.h"
#include <cmath>

Plataforma::Plataforma() : EntidadJuego(0, 0)
{
    ancho = 100.f;
    alto  = 20.f;
    esMovil = false;
    amplitudMovimiento = 0.f;
    velocidadMovimiento = 60.f;
    origenX = 0.f;
}

Plataforma::Plataforma(float px, float py, float w, float h, bool movil)
    : EntidadJuego(px, py)
{
    ancho = w;
    alto  = h;
    esMovil = movil;
    amplitudMovimiento = movil ? 80.f : 0.f;
    velocidadMovimiento = 60.f;
    origenX = px;

    // El item gráfico lo crea el nivel (puede ser un QGraphicsRectItem)
    // Aquí lo dejamos nulo para que el nivel lo gestione
    itemGrafico = nullptr;
}

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
