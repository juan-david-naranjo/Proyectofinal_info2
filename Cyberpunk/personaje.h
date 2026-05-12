#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "entidadjuego.h"
#include <QKeyEvent>
#include <QPixmap>

// ============================================================
//  Personaje — Kael
//
//  Nivel 1:
//    - Movimiento parabólico (gravedad real en actualizar)
//    - Gran salto + doble salto
//    - El viento (calculado por el nivel) se aplica vía aplicarViento()
//    - Fricción en suelo
//    - Colisiones con plataformas (resueltas por el nivel usando GestorFisicas)
//
//  Nivel 2:
//    - Deslizamiento (slide) con duración limitada
//    - Boost de velocidad temporal
//    - FactorSigilo: reduce radio de detección del robot si va lento
// ============================================================
class Personaje : public EntidadJuego
{
public:
    Personaje();
    explicit Personaje(float x, float y);
    ~Personaje() override;

    // ── Entrada ─────────────────────────────────────────────
    void keyPressed (int key);
    void keyReleased(int key);

    // Métodos de acción directa (llamados por MainWindow)
    void saltar();
    void activarBoost();
    void activarDesliz();

    // ── Actualización ────────────────────────────────────────
    // dt en segundos; tiempoTotal = tiempo acumulado de juego (para el viento)
    // nivel: 1 → aplica gravedad + viento; 2 → aplica inercia
    void actualizar(float dt) override;
    void actualizarNivel1(float dt, float tiempoTotal);
    void actualizarNivel2(float dt);

    // Llamado por el nivel tras resolver colisiones
    void aterrizarEnSuelo(float suloY);   // posiciona encima de la plataforma
    void despegarSuelo();                 // al caer de un borde

    // ── Getters ──────────────────────────────────────────────
    int   getVidas()        const;
    float getEnergia()      const;
    bool  isEnSuelo()       const;
    bool  isBoostActivo()   const;
    bool  isDeslizando()    const;
    float getFactorSigilo() const;
    float getAncho()        const { return ANCHO; }
    float getAlto()         const { return ALTO;  }

    // Hitbox explícita para el sistema de colisiones (sin Qt).
    // Coincide con el rectángulo del sprite: posición (x,y) + tamaño ANCHO×ALTO.
    Hitbox getHitbox() const override { return Hitbox(x, y, ANCHO, ALTO); }

    // ── Daño / reset ─────────────────────────────────────────
    void recibirDanio(int cantidad = 1);
    void resetearPosicion(float rx, float ry);

private:
    // ── Tamaño del sprite en px ──────────────────────────────
    static constexpr float ANCHO = 70.f;
    static constexpr float ALTO  = 124.f;

    // ── Estado general ───────────────────────────────────────
    int   vidas;
    float energia;
    float velMax;           // Velocidad horizontal máxima normal
    bool  enSuelo;
    bool  keys[4];          // 0=Izq, 1=Der, 2=Jump, 3=Down

    QPixmap *Sprite;

    // ── Salto ────────────────────────────────────────────────
    bool  puedeDoubleSalto;
    float fuerzaSalto;
    int   saltosRestantes;

    // ── Deslizamiento (Nivel 2) ──────────────────────────────
    bool  deslizando;
    float tiempoDesliz;
    static constexpr float DURACION_DESLIZ_MAX = 0.8f;

    // ── Boost de velocidad ───────────────────────────────────
    bool  boostActivo;
    float tiempoBoost;
    static constexpr float DURACION_BOOST     = 3.f;
    static constexpr float MULTIPLICADOR_BOOST = 2.0f;

    // ── Sigilo ───────────────────────────────────────────────
    float factorSigilo;
};

#endif // PERSONAJE_H
