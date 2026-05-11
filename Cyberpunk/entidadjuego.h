#ifndef ENTIDADJUEGO_H
#define ENTIDADJUEGO_H
#include <QGraphicsPixmapItem>

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

    //---------Setters------------
    void setPosicion(float nx, float ny);
    void setVelocidad(float vx, float vy);


    bool estaActiva() const ;
    void desactivar()       ;

    virtual void actualizar(float dt) = 0;
    virtual ~EntidadJuego();           //Para la herencia
};

#endif // ENTIDADJUEGO_H
