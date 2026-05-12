#include "nivel_1.h"
#include "gestorfisicas.h"

Nivel_1::Nivel_1()
    : Nivel()
    , velocidadScroll(0.f)
    , puertaCerrada(false)
    , timerAcumulado(0.f)
    , spawnX(100.f)
    , spawnY(700.f)
{
    tiempoRestante = TIEMPO_NIVEL;
}

Nivel_1::~Nivel_1() {}

// ── Inicializar ──────────────────────────────────────────────────────────────
void Nivel_1::inicializar(Personaje* p)
{
    jugador = p;
    generarPlataformas();
    if (jugador)
        jugador->resetearPosicion(spawnX, spawnY);
}

// ── Generar plataformas ───────────────────────────────────────────────────────
// Disposición vertical tipo Jump King: plataformas escalonadas hacia arriba.
// Las últimas filas tienen plataformas más angostas y separadas.
void Nivel_1::generarPlataformas()
{
    limpiarPlataformas();

    // Suelo base
    plataformas.push_back(new Plataforma(0.f, 780.f, 800.f, 30.f));

    // Escalera de plataformas (Y va disminuyendo = sube en pantalla)
    struct PlatData { float x, y, w; bool movil; };
    PlatData datos[] = {
        {  50.f, 650.f, 180.f, false },
        { 300.f, 580.f, 140.f, false },
        { 100.f, 510.f, 160.f, true  },   // móvil
        { 350.f, 440.f, 120.f, false },
        {  80.f, 370.f, 130.f, false },
        { 400.f, 300.f, 100.f, true  },   // móvil
        { 200.f, 230.f, 110.f, false },
        { 500.f, 160.f, 100.f, false },
        { 300.f,  90.f, 140.f, false },   // Meta / puerta
    };

    for (auto& d : datos)
        plataformas.push_back(new Plataforma(d.x, d.y, d.w, 22.f, d.movil));
}

// ── Actualizar ────────────────────────────────────────────────────────────────
void Nivel_1::actualizar(float dt)
{
    if (!jugador) return;

    // ── Timer del nivel ──
    timerAcumulado += dt;
    if (timerAcumulado >= 1.f)
    {
        timerAcumulado -= 1.f;
        tiempoRestante--;
        if (tiempoRestante <= 0)
        {
            tiempoRestante = 0;
            puertaCerrada  = true;
        }
    }

    tiempoAcumulado += dt;

    // ── Actualizar plataformas móviles ──
    for (Plataforma* plat : plataformas)
        plat->actualizar(dt);

    // ── Actualizar personaje (físicas nivel 1) ──
    jugador->actualizarNivel1(dt, tiempoAcumulado);

    // ── Resolver colisiones ──
    resolverColisiones();

    // ── Verificar caída ──
    verificarCaida();

    // ── Verificar victoria ──
    // Si el jugador alcanza la plataforma más alta (y < 120) → completado
    if (jugador->getY() < 120.f && !puertaCerrada)
        completado = true;
}

// ── Verificar caída ───────────────────────────────────────────────────────────
// Si el jugador cae por debajo del suelo (y > 820), reinicia.
void Nivel_1::verificarCaida()
{
    if (!jugador) return;
    if (jugador->getY() > 820.f)
    {
        jugador->recibirDanio(1);
        jugador->resetearPosicion(spawnX, spawnY);
    }
}
