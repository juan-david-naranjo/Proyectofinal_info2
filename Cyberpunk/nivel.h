#ifndef NIVEL_H
#define NIVEL_H

#include <vector>
#include "entidadjuego.h"
#include "personaje.h"
#include "plataforma.h"

// ============================================================
//  Nivel  (clase base abstracta)
//  Gestiona la colección de entidades y el loop de físicas.
//  El nivel resuelve colisiones entre el personaje y las
//  plataformas/obstáculos cada tick, usando GestorFisicas.
// ============================================================
class Nivel
{
public:
    int   tiempoRestante;
    bool  completado;
    float tiempoAcumulado;     // Para el viento y otras físicas time-based

    std::vector<Plataforma*>   plataformas;
    Personaje*                 jugador;

    Nivel();
    virtual ~Nivel();

    // Inicializar entidades del nivel
    virtual void inicializar(Personaje* p) = 0;

    // Tick principal: actualiza físicas, resuelve colisiones
    virtual void actualizar(float dt) = 0;

    // Resuelve colisiones personaje ↔ todas las plataformas
    void resolverColisiones();

protected:
    void limpiarPlataformas();
};

#endif // NIVEL_H
