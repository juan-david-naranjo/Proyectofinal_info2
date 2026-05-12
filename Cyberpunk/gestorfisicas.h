#ifndef GESTORFISICAS_H
#define GESTORFISICAS_H

#include <cmath>
#include "entidadjuego.h"   // Hitbox (puro, sin Qt)

// ============================================================
//  GestorFisicas
//  Motor de físicas estático para el juego Cyberpunk.
//
//  Nivel 1:
//    - Gravedad constante (caída parabólica)
//    - Fricción horizontal con el suelo
//    - Viento oscilatorio (movimiento sinusoidal) que empuja al personaje
//    - Colisiones AABB personaje ↔ plataforma
//
//  Nivel 2:
//    - Inercia (MRUA: aceleración y frenado suave)
//    - Colisiones AABB personaje ↔ obstáculos
//    - Movimiento parabólico conservado
//
//  Todos los métodos son estáticos: GestorFisicas::aplicarGravedad(...)
//
//  COLISIONES:
//    Las colisiones usan Hitbox (definida en entidadjuego.h) en lugar de
//    QRectF, por lo que el sistema de físicas NO depende de Qt.
//    Esto facilita pruebas unitarias y portabilidad.
// ============================================================

class GestorFisicas
{
public:
    GestorFisicas() = default;

    // ── Constantes físicas ──────────────────────────────────
    static constexpr float GRAVEDAD          = 800.f;   // px/s²  (eje Y crece ↓)
    static constexpr float FRICCION_SUELO    = 0.85f;   // factor multiplicativo por tick
    static constexpr float VIENTO_AMPLITUD   = 120.f;   // px/s  (fuerza máx. del viento)
    static constexpr float VIENTO_FRECUENCIA = 0.4f;    // Hz
    static constexpr float VEL_MAX_CAIDA     = 900.f;   // px/s  (terminal velocity)
    static constexpr float INERCIA_ACCEL     = 600.f;   // px/s²
    static constexpr float INERCIA_FRENO     = 400.f;   // px/s²

    // ── Nivel 1: Física vertical ────────────────────────────
    static void aplicarGravedad(float &vy, float &y, float dt);

    // ── Nivel 1: Viento oscilatorio ─────────────────────────
    static float calcularFuerzaViento(float tiempoTotal);
    static void  aplicarViento(float &vx, float tiempoTotal, float dt);

    // ── Nivel 1: Fricción horizontal ────────────────────────
    static void aplicarFriccion(float &vx, float dt);

    // ── Nivel 2: Inercia (MRUA) ─────────────────────────────
    static void aplicarInercia(float &vx, float velObjetivo, float dt);

    // ── Colisiones AABB con Hitbox (sin Qt) ─────────────────
    // Comprueba si dos hitboxes se solapan.
    static bool colisionHitbox(const Hitbox &a, const Hitbox &b);

    // Resuelve la penetración entre una entidad (e) y un obstáculo (o).
    // Recibe y modifica posición (ex,ey), velocidad (vx,vy) y flag enSuelo.
    // Acepta tanto floats sueltos como Hitbox directamente (sobrecarga).
    static bool resolverColision(float &ex, float &ey,
                                 float  ew, float  eh,
                                 float &vx, float &vy,
                                 bool  &enSuelo,
                                 float  ox, float  oy,
                                 float  ow, float  oh);

    // Sobrecarga cómoda: recibe las hitboxes directamente.
    // La hitbox del obstáculo 'o' es inmutable (es estática en el nivel).
    static bool resolverColision(Hitbox       &entidad,
                                 float        &vx,
                                 float        &vy,
                                 bool         &enSuelo,
                                 const Hitbox &obstaculo);

    // ── Colisión circular ────────────────────────────────────
    static bool colisionCirculo(float ax, float ay,
                                float bx, float by,
                                float radio);
};

#endif // GESTORFISICAS_H
