#include "enemigo.h"

Enemigo::Enemigo() : EntidadJuego(0, 0), velocidad(60.f), radio(30.f) {}

Enemigo::Enemigo(float px, float py, float vel, float r)
    : EntidadJuego(px, py), velocidad(vel), radio(r) {}

Enemigo::Enemigo(const Enemigo& otro)
    : EntidadJuego(otro)
    , velocidad(otro.velocidad)
    , radio(otro.radio)
    , jugadorPosX(otro.jugadorPosX)
    , jugadorPosY(otro.jugadorPosY)
{}

bool Enemigo::operator==(const Enemigo& otro) const
{
    // Iguales si están en la misma posición con la misma velocidad base
    return x == otro.x && y == otro.y && velocidad == otro.velocidad;
}

// ============================================================
//  Operador de asignación — Regla de los Tres
//  Enemigo no posee recursos dinámicos propios (itemGrafico
//  pertenece a la escena Qt y se gestiona en EntidadJuego).
//  Se delega en la base y se copian los campos propios.
// ============================================================
Enemigo& Enemigo::operator=(const Enemigo& otro)
{
    if (this == &otro) return *this;
    EntidadJuego::operator=(otro);   // posición, velocidad, activa (itemGrafico ignorado)
    velocidad    = otro.velocidad;
    radio        = otro.radio;
    jugadorPosX  = otro.jugadorPosX;
    jugadorPosY  = otro.jugadorPosY;
    return *this;
}


// El ciclo del agente: percibir → razonar → actuar
// Las subclases implementan cada paso; aquí se orquesta.
// NOTA: percibir() necesita las coordenadas del jugador, que el nivel
// debe pasar antes de llamar a actualizar().
// Se llama a actuar(dt) con el dt del tick.
void Enemigo::actualizar(float dt)
{
    // El nivel actualiza jugadorPosX/Y y llama a percibir+razonar+actuar
    // directamente para poder pasar la posición del jugador.
    // Este método queda como fallback no-op.
    (void)dt;
}
