#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "entidadjuego.h"
#include <vector>
#include <QKeyEvent>
#include <QPixmap>
#include <QDebug>
#include <QString>


#include <QPainter>

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
    //----------- Sobrecargas Obligatorias
    Personaje(const Personaje& otro);
    bool operator==(const Personaje& otro) const;

    // ── Entrada ──────────────────────────────────────────────
    void keyPressed (int key);
    void keyReleased(int key);

    void saltar();
    void activarBoost();
    void activarDesliz();
    void activarDeslizNivel2();

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

    // Hitbox getHitbox() const override { return Hitbox(x, y, ANCHO, ALTO); }
    Hitbox getHitbox() const override {
        return Hitbox(x + hitboxOffsetX,
                      y + hitboxOffsetY,
                      ANCHO, ALTO);
    }

    void setHitboxOffset(float offsetX,float offsetY,float anchoEfectivo, float altoEfectivo);
    void setVidas(int cantidad);//setter para nivel_2
    // ── Daño / reset ──────────────────────────────────────────
    void recibirDanio(int cantidad = 1);
    void resetearPosicion(float rx, float ry);

private:
    // ── Tamaño del sprite ─────────────────────────────────────
    // static constexpr float ANCHO = 70.f;
    // static constexpr float ALTO  = 100.f;

    float ANCHO = 70.f;
    float ALTO  = 100.f;

    float hitboxOffsetY  = 0.f;
    float hitboxOffsetX = 0.f;
    float hitboxAnchoReal  = ANCHO;
    float hitboxAltoReal = ALTO;

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

    // Nivel 2
    std::vector<QPixmap> framesIdle;
    std::vector<QPixmap> framesCorriendo;
    std::vector<QPixmap> framesDeslizando;
    std::vector<QPixmap> framesUprun;
    std::vector<QPixmap> framesDownrun;


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
        CORRIENDO_ARRIBA,
        CORRIENDO_ABAJO,
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
    // void tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop = true);

    // personaje.h — actualizar la firma:
    // void tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop,
    //                    float multVelocidad = 1.0f);  // 1.0 = normal, >1 = más lento
    void tickAnimacion(float dt, std::vector<QPixmap>& frames, bool loop,
                       float multVelocidad = 1.0f);

};

#endif // PERSONAJE_H
