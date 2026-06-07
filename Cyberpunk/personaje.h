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
//  Nivel 1 (spritesheet 669x373, fondo negro):
//    Fila 1 (y=44-109):  IDLE (1f) + CORRER (7f)
//    Fila 2 (y=110-208): SALTO completo (12f) + DOBLE SALTO
//    Fila 3 (y=214-280): VIENTO CAÍDA + COLISIÓN
//    Fila 4 (y=294-353): CAÍDA FINAL (3f) — ≥4 plataformas de caída
//
//  Nivel 2:
//    IDLE, CORRIENDO, CORRIENDO_ARRIBA, CORRIENDO_ABAJO,
//    DESLIZANDO (con activarDeslizNivel2), BOOST con tinte cian
//    Hitbox ajustable en tiempo de ejecución (setHitboxOffset)
// ============================================================
class Personaje : public EntidadJuego
{
public:

    enum class ClaseActiva {
        VELOCISTA,
        ESPECTRO
    };


    void setClase(ClaseActiva nuevaClase) { clase = nuevaClase; }

    // Cambiamos activarBoost por una función general
    void usarHabilidadEspecial();

    // Para el HUD (le cambiamos el nombre para que sirva para ambos)
    float getProgresoCooldownHabilidad() const;

    // Para que Nivel_2 sepa si Kael es invisible
    bool isSigiloActivo() const { return tiempoSigiloActivo > 0.f; }

    Personaje();
    explicit Personaje(float x, float y);
    ~Personaje() override;
    //----------- Sobrecargas Obligatorias
    Personaje(const Personaje& otro);
    Personaje& operator=(const Personaje& otro);  // Regla de los Tres
    bool operator==(const Personaje& otro) const;

    // Entrada
    void keyPressed (int key);
    void keyReleased(int key);
    void saltar();
    void activarBoost();
    void activarDesliz();           // nivel 1 (solo en suelo)
    void activarDeslizNivel2();     // nivel 2 (en movimiento, con impulso)

    //  Actualización
    void actualizar(float dt) override;
    void actualizarNivel1(float dt, float tiempoTotal);
    void actualizarNivel2(float dt);

    void aterrizarEnSuelo(float suloY);
    void despegarSuelo();

    void cargarSpritesNivel1();
    void cargarSpritesNivel2();
    void invalidarItem();
    void recrearItem();

    // Caída final (nivel 1)
    void activarCaidaFinal();
    bool caidaFinalTerminada() const;

    //  Hitbox ajustable (nivel 2)
    void setHitboxOffset(float offsetX, float offsetY,
                         float anchoEfectivo, float altoEfectivo);

    // Tamaño visual del sprite (solo nivel 1)
    // No afecta hitbox ni física. Llamar ANTES de cargarSpritesNivel1().
    void setSpriteSize(float ancho, float alto);

    //Getters
    int   getVidas()        const;
    float getEnergia()      const;
    bool  isEnSuelo()       const;
    bool  isBoostActivo()   const;
    bool  isDeslizando()    const;
    float getYMasAlta()     const;
    float getAncho()        const { return ANCHO; }
    float getAlto()         const { return ALTO;  }

    Hitbox getHitbox() const override {
        return Hitbox(x + hitboxOffsetX, y + hitboxOffsetY,
                      hitboxAnchoReal,   hitboxAltoReal);
    }
    void setVidas(int cantidad);//setter para nivel_2
    float getHitboxOffsetX() const { return hitboxOffsetX; }
    float getHitboxOffsetY() const { return hitboxOffsetY; }

    float getProgresoCooldownBoost();       //helper para el HUD



    // Daño / reset
    void recibirDanio(int cantidad = 1);
    void resetearPosicion(float rx, float ry);

    void resetearEfectos();

private:
    // Tamaño lógico de la hitbox (modificable via setHitboxOffset)
    float ANCHO = 70.f;
    float ALTO  = 70.f;

    // Tamaño visual del sprite nivel 1 (independiente de la hitbox)
    // Estos valores NO se tocan al llamar setHitboxOffset.
    float spriteAncho = 70.f;
    float spriteAlto  = 70.f;

    // Hitbox interna (puede diferir del sprite para mayor precisión)
    float hitboxOffsetX   = 15.f;
    float hitboxOffsetY   =  8.f;
    float hitboxAnchoReal = 40.f;
    float hitboxAltoReal  = 62.f;

    //Helpers
    QPixmap eliminarFondo(const QPixmap& src, QColor cf, int tol);

    //Animaciones Nivel 1
    std::vector<QPixmap> n1_framesIdle;
    std::vector<QPixmap> n1_framesCorriendo;
    std::vector<QPixmap> n1_framesSaltando;
    std::vector<QPixmap> n1_framesDobleSalto;
    std::vector<QPixmap> n1_framesVientoCalda;
    std::vector<QPixmap> n1_framesColision;
    std::vector<QPixmap> n1_framesCaidaFinal;

    // Animaciones Nivel 2
    std::vector<QPixmap> framesIdle;
    std::vector<QPixmap> framesCorriendo;
    std::vector<QPixmap> framesDeslizando;
    std::vector<QPixmap> framesUprun;
    std::vector<QPixmap> framesDownrun;

    // Estado de animación
    enum class EstadoAnim {
        // Comunes
        IDLE,
        CORRIENDO,
        // Nivel 1
        SALTANDO,
        DOBLE_SALTO,
        VIENTO_CAIDA,
        COLISION,
        CAIDA_FINAL,
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
    bool       enCaidaFinal;       // bloquea física durante animación caída

    //  Estado general
    int   vidas;
    float velMax;
    float energia;          //para el boost
    bool  enSuelo;
    bool  keys[4];       // 0=Izq 1=Der 2=Up 3=Down
    QPixmap* Sprite;

    // Salto
    bool  puedeDoubleSalto;
    float fuerzaSalto;
    int   saltosRestantes;
    float tiempoViento;
    static constexpr float UMBRAL_VIENTO = 0.6f;

    // Caída profunda (nivel 1)
    float yMasAlta;
    int   plataformasCalda;

    // Deslizamiento
    bool  deslizando;
    float tiempoDesliz;
    static constexpr float DURACION_DESLIZ_MAX = 0.8f;

    //Boost
    bool  boostActivo;          //bandera
    float tiempoBoost;          //temporizador
    float cooldownBoost=0.f;
    static constexpr float MULTIPLICADOR_BOOST = 2.0f;

    ClaseActiva clase = ClaseActiva::VELOCISTA; // Por defecto

    // Habilidad Especial (Reemplaza las variables del boost)
    float cooldownHabilidad = 0.f;
    static constexpr float COOLDOWN_MAX = 5.f;

    static constexpr float DURACION_BOOST = 3.f;
    float tiempoSigiloActivo = 0.f;
    static constexpr float DURACION_SIGILO = 2.f;

    //tickAnimacion
    void tickAnimacion(float dt, std::vector<QPixmap>& frames,
                       bool loop, float multVelocidad = 1.0f);
};

#endif // PERSONAJE_H
