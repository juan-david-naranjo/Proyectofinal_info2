#include "nivel.h"
#include "gestorfisicas.h"

Nivel::Nivel()
    : tiempoRestante(60)
    , completado(false)
    , tiempoAcumulado(0.f)
    , jugador(nullptr)
{}

Nivel::~Nivel()
{
    limpiarPlataformas();
}

Nivel::Nivel(const Nivel& otro)
    : tiempoRestante(otro.tiempoRestante)
    , completado(otro.completado)
    , tiempoAcumulado(otro.tiempoAcumulado)
    , jugador(otro.jugador)    // puntero compartido (el jugador no se duplica)
{
    // Copia profunda de plataformas: cada Nivel tiene sus propios obstáculos
    for (Plataforma* plat : otro.plataformas)
        plataformas.push_back(new Plataforma(*plat));
}

bool Nivel::operator==(const Nivel& otro) const
{
    // Iguales si tienen el mismo tiempo restante y mismo estado de completitud
    return tiempoRestante == otro.tiempoRestante
           && completado == otro.completado;
}

// ============================================================
//  Operador de asignación — Regla de los Tres
//  Hace deep copy de las plataformas (heap) y copia el puntero
//  compartido al jugador (el jugador no pertenece al nivel).
// ============================================================
Nivel& Nivel::operator=(const Nivel& otro)
{
    if (this == &otro) return *this;

    limpiarPlataformas();   // liberar plataformas actuales

    tiempoRestante  = otro.tiempoRestante;
    completado      = otro.completado;
    tiempoAcumulado = otro.tiempoAcumulado;
    jugador         = otro.jugador;         // puntero compartido, no se duplica

    for (Plataforma* plat : otro.plataformas)
        plataformas.push_back(new Plataforma(*plat));

    return *this;
}




// Resuelve colisiones del personaje contra todas las plataformas del nivel.
// Se llama DESPUÉS de mover al personaje (en actualizar).
// Usa Hitbox explícitas: no depende de Qt para la detección de colisiones.
void Nivel::resolverColisiones()
{
    if (!jugador) return;

    // Obtener hitbox actual del personaje (posición + tamaño reales)
    Hitbox hbJugador = jugador->getHitbox();
    float  vx        = jugador->getVx();
    float  vy        = jugador->getVy();
    bool   enSuelo   = false;

    for (Plataforma* plat : plataformas)
    {
        if (!plat) continue;

        // Hitbox de la plataforma (actualizada en cada tick para las móviles)
        Hitbox hbPlat = plat->getHitbox();

        bool toco = GestorFisicas::resolverColision(
                        hbJugador,   // se modifica si hay colisión
                        vx, vy, enSuelo,
                        hbPlat);

        if (toco)
        {
            // Propagar posición resuelta de la hitbox al personaje
            jugador->setPosicion(hbJugador.x - jugador->getHitboxOffsetX(),
            hbJugador.y - jugador->getHitboxOffsetY());
            jugador->setVelocidad(vx, vy);
            if (enSuelo)
                jugador->aterrizarEnSuelo(plat->getY());
        }
    }

    if (!enSuelo && !jugador->isEnSuelo())
        jugador->despegarSuelo();
}

void Nivel::limpiarPlataformas()
{
    for (Plataforma* p : plataformas) delete p;
    plataformas.clear();
}
