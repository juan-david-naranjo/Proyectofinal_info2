#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "entidadjuego.h"
#include <QKeyEvent>
#include <QPixmap>
#include <QDebug>

// ============================================================
//  Personaje — Kael
//
//  Spritesheet nivel 1 (669x373, fondo negro):
//    Fila 1 (y=44-109):  IDLE (1f) + CORRER (7f)
//    Fila 2 (y=110-208): SALTO completo (12f)
//    Fila 3 (y=214-280): VIENTO — OMITIDA (pendiente ventilador)
//    Fila 4 (y=294-353): CAÍDA FINAL (3f, grupos 1-3 del personaje)
//                        Se activa al caer ≥4 plataformas (~480px)
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

    void aterrizarEnSuelo(float suloY);
    void despegarSuelo();

    void cargarSpritesNivel1();
    void cargarSpritesNivel2();

    // ── Caída final ────────────────────────────────────────────
    void activarCaidaFinal();
    bool caidaFinalTerminada() const;

    // ── Getters ───────────────────────────────────────────────
    int   getVidas()        const;
    float getEnergia()      const;
    bool  isEnSuelo()       const;
    bool  isBoostActivo()   const;
    bool  isDeslizando()    const;
    float getFactorSigilo() const;
    float getYMasAlta()     const;
    float getAncho()        const { return ANCHO; }
    float getAlto()         const { return ALTO;  }
    Hitbox getHitbox() const override { return Hitbox(x, y, ANCHO, ALTO); }

    // ── Daño / reset ──────────────────────────────────────────
    void recibirDanio(int cantidad = 1);
    void resetearPosicion(float rx, float ry);

private:
    // ── Tamaño lógico ─────────────────────────────────────────
    static constexpr float ANCHO = 70.f;
    static constexpr float ALTO  = 70.f;

    QPixmap eliminarFondo(const QPixmap& src, QColor cf, int tol);

    // ── Animaciones Nivel 1 ───────────────────────────────────
    QVector<QPixmap> n1_framesIdle;         // Fila 1, grupo 1  (1f)
    QVector<QPixmap> n1_framesCorriendo;    // Fila 1, grupos 2-8 (7f)
    QVector<QPixmap> n1_framesSaltando;     // Fila 2, todos (12f)
    QVector<QPixmap> n1_framesVientoCalda;  // Fila 3 — OMITIDA, siempre vacío
    QVector<QPixmap> n1_framesCaidaFinal;   // Fila 4, grupos 1-3 (3f)

    // ── Animaciones Nivel 2 ───────────────────────────────────
    QVector<QPixmap> framesIdle;
    QVector<QPixmap> framesCorriendo;
    QVector<QPixmap> framesDeslizando;
    QVector<QPixmap> framesBoost;

    // ── Estado de animación ───────────────────────────────────
    enum class EstadoAnim {
        IDLE,
        CORRIENDO,
        SALTANDO,
        CAIDA_FINAL,    // animación antes del respawn
        DESLIZANDO,     // nivel 2
        BOOST           // nivel 2
    };

    EstadoAnim estadoAnim;
    int        frameActual;
    float      tiempoFrame;
    float      duracionFrame;
    bool       miraDerecha;
    bool       enCaidaFinal;

    // ── Estado general ────────────────────────────────────────
    int   vidas;
    float energia;
    float velMax;
    bool  enSuelo;
    bool  keys[4];      // 0=Izq 1=Der 2=Up 3=Down
    QPixmap* Sprite;

    // ── Salto ─────────────────────────────────────────────────
    bool  puedeDoubleSalto;
    float fuerzaSalto;
    int   saltosRestantes;
    float tiempoViento;
    static constexpr float UMBRAL_VIENTO = 0.6f;

    // ── Detección caída profunda ──────────────────────────────
    float yMasAlta;     // Y mínima alcanzada (valor menor = más arriba)
    int   plataformasCalda;

    // ── Deslizamiento (N2) ────────────────────────────────────
    bool  deslizando;
    float tiempoDesliz;
    static constexpr float DURACION_DESLIZ_MAX = 0.8f;

    // ── Boost (N2) ────────────────────────────────────────────
    bool  boostActivo;
    float tiempoBoost;
    static constexpr float DURACION_BOOST      = 3.f;
    static constexpr float MULTIPLICADOR_BOOST = 2.0f;

    // ── Sigilo (N2) ───────────────────────────────────────────
    float factorSigilo;

    void tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop);
};

#endif // PERSONAJE_H
