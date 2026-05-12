#ifndef ENTIDADJUEGO_H
#define ENTIDADJUEGO_H
#include <QGraphicsPixmapItem>

// ============================================================
//  Punto2D — Par de coordenadas puro (sin Qt)
//
//  Sustituye a QPointF en contextos que no necesitan Qt,
//  como los waypoints del RobotSeguridad.
// ============================================================
struct Punto2D
{
    float x, y;
    Punto2D() : x(0.f), y(0.f) {}
    Punto2D(float px, float py) : x(px), y(py) {}
};

// ============================================================
//  Hitbox — Rectángulo de colisión puro (sin Qt)
//
//  Representa el AABB (Axis-Aligned Bounding Box) de una
//  entidad para el sistema de colisiones. No depende de Qt
//  y puede usarse en cualquier contexto lógico o de pruebas.
//
//  Campos:
//    x, y  — esquina superior izquierda (px)
//    w, h  — ancho y alto (px)
//
//  Uso habitual:
//    Hitbox hb = entidad.getHitbox();
//    if (GestorFisicas::colisionHitbox(hb, otra)) { ... }
// ============================================================
struct Hitbox
{
    float x, y;   // Esquina superior izquierda
    float w, h;   // Dimensiones

    Hitbox() : x(0.f), y(0.f), w(0.f), h(0.f) {}
    Hitbox(float px, float py, float pw, float ph)
        : x(px), y(py), w(pw), h(ph) {}

    // Bordes derivados (evitan recalcular en cada comprobación)
    float derecha()  const { return x + w; }
    float abajo()    const { return y + h; }

    // Comprobación rápida de solapamiento AABB sin depender de Qt
    bool solapa(const Hitbox& otro) const
    {
        return x < otro.derecha()  &&
               derecha() > otro.x  &&
               y < otro.abajo()    &&
               abajo()  > otro.y;
    }
};

class EntidadJuego
{
protected:
    float x;
    float y;
    float Vy;
    float Vx;
    bool activa;
    QGraphicsPixmapItem* itemGrafico;

public:
    EntidadJuego();
    EntidadJuego(float X, float Y);



    //----------Getters-----------
    float getX()     const;
    float getY()     const;
    float getVx()    const;
    float getVy()    const;
    QGraphicsPixmapItem* getItem();

    // Devuelve el rectángulo de colisión lógico de la entidad.
    // Las subclases deben sobreescribirlo para reflejar su tamaño real.
    // Por defecto retorna un Hitbox de tamaño 0 en la posición de la entidad.
    virtual Hitbox getHitbox() const { return Hitbox(x, y, 0.f, 0.f); }

    //---------Setters------------
    void setPosicion(float nx, float ny);
    void setVelocidad(float vx, float vy);


    bool estaActiva() const ;
    void desactivar()       ;

    virtual void actualizar(float dt) = 0;
    virtual ~EntidadJuego();           //Para la herencia
};

#endif // ENTIDADJUEGO_H
