#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "entidadjuego.h"
#include <QKeyEvent>
#include <QPixmap>
#include <QDebug>
#include <QString>

// ============================================================
//  Personaje — Kael
//
//  Nivel 1:
//    - Movimiento parabólico (gravedad real en actualizarNivel1)
//    - Gran salto + doble salto
//    - Viento oscilatorio (GestorFisicas::aplicarViento)
//    - Fricción en suelo
//    - Animaciones: Idle, Corriendo, Saltando, DobleSalto,
//                   VientoCalda, Colision
//
//  Nivel 2:
//    - Deslizamiento (slide) con duración limitada
//    - Boost de velocidad temporal
//    - FactorSigilo: reduce radio de detección del robot
// ============================================================
class Personaje : public EntidadJuego
{
public:
    Personaje();
    explicit Personaje(float x, float y);
    ~Personaje() override;

    // ── Entrada ──────────────────────────────────────────────
    void keyPressed (int key);
    void keyReleased(int key);

    void saltar();
    void activarBoost();
    void activarDesliz();

    // ── Actualización ─────────────────────────────────────────
    void actualizar(float dt) override;
    void actualizarNivel1(float dt, float tiempoTotal);
    void actualizarNivel2(float dt);

    // Llamado por el nivel tras resolver colisiones
    void aterrizarEnSuelo(float suloY);
    void despegarSuelo();

    // Carga de sprites
    void cargarSpritesNivel1();
    void cargarSpritesNivel2();

    // ── Getters ───────────────────────────────────────────────
    int   getVidas()        const;
    float getEnergia()      const;
    bool  isEnSuelo()       const;
    bool  isBoostActivo()   const;
    bool  isDeslizando()    const;
    float getFactorSigilo() const;
    float getAncho()        const { return ANCHO; }
    float getAlto()         const { return ALTO;  }

    Hitbox getHitbox() const override { return Hitbox(x, y, ANCHO, ALTO); }

    // ── Daño / reset ──────────────────────────────────────────
    void recibirDanio(int cantidad = 1);
    void resetearPosicion(float rx, float ry);

private:
    // ── Tamaño del sprite ─────────────────────────────────────
    static constexpr float ANCHO = 70.f;
    static constexpr float ALTO  = 124.f;

    // ── Utilidades de sprites ─────────────────────────────────
    QPixmap eliminarFondo(const QPixmap& source, QColor colorFondo, int tolerancia);

    // ── Vectores de frames por animación — Nivel 1 ────────────
    QVector<QPixmap> n1_framesIdle;
    QVector<QPixmap> n1_framesCorriendo;
    QVector<QPixmap> n1_framesSaltando;       // SALTAR (JUMP)
    QVector<QPixmap> n1_framesDobleSalto;     // DOBLE SALTO
    QVector<QPixmap> n1_framesVientoCalda;    // CAER POR VIENTO
    QVector<QPixmap> n1_framesColision;       // COLISIÓN/CAÍDA

    // ── Vectores de frames por animación — Nivel 2 ────────────
    QVector<QPixmap> framesIdle;
    QVector<QPixmap> framesCorriendo;
    QVector<QPixmap> framesDeslizando;
    QVector<QPixmap> framesBoost;

    // ── Estado de animación ───────────────────────────────────
    // Estados compartidos; el nivel activo decide cuáles usar
    enum class EstadoAnim {
        // Comunes
        IDLE,
        CORRIENDO,
        // Nivel 1
        SALTANDO,
        DOBLE_SALTO,
        VIENTO_CAIDA,
        COLISION,
        // Nivel 2
        DESLIZANDO,
        BOOST
    };

    EstadoAnim estadoAnim;
    int        frameActual;
    float      tiempoFrame;
    float      duracionFrame;
    bool       miraDerecha;

    // ── Estado general ────────────────────────────────────────
    int   vidas;
    float energia;
    float velMax;
    bool  enSuelo;
    bool  keys[4];          // 0=Izq, 1=Der, 2=Jump/Up, 3=Down

    QPixmap* Sprite;        // Hoja de sprites (legacy, se puede quitar)

    // ── Salto ─────────────────────────────────────────────────
    bool  puedeDoubleSalto;
    float fuerzaSalto;
    int   saltosRestantes;

    // Detectar empuje fuerte del viento (para cambiar anim)
    float tiempoViento;     // segundos en el aire bajo viento fuerte
    static constexpr float UMBRAL_VIENTO = 0.6f;   // s antes de "viento caida"

    // ── Deslizamiento (Nivel 2) ───────────────────────────────
    bool  deslizando;
    float tiempoDesliz;
    static constexpr float DURACION_DESLIZ_MAX = 0.8f;

    // ── Boost (Nivel 2) ───────────────────────────────────────
    bool  boostActivo;
    float tiempoBoost;
    static constexpr float DURACION_BOOST      = 3.f;
    static constexpr float MULTIPLICADOR_BOOST = 2.0f;

    // ── Sigilo (Nivel 2) ──────────────────────────────────────
    float factorSigilo;

    // ── Helpers internos ─────────────────────────────────────
    // Avanza la animación y aplica el frame al itemGrafico
    void tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop = true);
};

#endif // PERSONAJE_H
