#ifndef ROBOTSEGURIDAD_H
#define ROBOTSEGURIDAD_H

#include "enemigo.h"
#include <vector>

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

    // ── Estado del agente ────────────────────────────────────
    EstadoAgente estado;

    // ── Ciclo del agente ─────────────────────────────────────
    void percibir(float jugadorX, float jugadorY) override;
    void razonar()  override;
    void actuar(float dt) override;

    // Método completo que el nivel llama cada tick
    void tick(float jugadorX, float jugadorY, float dt);

    // Actualiza lista de waypoints con la última posición vista
    void actualizarWaypoints();

    // Distancia al jugador (calculada en percibir)
    float calcularDistancia() const;

    // Getters de estado para la UI/debug
    EstadoAgente getEstado()        const { return estado; }
    float        getRadioDeteccion()const { return radioDeteccion; }


    // [NUEVO] — Asigna el ítem gráfico creado por el nivel en la escena
    void setItemGrafico(QGraphicsPixmapItem* item) { itemGrafico = item; }

    ~RobotSeguridad() override = default;

private:
    float radioDeteccion;
    float radioDesenganche;
    float velPatrulla;
    float velPersecucion;

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
