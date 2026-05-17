#include "gestorfisicas.h"
#include <cmath>
#include <algorithm>

// ── Nivel 1: Gravedad ────────────────────────────────────────────────────────
// Aplica aceleración gravitatoria a Vy y desplaza Y.
// Vy se satura al llegar a VEL_MAX_CAIDA (velocidad terminal).
void GestorFisicas::aplicarGravedad(float &vy, float &y, float dt)
{
    vy += GRAVEDAD * dt;
    if (vy > VEL_MAX_CAIDA) vy = VEL_MAX_CAIDA;
    y += vy * dt;
}

// ── Nivel 1: Viento oscilatorio ──────────────────────────────────────────────
// Devuelve la fuerza horizontal del viento usando movimiento oscilatorio:
//   F(t) = A · sin(2π · f · t)
// Positivo empuja a la derecha, negativo a la izquierda.
float GestorFisicas::calcularFuerzaViento(float tiempoTotal)
{
    return VIENTO_AMPLITUD * std::sin(2.f * M_PI * VIENTO_FRECUENCIA * tiempoTotal);
}

// Aplica la fuerza del viento como aceleración sobre Vx.
void GestorFisicas::aplicarViento(float &vx, float tiempoTotal, float dt)
{
    float fuerza = calcularFuerzaViento(tiempoTotal);
    vx += fuerza * dt;
}

// ── Nivel 1: Fricción horizontal ─────────────────────────────────────────────
// Multiplica Vx por el factor de fricción cada tick.
// Si Vx es muy pequeño lo lleva a 0 para evitar micro-oscilaciones.
void GestorFisicas::aplicarFriccion(float &vx, float dt)
{
    // Fricción continua basada en dt para que sea independiente del framerate
    float factor = std::pow(FRICCION_SUELO, dt * 60.f); // normalizado a 60 fps
    vx *= factor;
    if (std::abs(vx) < 0.5f) vx = 0.f;
}

// ── Nivel 2: Inercia (MRUA) ──────────────────────────────────────────────────
// Acelera o frena vx hacia velObjetivo con aceleración diferente según dirección.
// Si velObjetivo == 0 → frena con INERCIA_FRENO
// Si no              → acelera con INERCIA_ACCEL
void GestorFisicas::aplicarInercia(float &vx, float velObjetivo, float dt)
{
    float diferencia = velObjetivo - vx;
    float accel = (std::abs(velObjetivo) < 0.01f) ? INERCIA_FRENO : INERCIA_ACCEL;
    float paso  = accel * dt;

    if (std::abs(diferencia) <= paso)
        vx = velObjetivo;         // Llegó al objetivo exacto
    else
        vx += (diferencia > 0) ? paso : -paso;
}

// ── Colisión AABB con Hitbox (sin Qt) ────────────────────────────────────────
// Comprueba solapamiento entre dos Hitbox usando solo aritmética de floats.
// Sustituye a la antigua colisionAABB(QRectF, QRectF) que dependía de Qt.
bool GestorFisicas::colisionHitbox(const Hitbox &a, const Hitbox &b)
{
    return a.solapa(b);
}

// ── Resolución de colisión AABB (floats sueltos) ─────────────────────────────
// Usa el método del "mínimo eje de penetración" (SAT simplificado para AABB):
// calcula cuánto se superpone en X y en Y, y empuja por el eje menor.
// Retorna true si hubo colisión y la ajustó.
bool GestorFisicas::resolverColision(float &ex, float &ey,
                                     float  ew, float  eh,
                                     float &vx, float &vy,
                                     bool  &enSuelo,
                                     float  ox, float  oy,
                                     float  ow, float  oh)
{
    // Rectángulos
    float eRight  = ex + ew,  eBottom = ey + eh;
    float oRight  = ox + ow,  oBottom = oy + oh;

    // ¿Hay solapamiento?
    if (ex >= oRight || eRight <= ox || ey >= oBottom || eBottom <= oy)
        return false;

    // Penetración en cada eje
    float penX_izq = oRight  - ex;    // entidad viene de la derecha del obstáculo
    float penX_der = eRight  - ox;    // entidad viene de la izquierda del obstáculo
    float penY_arr = oBottom - ey;    // entidad viene de abajo del obstáculo
    float penY_aba = eBottom - oy;    // entidad viene de arriba del obstáculo

    float minPenX = std::min(penX_izq, penX_der);
    float minPenY = std::min(penY_arr, penY_aba);

    if (minPenY < minPenX)
    {
        // Resolver por eje Y (más común: caer sobre plataforma)
        if (penY_aba < penY_arr)
        {
            ey    = oy - eh;   // Posicionar encima
            vy    = 0.f;
            enSuelo = true;
        }
        else
        {
            ey = oBottom;      // Chocó con el techo
            if (vy < 0) vy = 0.f;
        }
    }
    else
    {
        // Resolver por eje X (choque lateral)
        if (penX_der < penX_izq)
            ex = ox - ew;      // Empujar a la izquierda
        else
            ex = oRight;       // Empujar a la derecha
        vx = 0.f;
    }

    return true;
}

// ── Resolución de colisión AABB (sobrecarga con Hitbox) ──────────────────────
// Versión más ergonómica: recibe Hitbox directamente en lugar de floats sueltos.
// Actualiza entidad.x e entidad.y en caso de colisión.
bool GestorFisicas::resolverColision(Hitbox       &entidad,
                                     float        &vx,
                                     float        &vy,
                                     bool         &enSuelo,
                                     const Hitbox &obstaculo)
{
    bool choco = resolverColision(
                     entidad.x, entidad.y,
                     entidad.w, entidad.h,
                     vx, vy, enSuelo,
                     obstaculo.x, obstaculo.y,
                     obstaculo.w, obstaculo.h);
    return choco;
}

// ── Colisión circular (radio de detección del robot) ────────────────────────
bool GestorFisicas::colisionCirculo(float ax, float ay,
                                    float bx, float by,
                                    float radio)
{
    float dx = ax - bx;
    float dy = ay - by;
    return (dx*dx + dy*dy) <= (radio * radio);
}
