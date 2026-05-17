#ifndef NIVEL_2_H
#define NIVEL_2_H

#include "nivel.h"
#include "robotseguridad.h"
#include <vector>
#include <QPixmap>
#include <QGraphicsScene>

// ============================================================
//  Nivel_2 — Vista cenital, laberinto + sigilo
//
//  Físicas activas:
//    • Inercia (MRUA) en el movimiento del personaje
//    • Colisiones AABB con obstáculos/paredes del laberinto
//    • Movimiento parabólico conservado (física base)
//
//  IA activa:
//    • RobotSeguridad: percibir → razonar → actuar (cada tick)
//    • Estado PATRULLAJE / PERSECUCION (mín. 3 s)
//    • Aprendizaje: incorpora última posición vista a waypoints
//
//  Dinámica:
//    • El jugador debe llegar a la computadora (objetivo) sin ser atrapado
//    • Si el robot toca al jugador → pierde una vida y respawnea
// ============================================================
class Nivel_2 : public Nivel
{
public:
    QPixmap *Escenario;
    Nivel_2();
    ~Nivel_2() override;

    std::vector<RobotSeguridad*> robots;

    void inicializar(Personaje* p) override;
    void actualizar(float dt) override;

    // void verificarDeteccion();   // Robot atrapa al jugador → daño
    // void verificarVictoria();    // Jugador llega a la computadora



    // Getters de estado (para HUD externo)
    int   getTiempoRestante() const;
    bool  isCompletado()      const;



    //para el escenario
    void setScene(QGraphicsScene* scene);

private:
    // Posición del objetivo (computadora a apagar)
    float objetivoX;
    float objetivoY;
    float objetivoRadio;

    float spawnX;
    float spawnY;

    float tiempoContador;     /// Acumulador para descontar tiempoRestante cada segundo



    // ── Escena Qt ─────────────────────────────────────────────
    QGraphicsScene*              escena;          ///< Referencia a la escena actual





    // Ítems visuales (propiedad de la escena, no los borramos)
    QList<QGraphicsRectItem*>    itemsParedes;    ///< Rectángulos de las paredes
    QGraphicsEllipseItem*        itemObjetivo;    ///< Círculo del objetivo
    QList<QGraphicsEllipseItem*> itemsDeteccion;  ///< Círculos de detección por robot


    void generarLaberinto();
    void generarRobots();
    void limpiarRobots();



    void resolverColisiones();    ///< Jugador ↔ paredes
    void verificarDeteccion();    ///< Robot alcanza al jugador → daño + respawn
    void verificarVictoria();     ///< Jugador llega a la computadora

    void agregarItemsEscena();    ///< Crea y añade todos los QGraphicsItem a la escena
    void actualizarCirculosDeteccion(); ///< Mueve y recolorea los círculos cada tick

    static QPixmap crearSpriteRobot(int w, int h); ///< Dibuja el robot con QPainter


};

#endif // NIVEL_2_H
