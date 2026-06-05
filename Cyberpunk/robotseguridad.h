#ifndef ROBOTSEGURIDAD_H
#define ROBOTSEGURIDAD_H

#include "enemigo.h"
#include <vector>
#include <QDebug>

// ============================================================
//  RobotSeguridad  —  Agente inteligente del Nivel 2
//
//  Percepción:   Radio de detección circular.
//                Si el jugador entra → estado PERSECUCION.
//
//  Razonamiento: Máquina de estados (EstadoAgente):
//                  PATRULLAJE → sigue waypoints en orden.
//                  PERSECUCION → persigue al jugador (mínimo 3 s).
//
//  Aprendizaje:  Guarda la última posición vista del jugador y la
//                añade al vector de waypoints para que la próxima
//                patrulla pase por ahí.
//
//  Movimiento:   Inercia (MRUA) usando GestorFisicas::aplicarInercia.
//                Boost de velocidad al entrar en PERSECUCION.
//
//  NOTA: usa Punto2D (definido en entidadjuego.h) en lugar de QPointF
//        para que los waypoints no dependan de Qt.
// ============================================================

enum class EstadoAgente { PATRULLAJE, PERSECUCION };


class RobotSeguridad : public Enemigo
{
public:
    // ── Constructor ─────────────────────────────────────────
    /**
     * @param x,y              Posición inicial
     * @param radioDeteccion   Px del radio de visión
     * @param radioDesenganche Px a los que pierde de vista al jugador
     * @param velPatrulla      Velocidad normal de patrulla (px/s)
     * @param valPersecucion   Velocidad de persecución (px/s)
     * @param waypoints        Lista de puntos de patrulla iniciales
     */
    RobotSeguridad(float x, float y,
                   float radioDeteccion,
                   float radioDesenganche,
                   float velPatrulla,
                   float valPersecucion,
                   const std::vector<Punto2D> &waypoints);

    RobotSeguridad(const RobotSeguridad& otro);
    bool operator==(const RobotSeguridad& otro) const;

    // ── Estado del agente ────────────────────────────────────
    EstadoAgente estado;

    // ── Ciclo del agente ─────────────────────────────────────
    void percibir(float jugadorX, float jugadorY) override;
    void razonar()  override{ razonar(false);}
    void razonar(bool jugadorOculto = false) ;
    void actuar(float dt) override;

    bool atrapoJugador() const { return capturado; }
    void resetCaptura()        { capturado = false; }

    //void tick(float jx, float jy, float dt, bool jugadorOculto = false);
    void tick(float jx, float jy, float dt, bool jugadorOculto,
              const std::vector<Hitbox>& paredes = {});

    // Actualiza lista de waypoints con la última posición vista
    void actualizarWaypoints();

    // Distancia al jugador (calculada en percibir)
    float calcularDistancia() const;

    // Getters de estado para la UI/debug
    EstadoAgente getEstado()        const { return estado; }
    float        getRadioDeteccion()const { return radioDeteccion; }



    void setItemGrafico(QGraphicsPixmapItem* item) { itemGrafico = item; }

    QPixmap getPrimerFrame() const
    {
        if (!framesPatrullaje.empty()) return framesPatrullaje.at(0);
        return QPixmap();
    }

    void cargarSprites(const QPixmap& sheet);
    ~RobotSeguridad() override = default;

private:
    std::vector<Hitbox> paredesCache;
    float radioDeteccion;
    float radioDesenganche;
    float velPatrulla;
    float velPersecucion;
    bool capturado = false;
    static constexpr float RADIO_CAPTURA = 60.f;
    // Miembros privados nuevos (sección private)
    std::vector<QPixmap> framesPatrullaje;
    std::vector<QPixmap> framesAlert;
    int   frameActual            = 0;
    float tiempoFrame            = 0.f;
    float duracionFramePatrullaje;
    float duracionFrameAlert;

    // ── Detección de atasco ───────────────────────────────────
    float   posXAnterior  = 0.f;
    float   posYAnterior  = 0.f;
    float   tiempoStuck   = 0.f;
    bool    tieneDesvio   = false;
    Punto2D puntoDesvio   = {0.f, 0.f};
    int     ladoDesvio    = 1;          // alterna +1/-1 para no girar siempre igual

    static constexpr float UMBRAL_STUCK = 0.35f;  // segundos sin moverse → atascado
    static constexpr float DIST_DESVIO  = 90.f;   // distancia del punto de rodeo
    static constexpr float RADIO_LLEGADA_DESVIO = 25.f;


    // Los métodos nuevos ya no necesitan recibir paredes como parámetro:
    bool posicionLibre(float px, float py, float tam) const;
    void moverHaciaConEvacion(float tx, float ty, float dt);



    // Waypoints de patrulla (aprendizaje: se añaden posiciones del jugador)
    std::vector<Punto2D> waypoints;
    std::vector<Punto2D> historial;   // Posiciones vistas del jugador
    int   indiceWaypoint = 0;

    // Temporización de la persecución (mínimo 3 s)
    float tiempoPersecucion  = 0.f;
    static constexpr float DURACION_MIN_PERSECUCION = 3.f;

    // Distancia calculada en percibir()
    float distanciaJugador = 9999.f;

    // Velocidad objetivo actual (para inercia)
    float velObjetivo = 0.f;

    // Mover hacia un punto con inercia
    void moverHacia(float tx, float ty, float dt);
};

#endif // ROBOTSEGURIDAD_H
