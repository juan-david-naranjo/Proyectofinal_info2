#include "personaje.h"

Personaje::Personaje() {}
Personaje::Personaje(float X,float Y):EntidadJuego(X,Y) {



    itemGrafico = new QGraphicsPixmapItem();


    vidas=10;              // Vidas restantes
    energia=100;            // Barra de energía (0-100)
    velMax=10;             // Velocidad horizontal máxima


    Sprite = new QPixmap(":/Kael_nivel1/Sprites/Nivel1/sprites nivel 1 kael.png");
    itemGrafico->setPixmap(*Sprite);

    itemGrafico->setOffset(43,92);
    itemGrafico->setPixmap(Sprite->copy(43, 92, 70, 124));
}

void Personaje::mover(int key, bool is_valid)
{

    if (key >= 0 && key < 4)
        keys[key] = is_valid ? 1 : 0;

    actualizar(2);
    keys[key]=false;
}





void Personaje::actualizar(float dt){
    // Movimiento horizontal
    if (keys[0]) Vx = -velMax;       // izquierda
    if (keys[1]) Vx =  velMax;       // derecha
    if (keys[2]){ saltar();return;}
    if (keys[3]){ bajar();return;}
    if (!keys[0] && !keys[1]) Vx = 0;


    // Salto (solo al presionar, no al mantener)
    // El salto se maneja por evento puntual, no por keys[]

    // Actualizar posición
    x += Vx * dt;
    // y += Vy * dt;

    // Sincronizar sprite con posición lógica
    if (itemGrafico!=nullptr) itemGrafico->setPos(x, y);
}

void Personaje::saltar() {
    if (saltosRestantes > 0) {
        Vy = -fuerzaSalto;   // negativo porque Y crece hacia abajo en Qt
        y += Vy;
        if (itemGrafico!=nullptr) itemGrafico->setPos(x, y);
        // enSuelo = false;

    }
}

void Personaje::bajar() {
    if (saltosRestantes > 0) {
        Vy = fuerzaSalto;   // negativo porque Y crece hacia abajo en Qt
        y += Vy;
        if (itemGrafico!=nullptr) itemGrafico->setPos(x, y);
        // enSuelo = false;

    }
}








// ── Getters de estado ────────────────────────────────────
int   Personaje::getVidas()          const { return vidas; }
float Personaje::getEnergia()        const { return energia; }
bool  Personaje::isEnSuelo()         const { return enSuelo; }
bool  Personaje::isBoostActivo()     const { return boostActivo; }
bool  Personaje::isDeslizando()      const { return deslizando; }
float Personaje::getFactorSigilo()   const { return factorSigilo; }


Personaje::~Personaje(){}