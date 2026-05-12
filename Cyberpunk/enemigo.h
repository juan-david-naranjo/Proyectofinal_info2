#ifndef ENEMIGO_H
#define ENEMIGO_H

#include "entidadjuego.h"

// ============================================================
//  Enemigo  (clase abstracta)
//  Base para todos los enemigos del juego.
//  Define el ciclo percibir → razonar → actuar del agente.
// ============================================================
class Enemigo : public EntidadJuego
{
protected:
    float velocidad;
    float radio;       // Radio de colisión/detección base

public:
    Enemigo();
    Enemigo(float x, float y, float vel, float r);

    // ── Ciclo del agente inteligente ────────────────────────
    virtual void percibir(float jugadorX, float jugadorY) = 0;
    virtual void razonar()  = 0;
    virtual void actuar(float dt) = 0;

    // actualizar implementa el ciclo completo
    void actualizar(float dt) override;

    float getVelocidad() const { return velocidad; }

    // Hitbox del enemigo para detección de colisiones de contacto.
    // El radio se usa como mitad del lado del cuadrado bounding box.
    virtual Hitbox getHitbox() const override
    {
        return Hitbox(x - radio, y - radio, radio * 2.f, radio * 2.f);
    }

    virtual ~Enemigo() override = default;

protected:
    // Posición del jugador (actualizada en percibir)
    float jugadorPosX = 0.f;
    float jugadorPosY = 0.f;
};

#endif // ENEMIGO_H
