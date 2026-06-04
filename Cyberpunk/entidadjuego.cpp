#include "entidadjuego.h"

EntidadJuego::EntidadJuego() {}

EntidadJuego::EntidadJuego(float X, float Y) {
    x=X;
    y=Y;
    Vx=0.f;
    Vy=0.f;
    itemGrafico=nullptr;
}


//------ Metodos Sobrecargados Obligatorios ----------
//       Constructor de copia
//       Operador de comparacion " == "
EntidadJuego::EntidadJuego(const EntidadJuego& otro)
    : x(otro.x)
    , y(otro.y)
    , Vx(otro.Vx)
    , Vy(otro.Vy)
    , activa(otro.activa)
    , itemGrafico(nullptr)   // no se copia el ítem Qt — pertenece a la escena
{}

bool EntidadJuego::operator==(const EntidadJuego& otro) const
{
    // Dos entidades son iguales si ocupan la misma posición lógica
    return x == otro.x && y == otro.y;
}



// Getters de posición y velocidad
float EntidadJuego::getX()    const { return x; }
float EntidadJuego::getY()    const { return y; }
float EntidadJuego::getVx()   const { return Vx; }
float EntidadJuego::getVy()   const { return Vy; }
QGraphicsPixmapItem* EntidadJuego::getItem() { return itemGrafico; }


// Setters
void EntidadJuego::setPosicion(float nx, float ny) { x = nx; y = ny; }
void EntidadJuego::setVelocidad(float vx, float vy) { Vx = vx; Vy = vy; }

// Indica si la entidad sigue activa en el nivel
bool EntidadJuego::estaActiva() const { return activa; }
void EntidadJuego::desactivar()       { activa = false; }

EntidadJuego::~EntidadJuego(){if (itemGrafico && !itemGrafico->scene())
        delete itemGrafico;}





