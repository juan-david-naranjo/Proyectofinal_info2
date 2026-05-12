#include "robotseguridad.h"
#include "gestorfisicas.h"
#include <cmath>
#include <algorithm>

RobotSeguridad::RobotSeguridad(float px, float py,
                               float rDeteccion,
                               float rDesenganche,
                               float vPatrulla,
                               float vPersecucion,
                               const std::vector<Punto2D> &wps)
    : Enemigo(px, py, vPatrulla, rDeteccion)
    , estado(EstadoAgente::PATRULLAJE)
    , radioDeteccion(rDeteccion)
    , radioDesenganche(rDesenganche)
    , velPatrulla(vPatrulla)
    , velPersecucion(vPersecucion)
    , waypoints(wps)
{
    // El ítem gráfico lo asigna el nivel
    itemGrafico = nullptr;

    // Si no hay waypoints, el robot se queda quieto en su posición
    if (waypoints.empty())
        waypoints.push_back(Punto2D(px, py));
}

// ── PERCIBIR ─────────────────────────────────────────────────────────────────
// Calcula la distancia al jugador y actualiza la posición conocida.
void RobotSeguridad::percibir(float jx, float jy)
{
    jugadorPosX  = jx;
    jugadorPosY  = jy;
    float dx     = jx - x;
    float dy     = jy - y;
    distanciaJugador = std::sqrt(dx*dx + dy*dy);
}

float RobotSeguridad::calcularDistancia() const
{
    return distanciaJugador;
}

// ── RAZONAR ──────────────────────────────────────────────────────────────────
// Evalúa el estado actual y decide la transición.
void RobotSeguridad::razonar()
{
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
        // Transición a PERSECUCION si el jugador entra en el radio
        if (GestorFisicas::colisionCirculo(x, y,
                                           jugadorPosX, jugadorPosY,
                                           radioDeteccion))
        {
            estado = EstadoAgente::PERSECUCION;
            tiempoPersecucion = 0.f;
            // Guardar posición en historial (aprendizaje)
            actualizarWaypoints();
        }
        break;

    case EstadoAgente::PERSECUCION:
        // Solo puede volver a patrullaje si pasó el tiempo mínimo Y
        // el jugador salió del radio de desenganche
        if (tiempoPersecucion >= DURACION_MIN_PERSECUCION &&
            !GestorFisicas::colisionCirculo(x, y,
                                            jugadorPosX, jugadorPosY,
                                            radioDesenganche))
        {
            estado = EstadoAgente::PATRULLAJE;
            // El próximo waypoint será la última posición guardada
        }
        break;
    }
}

// ── ACTUAR ───────────────────────────────────────────────────────────────────
// Ejecuta el movimiento según el estado.
void RobotSeguridad::actuar(float dt)
{
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
    {
        // Seguir waypoints en orden circular (usando Punto2D, sin Qt)
        Punto2D objetivo = waypoints[indiceWaypoint];
        float dx = objetivo.x - x;
        float dy = objetivo.y - y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 8.f)
        {
            // Llegó al waypoint → avanzar al siguiente
            indiceWaypoint = (indiceWaypoint + 1) % static_cast<int>(waypoints.size());
        }
        else
        {
            moverHacia(objetivo.x, objetivo.y, dt);
        }
        break;
    }

    case EstadoAgente::PERSECUCION:
    {
        tiempoPersecucion += dt;
        // Guardar la última posición vista (aprendizaje continuo)
        if (historial.empty() ||
            std::abs(jugadorPosX - historial.back().x) > 20.f ||
            std::abs(jugadorPosY - historial.back().y) > 20.f)
        {
            historial.push_back(Punto2D(jugadorPosX, jugadorPosY));
        }
        // Perseguir con velocidad aumentada
        moverHacia(jugadorPosX, jugadorPosY, dt);
        break;
    }
    }

    // Sincronizar sprite
    if (itemGrafico)
        itemGrafico->setPos(x, y);
}

// ── TICK (método completo para el nivel) ─────────────────────────────────────
void RobotSeguridad::tick(float jx, float jy, float dt)
{
    percibir(jx, jy);
    razonar();
    actuar(dt);
}

// ── APRENDIZAJE: actualizar waypoints ────────────────────────────────────────
// Añade la última posición vista del jugador al patrón de vigilancia.
void RobotSeguridad::actualizarWaypoints()
{
    Punto2D nuevoPunto(jugadorPosX, jugadorPosY);

    // Evitar duplicados muy cercanos (usando Punto2D, sin Qt)
    for (const Punto2D &wp : waypoints)
    {
        float dx = wp.x - nuevoPunto.x;
        float dy = wp.y - nuevoPunto.y;
        if (std::sqrt(dx*dx + dy*dy) < 40.f) return;
    }

    // Insertar después del waypoint actual para que lo visite pronto
    auto it = waypoints.begin() + ((indiceWaypoint + 1) % waypoints.size());
    waypoints.insert(it, nuevoPunto);
}

// ── MOVIMIENTO CON INERCIA ───────────────────────────────────────────────────
// Mueve el robot hacia (tx, ty) usando MRUA en ambos ejes.
void RobotSeguridad::moverHacia(float tx, float ty, float dt)
{
    float velActual = (estado == EstadoAgente::PERSECUCION)
                      ? velPersecucion
                      : velPatrulla;

    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist < 1.f) { Vx = 0; Vy = 0; return; }

    // Dirección normalizada
    float nx = dx / dist;
    float ny = dy / dist;

    float targetVx = nx * velActual;
    float targetVy = ny * velActual;

    // Aplicar inercia (MRUA)
    GestorFisicas::aplicarInercia(Vx, targetVx, dt);
    GestorFisicas::aplicarInercia(Vy, targetVy, dt);

    x += Vx * dt;
    y += Vy * dt;
}
