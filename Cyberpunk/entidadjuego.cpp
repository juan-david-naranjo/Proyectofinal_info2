#include "entidadjuego.h"

EntidadJuego::EntidadJuego() {}

EntidadJuego::EntidadJuego(float X, float Y) {
    x=X;
    y=Y;
    itemGrafico=nullptr;
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

EntidadJuego::~EntidadJuego(){}





