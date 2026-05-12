#include "personaje.h"
#include "gestorfisicas.h"
#include <algorithm>
#include <cmath>

Personaje::Personaje() : EntidadJuego(0, 0)
{
    vidas          = 3;
    energia        = 100.f;
    velMax         = 200.f;      // px/s
    enSuelo        = false;
    std::fill(std::begin(keys), std::end(keys), false);

    puedeDoubleSalto = true;
    fuerzaSalto      = 450.f;   // px/s inicial hacia arriba
    saltosRestantes  = 2;        // doble salto habilitado

    deslizando       = false;
    tiempoDesliz     = 0.f;

    boostActivo      = false;
    tiempoBoost      = 0.f;

    factorSigilo     = 1.f;

    Sprite       = nullptr;
    itemGrafico  = nullptr;
}

Personaje::Personaje(float X, float Y) : Personaje()
{
    x = X;
    y = Y;

    itemGrafico = new QGraphicsPixmapItem();
    Sprite      = new QPixmap(":/Kael_nivel1/Sprites/Nivel1/sprites nivel 1 kael.png");

    if (!Sprite->isNull())
    {
        itemGrafico->setPixmap(Sprite->copy(43, 92, (int)ANCHO, (int)ALTO));
    }
    itemGrafico->setPos(x, y);
}

Personaje::~Personaje()
{
    delete Sprite;
}

// ── Entrada ──────────────────────────────────────────────────────────────────
void Personaje::keyPressed(int key)
{
    if (key >= 0 && key < 4) keys[key] = true;
}

void Personaje::keyReleased(int key)
{
    if (key >= 0 && key < 4) keys[key] = false;
}

// ── Salto ─────────────────────────────────────────────────────────────────────
void Personaje::saltar()
{
    if (saltosRestantes <= 0) return;

    Vy = -fuerzaSalto;       // Negativo: sube en Qt (Y crece hacia abajo)
    enSuelo = false;
    saltosRestantes--;

    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Boost ─────────────────────────────────────────────────────────────────────
void Personaje::activarBoost()
{
    boostActivo  = true;
    tiempoBoost  = DURACION_BOOST;
}

// ── Deslizamiento ─────────────────────────────────────────────────────────────
void Personaje::activarDesliz()
{
    if (enSuelo && !deslizando)
    {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;
    }
}

// ── actualizar genérico (usa nivel 1 por defecto) ─────────────────────────────
void Personaje::actualizar(float dt)
{
    actualizarNivel1(dt, 0.f);
}

// ── Nivel 1 ───────────────────────────────────────────────────────────────────
void Personaje::actualizarNivel1(float dt, float tiempoTotal)
{
    // --- Velocidad horizontal según input ---
    float velHoriz = 0.f;
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    if (keys[0]) velHoriz = -velEfectiva;
    if (keys[1]) velHoriz =  velEfectiva;

    // Aplicar fricción solo si está en suelo y no hay input
    if (enSuelo && std::abs(velHoriz) < 0.01f)
    {
        GestorFisicas::aplicarFriccion(Vx, dt);
    }
    else
    {
        Vx = velHoriz;
    }

    // --- Viento oscilatorio (empuja horizontalmente) ---
    // Solo si está en el aire (el suelo amortigua el viento)
    if (!enSuelo)
        GestorFisicas::aplicarViento(Vx, tiempoTotal, dt);

    // --- Gravedad + posición vertical ---
    // El nivel resuelve colisiones DESPUÉS de aplicarGravedad
    GestorFisicas::aplicarGravedad(Vy, y, dt);

    // --- Posición horizontal ---
    x += Vx * dt;

    // --- Boost: descontar tiempo ---
    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f)
        {
            boostActivo = false;
            tiempoBoost = 0.f;
        }
    }

    // --- Sincronizar sprite ---
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Nivel 2 ───────────────────────────────────────────────────────────────────
void Personaje::actualizarNivel2(float dt)
{
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    // --- Deslizamiento ---
    if (deslizando)
    {
        tiempoDesliz -= dt;
        if (tiempoDesliz <= 0.f)
        {
            deslizando   = false;
            tiempoDesliz = 0.f;
        }
        // Durante el desliz Vx se mantiene y decelera por inercia
        GestorFisicas::aplicarInercia(Vx, Vx * 0.5f, dt);
    }
    else
    {
        // Movimiento normal con inercia
        float velObjetivo = 0.f;
        if (keys[0]) velObjetivo = -velEfectiva;
        if (keys[1]) velObjetivo =  velEfectiva;
        GestorFisicas::aplicarInercia(Vx, velObjetivo, dt);

        float velObjetivoY = 0.f;
        if (keys[2]) velObjetivoY = -velEfectiva;
        if (keys[3]) velObjetivoY =  velEfectiva;
        GestorFisicas::aplicarInercia(Vy, velObjetivoY, dt);
    }

    x += Vx * dt;
    y += Vy * dt;

    // --- Factor de sigilo: si va lento, radio de detección del robot se reduce ---
    float velTotal = std::sqrt(Vx*Vx + Vy*Vy);
    factorSigilo = (velTotal < velMax * 0.4f) ? 0.5f : 1.f;

    // --- Boost ---
    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }

    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Colisión con suelo/plataforma ─────────────────────────────────────────────
void Personaje::aterrizarEnSuelo(float suloY)
{
    y       = suloY - ALTO;
    Vy      = 0.f;
    enSuelo = true;
    saltosRestantes = 2;    // Recargar doble salto
    if (itemGrafico) itemGrafico->setPos(x, y);
}

void Personaje::despegarSuelo()
{
    enSuelo = false;
}

// ── Daño / reset ──────────────────────────────────────────────────────────────
void Personaje::recibirDanio(int cantidad)
{
    vidas -= cantidad;
    if (vidas < 0) vidas = 0;
}

void Personaje::resetearPosicion(float rx, float ry)
{
    x = rx; y = ry;
    Vx = 0; Vy = 0;
    enSuelo = false;
    saltosRestantes = 2;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Getters ───────────────────────────────────────────────────────────────────
int   Personaje::getVidas()        const { return vidas;        }
float Personaje::getEnergia()      const { return energia;      }
bool  Personaje::isEnSuelo()       const { return enSuelo;      }
bool  Personaje::isBoostActivo()   const { return boostActivo;  }
bool  Personaje::isDeslizando()    const { return deslizando;   }
float Personaje::getFactorSigilo() const { return factorSigilo; }
