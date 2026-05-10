#ifndef NIVEL_H
#define NIVEL_H
#include <vector>
#include "entidadjuego.h"


using namespace std;

class Nivel
{
    int tiempoRestante;
    bool completado;
    vector<EntidadJuego> entidades;

public:
    Nivel();
    // virtual Inicializar();
};

#endif // NIVEL_H
