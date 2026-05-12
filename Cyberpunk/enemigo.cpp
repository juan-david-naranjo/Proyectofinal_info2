#include "enemigo.h"

Enemigo::Enemigo() : EntidadJuego(0, 0), velocidad(60.f), radio(30.f) {}

Enemigo::Enemigo(float px, float py, float vel, float r)
    : EntidadJuego(px, py), velocidad(vel), radio(r) {}

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
