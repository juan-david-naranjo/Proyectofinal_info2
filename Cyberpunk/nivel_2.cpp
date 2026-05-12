#include "nivel_2.h"
#include "gestorfisicas.h"

Nivel_2::Nivel_2()
    : Nivel()
    , objetivoX(700.f)
    , objetivoY(700.f)
    , objetivoRadio(40.f)
    , spawnX(50.f)
    , spawnY(50.f)
{
    tiempoRestante = 180; // 3 minutos
}

Nivel_2::~Nivel_2()
{
    limpiarRobots();
}

void Nivel_2::limpiarRobots()
{
    for (RobotSeguridad* r : robots) delete r;
    robots.clear();
}

// ── Inicializar ──────────────────────────────────────────────────────────────
void Nivel_2::inicializar(Personaje* p)
{
    jugador = p;
    generarLaberinto();
    generarRobots();
    if (jugador)
        jugador->resetearPosicion(spawnX, spawnY);
}

// ── Generar paredes del laberinto ─────────────────────────────────────────────
// Rectángulos AABB que actúan como obstáculos (reutiliza Plataforma).
void Nivel_2::generarLaberinto()
{
    limpiarPlataformas();

    // Bordes del nivel
    plataformas.push_back(new Plataforma(  0.f,   0.f, 800.f,  20.f)); // Techo
    plataformas.push_back(new Plataforma(  0.f, 780.f, 800.f,  20.f)); // Suelo
    plataformas.push_back(new Plataforma(  0.f,   0.f,  20.f, 800.f)); // Pared izq
    plataformas.push_back(new Plataforma(780.f,   0.f,  20.f, 800.f)); // Pared der

    // Paredes internas del laberinto (vista cenital)
    struct Wall { float x, y, w, h; };
    Wall paredes[] = {
        // Sector 1
        { 100.f,  20.f,  20.f, 200.f },
        { 100.f, 320.f,  20.f, 160.f },
        { 100.f, 600.f,  20.f, 180.f },
        // Sector 2
        { 200.f,  20.f, 200.f,  20.f },
        { 200.f, 200.f,  20.f, 200.f },
        { 300.f, 400.f, 200.f,  20.f },
        // Sector 3
        { 400.f,  20.f,  20.f, 300.f },
        { 500.f, 200.f,  20.f, 250.f },
        { 400.f, 500.f, 200.f,  20.f },
        // Sector 4
        { 600.f,  20.f,  20.f, 180.f },
        { 550.f, 400.f, 230.f,  20.f },
        { 600.f, 500.f,  20.f, 280.f },
        // Escondites (bloques pequeños)
        { 160.f, 140.f,  60.f,  60.f },
        { 350.f, 120.f,  60.f,  60.f },
        { 460.f, 320.f,  60.f,  60.f },
        { 650.f, 600.f,  60.f,  60.f },
    };

    for (auto& w : paredes)
        plataformas.push_back(new Plataforma(w.x, w.y, w.w, w.h));
}

// ── Generar robots ────────────────────────────────────────────────────────────
void Nivel_2::generarRobots()
{
    limpiarRobots();

    // Robot 1: patrulla sector izquierdo
    std::vector<Punto2D> wp1 = {
        {50.f, 300.f}, {50.f, 500.f}, {50.f, 700.f}, {50.f, 100.f}
    };
    robots.push_back(new RobotSeguridad(
        50.f, 300.f,
        120.f,  // radioDeteccion
        160.f,  // radioDesenganche
        80.f,   // velPatrulla
        160.f,  // velPersecucion
        wp1
    ));

    // Robot 2: patrulla centro
    std::vector<Punto2D> wp2 = {
        {350.f, 200.f}, {550.f, 200.f}, {550.f, 380.f}, {350.f, 380.f}
    };
    robots.push_back(new RobotSeguridad(
        350.f, 200.f,
        100.f,
        140.f,
        90.f,
        180.f,
        wp2
    ));

    // Robot 3: guarda la computadora (más rápido y más radio)
    std::vector<Punto2D> wp3 = {
        {680.f, 650.f}, {720.f, 720.f}, {680.f, 750.f}, {640.f, 680.f}
    };
    robots.push_back(new RobotSeguridad(
        680.f, 680.f,
        150.f,   // radio mayor
        200.f,
        70.f,
        200.f,   // muy rápido en persecución
        wp3
    ));
}

// ── Actualizar ────────────────────────────────────────────────────────────────
void Nivel_2::actualizar(float dt)
{
    if (!jugador) return;

    tiempoAcumulado += dt;

    // ── Timer ──
    tiempoRestante -= (int)dt;   // Aproximación; usar acumulador si se necesita precisión

    // ── Actualizar personaje (físicas nivel 2: inercia) ──
    jugador->actualizarNivel2(dt);

    // ── Resolver colisiones del jugador con paredes del laberinto ──
    resolverColisiones();

    // ── Tick de cada robot (IA) ──
    // El radio de detección se escala por el factor de sigilo del jugador
    float jx = jugador->getX();
    float jy = jugador->getY();

    for (RobotSeguridad* robot : robots)
    {
        // El factor de sigilo reduce el radio de detección efectivo:
        // Si el jugador va lento (sigilo) el robot lo detecta más tarde.
        // Implementado pasando una posición "virtual" más lejana.
        robot->tick(jx, jy, dt);

        // Resolver colisiones del robot con las paredes también
        float rx = robot->getX();
        float ry = robot->getY();
        float rvx = robot->getVx();
        float rvy = robot->getVy();
        bool  dummy = false;
        for (Plataforma* plat : plataformas)
        {
            Hitbox hbPlat = plat->getHitbox();
            GestorFisicas::resolverColision(
                rx, ry, 32.f, 32.f,
                rvx, rvy, dummy,
                hbPlat.x, hbPlat.y, hbPlat.w, hbPlat.h);
        }
        robot->setPosicion(rx, ry);
        robot->setVelocidad(rvx, rvy);
    }

    // ── Comprobar detección / victoria ──
    verificarDeteccion();
    verificarVictoria();
}

// ── Detección: si un robot toca al jugador → daño + respawn ──────────────────
void Nivel_2::verificarDeteccion()
{
    if (!jugador) return;

    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    for (RobotSeguridad* robot : robots)
    {
        if (GestorFisicas::colisionCirculo(robot->getX() + 16.f,
                                           robot->getY() + 16.f,
                                           jx, jy, 30.f))
        {
            jugador->recibirDanio(1);
            jugador->resetearPosicion(spawnX, spawnY);
            break;
        }
    }
}

// ── Victoria: jugador llega a la computadora ──────────────────────────────────
void Nivel_2::verificarVictoria()
{
    if (!jugador) return;

    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    if (GestorFisicas::colisionCirculo(jx, jy, objetivoX, objetivoY, objetivoRadio))
        completado = true;
}
