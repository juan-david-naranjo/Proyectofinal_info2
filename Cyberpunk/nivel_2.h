#ifndef NIVEL_2_H
#define NIVEL_2_H

#include "nivel.h"
#include "robotseguridad.h"
#include <vector>

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
    Nivel_2();
    ~Nivel_2() override;

    std::vector<RobotSeguridad*> robots;

    void inicializar(Personaje* p) override;
    void actualizar(float dt) override;

    void verificarDeteccion();   // Robot atrapa al jugador → daño
    void verificarVictoria();    // Jugador llega a la computadora

private:
    // Posición del objetivo (computadora a apagar)
    float objetivoX;
    float objetivoY;
    float objetivoRadio;

    float spawnX;
    float spawnY;

    void generarLaberinto();
    void generarRobots();

    void limpiarRobots();
};

#endif // NIVEL_2_H
