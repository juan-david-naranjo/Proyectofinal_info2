#ifndef PERSONAJE_H
#define PERSONAJE_H
#include "EntidadJuego.h"
#include <QKeyEvent>
#include <QPixmap>


class Personaje:public EntidadJuego
{
private:


    // ── Estado general ───────────────────────────────────────
    int   vidas;              // Vidas restantes
    float energia;            // Barra de energía (0-100)
    float velMax;             // Velocidad horizontal máxima
    bool  enSuelo;            // true si está sobre una plataforma
    bool keys[4]={false};     //Movimientos 0 (Izq), 1(Der), 2(Up),3(Down)


    QPixmap *Sprite;


    // ── Características: Gran salto / doble salto ────────────
    bool  puedeDoubleSalto;   // true si aún no ha usado el 2do salto
    float fuerzaSalto=20;        // Fuerza vertical al saltar
    int   saltosRestantes=1;    // 0 = no puede saltar, 1 = puede, 2 = doble salto

    // ── Característica: Deslizamiento (Nivel 2) ──────────────
    bool  deslizando;         // true mientras el jugador se desliza
    float tiempoDesliz;       // Duración del deslizamiento actual
    float duracionDeslizMax;  // Duración máxima permitida (ej. 0.8 s)

    // ── Característica: Boost de velocidad ───────────────────
    bool  boostActivo;        // true mientras el boost está encendido
    float tiempoBoost;        // Tiempo restante del boost actual
    float duracionBoost;      // Duración total de cada boost (ej. 3 s)
    float multiplicadorBoost; // Factor de velocidad extra (ej. 2.0)

    // ── Reducción de radio de detección en sigilo ─────────────
    // Nivel 2: moverse despacio reduce el radio de detección del robot
    float factorSigilo;       // 0.5 si lento, 1.0 si rápido
public:
    Personaje();
    Personaje(float x, float y);


    void saltar();
    void bajar();
    void mover(int key, bool is_valid);
    void actualizar(float dt) override;








    // ── Getters de estado ────────────────────────────────────
    int   getVidas()          const ;
    float getEnergia()        const ;
    bool  isEnSuelo()         const ;
    bool  isBoostActivo()     const ;
    bool  isDeslizando()      const ;
    float getFactorSigilo()   const ;

    ~Personaje() override;
};

#endif // PERSONAJE_H
