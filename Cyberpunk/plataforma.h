#ifndef PLATAFORMA_H
#define PLATAFORMA_H

#include "entidadjuego.h"

// ============================================================
//  Plataforma
//  Objeto estático del nivel 1 (y obstáculo en nivel 2).
//  Tiene una Hitbox explícita para colisiones (sin Qt).
//  esMovil: si true, la plataforma oscila horizontalmente
//           (plataforma móvil opcional).
// ============================================================
class Plataforma : public EntidadJuego
{
public:
    float ancho;
    float alto;
    bool  esMovil;

    // Para plataformas móviles
    float amplitudMovimiento;   // px de desplazamiento máx.
    float velocidadMovimiento;  // px/s
    float origenX;              // X inicial (centro del recorrido)

    Plataforma();
    Plataforma(float x, float y, float w, float h, bool movil = false);

    // Hitbox explícita para el sistema de colisiones (sin Qt).
    // Siempre refleja la posición actual (x,y) y el tamaño de la plataforma.
    Hitbox getHitbox() const override { return Hitbox(x, y, ancho, alto); }

    // EntidadJuego::actualizar — mueve la plataforma si es móvil
    void actualizar(float dt) override;

    ~Plataforma() override = default;
};

#endif // PLATAFORMA_H
