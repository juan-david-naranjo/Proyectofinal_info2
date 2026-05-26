#include "nivel_2.h"
#include "gestorfisicas.h"


#include <QPainter>
#include <cmath>
#include <algorithm>

Nivel_2::Nivel_2()
    : Nivel()
    , objetivoX(638.f)
    , objetivoY(314.f)
    , objetivoRadio(40.f)
    , spawnX(50.f)
    , spawnY(50.f)
    , escena(nullptr)
    , tiempoContador(0.f)
    ,itemObjetivo(nullptr)
{
    tiempoRestante = 180; // 3 minutos
    Escenario= new QPixmap(":/Kael_nivel2/Sprites/Nivel2/Escenario2.png");
    // Escenario->scaled(1250,660);
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

    // ── Cargar sonidos ────────────────────────────────────────────────────
    // Ajusta las rutas según tus recursos (qrc o ruta local)
    sonidoDeteccion.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_victoria.wav"));
    sonidoDeteccion.setVolume(0.9f);

    sonidoHackeoLoop.setSource(QUrl("qrc:/sonidoswav/Sonidos/Hacking.wav"));
    sonidoHackeoLoop.setLoopCount(QSoundEffect::Infinite);  // loop mientras hackeas
    sonidoHackeoLoop.setVolume(0.7f);

    sonidoVictoria.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_victoria.wav"));
    sonidoVictoria.setVolume(0.4f);

    // ── Inicializar estados anteriores de los robots ──────────────────────
    // Necesario para detectar el CAMBIO de estado (no el estado en sí)
    estadosAnteriores.assign(robots.size(), EstadoAgente::PATRULLAJE);

    musicaFondo.setAudioOutput(&audioFondo);
    musicaFondo.setSource(QUrl("qrc:/Sonidos/Sonidos/sonido_fondo.mp3"));
    audioFondo.setVolume(0.35f);       // suave para no tapar los efectos
    musicaFondo.setLoops(QMediaPlayer::Infinite);
    musicaFondo.play();

    if (escena)
        agregarItemsEscena();
}

// ── Generar paredes del laberinto ─────────────────────────────────────────────
// Rectángulos AABB que actúan como obstáculos (reutiliza Plataforma).
void Nivel_2::generarLaberinto()
{
    limpiarPlataformas();

    // ── Bordes del nivel (1250 × 700) ──────────────────────────────────────
    plataformas.push_back(new Plataforma(  0.f,   0.f, 1250.f,  20.f)); // Techo
    plataformas.push_back(new Plataforma(  0.f, 620.f, 1250.f,  20.f)); // Suelo
    plataformas.push_back(new Plataforma(  0.f,   0.f,  20.f, 700.f)); // Pared izq
    plataformas.push_back(new Plataforma(1250.f,   0.f,  20.f, 700.f)); // Pared der

    // ── Paredes internas del laberinto (vista cenital) ────────────────────
    // Estructura: {x, y, ancho, alto}
    struct Wall { float x, y, w, h; };
    static const Wall paredes[] =
        {
         // ── Corredor vertical izquierdo ──
         { 100.f,  20.f,  20.f, 200.f },
         { 100.f, 320.f,  20.f, 160.f },
         { 100.f, 580.f,  20.f, 100.f },

         // ── Corredor horizontal superior ──
         { 100.f,  20.f, 300.f,  20.f },
         { 500.f,  20.f, 280.f,  20.f },

         // ── Divisiones centrales ──
         { 200.f, 200.f,  20.f, 200.f },
         { 300.f, 300.f, 200.f,  20.f },

         // ── Sector 3 ──
         { 400.f,  20.f,  20.f, 280.f },
         { 500.f, 180.f,  20.f, 240.f },
         { 400.f, 480.f, 200.f,  20.f },

         // ── Sector 4 ──
         { 600.f,  20.f,  20.f, 160.f },
         { 540.f, 380.f, 240.f,  20.f },
         { 600.f, 480.f,  20.f, 200.f },

         // ── Corredor horizontal inferior ──
         { 120.f, 580.f, 280.f,  20.f },
         { 500.f, 580.f, 280.f,  20.f },

         // ── Bloques de cobertura (el jugador puede esconderse) ──
         { 160.f, 140.f,  60.f,  60.f },
         { 350.f, 120.f,  60.f,  60.f },
         { 460.f, 320.f,  60.f,  60.f },
         { 650.f, 560.f,  60.f,  60.f },
         };

    // for (const auto& w : paredes)
    //     plataformas.push_back(new Plataforma(w.x, w.y, w.w, w.h));
}

// ── Generar robots ────────────────────────────────────────────────────────────
void Nivel_2::generarRobots()
{
    limpiarRobots();

    // ── Robot 1: patrulla el sector izquierdo ─────────────────────────────
    std::vector<Punto2D> wp1 = {
        {50.f, 300.f}, {50.f, 500.f}, {50.f, 620.f}, {50.f, 100.f}
    };
    robots.push_back(new RobotSeguridad(
        50.f, 300.f,
        120.f,  // radioDeteccion
        160.f,  // radioDesenganche
        80.f,   // velPatrulla
        160.f,  // velPersecucion
        wp1
        ));

    // ── Robot 2: patrulla el sector central ──────────────────────────────
    std::vector<Punto2D> wp2 = {
        {340.f, 200.f}, {540.f, 200.f}, {540.f, 360.f}, {340.f, 360.f}
    };
    robots.push_back(new RobotSeguridad(
        340.f, 200.f,
        100.f,
        140.f,
        90.f,
        180.f,
        wp2
        ));

    // ── Robot 3: guarda la computadora (mayor radio, más rápido) ─────────
    std::vector<Punto2D> wp3 = {
        {660.f, 480.f}, {720.f, 540.f}, {680.f, 620.f}, {620.f, 560.f}
    };
    robots.push_back(new RobotSeguridad(
        660.f, 500.f,
        150.f,
        200.f,
        70.f,
        210.f,
        wp3
        ));
}





void Nivel_2::agregarItemsEscena()
{
    // ── 1. Paredes del laberinto ──────────────────────────────────────────────
    for (Plataforma* plat : plataformas)
    {
        Hitbox hb = plat->getHitbox();
        QGraphicsRectItem* rect = new QGraphicsRectItem(hb.x, hb.y, hb.w, hb.h);
        rect->setBrush(QBrush(QColor(30, 50, 90, 230)));
        rect->setPen(QPen(QColor(70, 110, 180, 200), 1));
        rect->setZValue(1.0);
        escena->addItem(rect);
        itemsParedes.append(rect);
    }

    // ── 2. Objetivo: la computadora ───────────────────────────────────────────
    itemObjetivo = new QGraphicsEllipseItem(
        objetivoX - objetivoRadio, objetivoY - objetivoRadio,
        objetivoRadio * 2.f, objetivoRadio * 2.f);
    itemObjetivo->setBrush(QBrush(QColor(0, 255, 120, 100)));
    itemObjetivo->setPen(QPen(QColor(0, 230, 90), 2));
    itemObjetivo->setZValue(2.0);
    escena->addItem(itemObjetivo);

    const float iconW = 36.f, iconH = 28.f;
    QGraphicsRectItem* pantalla = new QGraphicsRectItem(
        objetivoX - iconW * 0.5f, objetivoY - iconH * 0.5f - 4.f,
        iconW, iconH);
    pantalla->setBrush(QBrush(QColor(0, 200, 80)));
    pantalla->setPen(QPen(Qt::white, 1));
    pantalla->setZValue(2.1);
    escena->addItem(pantalla);

    // ── 3. Robots de seguridad ────────────────────────────────────────────────
    //
    //  Se carga la hoja de sprites UNA SOLA VEZ y se pasa a cada robot.
    //  Si los sprites del robot están en una hoja distinta a la del personaje,
    //  cambia la ruta en QPixmap robotSheet("...").
    //
    //  RUTA ACTUAL: misma hoja que el personaje.
    //  Ajusta si tus robots tienen su propio archivo.
    // ─────────────────────────────────────────────────────────────────────────
    QPixmap robotSheet(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");
    bool sheetOk = !robotSheet.isNull();

    if (!sheetOk)
        qDebug() << "WARN Nivel_2: no se pudo cargar la hoja de sprites de robots.";

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        RobotSeguridad* robot = robots[i];

        // ── Cargar sprites desde la hoja (llena framesPatrullaje y framesAlert)
        if (sheetOk)
            robot->cargarSprites(robotSheet);

        // ── Crear el QGraphicsPixmapItem con el primer frame disponible ───────
        QPixmap initPix = robot->getPrimerFrame();   // getter añadido a robotseguridad.h

        if (initPix.isNull())
        {
            // Fallback: cuadrado rojo si la hoja no cargó
            initPix = QPixmap(32, 32);
            initPix.fill(QColor(180, 30, 30));
        }

        QGraphicsPixmapItem* spriteItem = new QGraphicsPixmapItem(initPix);
        spriteItem->setTransformOriginPoint(initPix.width()  / 2.0,
                                            initPix.height() / 2.0);
        spriteItem->setPos(robot->getX(), robot->getY());
        spriteItem->setZValue(3.0);
        escena->addItem(spriteItem);

        // Asignar el ítem al robot para que actuar() lo mueva y anime
        robot->setItemGrafico(spriteItem);

        // ── Círculo de detección visual ────────────────────────────────────────
        float rd = robot->getRadioDeteccion();
        QGraphicsEllipseItem* circulo = new QGraphicsEllipseItem(
            robot->getX() + initPix.width()  * 0.5f - rd,
            robot->getY() + initPix.height() * 0.5f - rd,
            rd * 2.f, rd * 2.f);
        circulo->setBrush(QBrush(QColor(255, 60, 60, 25)));
        circulo->setPen(QPen(QColor(255, 60, 60, 100), 1, Qt::DashLine));
        circulo->setZValue(0.5);
        escena->addItem(circulo);
        itemsDeteccion.append(circulo);
    }

    // ── 4. Personaje (encima de todo) ─────────────────────────────────────────
    if (jugador && jugador->getItem())
    {
        jugador->getItem()->setZValue(4.0);
        escena->addItem(jugador->getItem());
    }
}









// ════════════════════════════════════════════════════════════════════════════
//  crearSpriteRobot  — dibuja un robot sencillo con QPainter
// ════════════════════════════════════════════════════════════════════════════
QPixmap Nivel_2::crearSpriteRobot(int w, int h)
{
    QPixmap pix(w, h);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Cuerpo hexagonal aproximado (círculo achatado)
    p.setBrush(QColor(160, 30, 30));
    p.setPen(QPen(QColor(220, 80, 80), 1.5));
    p.drawEllipse(2, 4, w - 4, h - 8);

    // Sensor / ojo central
    int eyeR = w / 6;
    p.setBrush(QColor(200, 230, 255));
    p.setPen(Qt::NoPen);
    p.drawEllipse(w / 2 - eyeR, h / 2 - eyeR - 1, eyeR * 2, eyeR * 2);

    // Pupila
    int pupR = std::max(1, eyeR / 2);
    p.setBrush(Qt::black);
    p.drawEllipse(w / 2 - pupR, h / 2 - pupR - 1, pupR * 2, pupR * 2);

    // Antenas
    p.setPen(QPen(QColor(180, 60, 60), 1));
    p.drawLine(w / 2 - 5, 4, w / 2 - 7, 0);
    p.drawLine(w / 2 + 5, 4, w / 2 + 7, 0);

    p.end();
    return pix;
}


// ════════════════════════════════════════════════════════════════════════════
//  resolverColisiones  — empuja al jugador fuera de las paredes (AABB)
// ════════════════════════════════════════════════════════════════════════════
void Nivel_2::resolverColisiones()
{
    if (!jugador) return;

    float jx  = jugador->getX();
    float jy  = jugador->getY();
    float jvx = jugador->getVx();
    float jvy = jugador->getVy();
    float jw  = jugador->getAncho();
    float jh  = jugador->getAlto();
    bool  dummy = false;

    for (Plataforma* plat : plataformas)
    {
        Hitbox hb = plat->getHitbox();
        GestorFisicas::resolverColision(
            jx, jy, jw, jh,
            jvx, jvy, dummy,
            hb.x, hb.y, hb.w, hb.h);
    }

    jugador->setPosicion(jx, jy);
    jugador->setVelocidad(jvx, jvy);
}

// ════════════════════════════════════════════════════════════════════════════
//  actualizarCirculosDeteccion  — mueve y recolorea los anillos de radar
//  cada tick, reflejando el estado actual de cada robot (PATRULLAJE / PERSECUCION)
// ════════════════════════════════════════════════════════════════════════════
void Nivel_2::actualizarCirculosDeteccion()
{
    for (int i = 0; i < (int)robots.size() && i < itemsDeteccion.size(); i++)
    {
        RobotSeguridad* robot  = robots[i];
        QGraphicsEllipseItem* circulo = itemsDeteccion[i];

        float rd = robot->getRadioDeteccion();
        float cx = robot->getX() + 16.f;   // Centro del sprite (32×32)
        float cy = robot->getY() + 16.f;

        circulo->setRect(cx - rd, cy - rd, rd * 2.f, rd * 2.f);

        // Rojo intenso cuando está persiguiendo; gris tenue en patrullaje
        if (robot->getEstado() == EstadoAgente::PERSECUCION)
        {
            circulo->setBrush(QBrush(QColor(255, 30, 30, 70)));
            circulo->setPen(QPen(QColor(255, 40, 40, 220), 2, Qt::SolidLine));
        }
        else
        {
            circulo->setBrush(QBrush(QColor(255, 60, 60, 25)));
            circulo->setPen(QPen(QColor(255, 60, 60, 100), 1, Qt::DashLine));
        }
    }
}






// ── Actualizar ────────────────────────────────────────────────────────────────
void Nivel_2::actualizar(float dt)
{
    if (!jugador) return;

    // ── Timer: usar acumulador para descontar segundos exactos ────────────
    // BUG ORIGINAL: (int)dt siempre es 0 a 60 fps (dt ≈ 0.016).
    // CORRECCIÓN: acumular y descontar solo cuando se completa un segundo.
    tiempoContador += dt;
    if (tiempoContador >= 1.f)
    {
        int segundosTranscurridos  = static_cast<int>(tiempoContador);
        tiempoRestante            -= segundosTranscurridos;
        tiempoContador            -= static_cast<float>(segundosTranscurridos);
        if (tiempoRestante < 0) tiempoRestante = 0;
    }

    // ── Actualizar personaje (físicas nivel 2: inercia + 4 direcciones) ───
    jugador->actualizarNivel2(dt);

    // ── Resolver colisiones del jugador con las paredes ───────────────────
    resolverColisiones();

    // ── Tick de cada robot: percibir → razonar → actuar ──────────────────
    // Se pasa el centro del jugador (más preciso que la esquina)
    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    for (int i = 0; i < (int)robots.size(); i++)
    {
        RobotSeguridad* robot = robots[i];
        robot->tick(jx, jy, dt);

        // Colisiones del robot con las paredes
        float rx  = robot->getX();
        float ry  = robot->getY();
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

    // ── Actualizar círculos de detección en la escena ─────────────────────
    actualizarCirculosDeteccion();

    // ── Comprobar condiciones de fin ──────────────────────────────────────
    verificarDeteccion();
    verificarVictoria(dt);
}




// ════════════════════════════════════════════════════════════════════════════
//  actualizarCirculosDeteccion  — mueve y recolorea los anillos de radar
//  cada tick, reflejando el estado actual de cada robot (PATRULLAJE / PERSECUCION)
// ════════════════════════════════════════════════════════════════════════════
// void Nivel_2::actualizarCirculosDeteccion()
// {
//     for (int i = 0; i < (int)robots.size() && i < itemsDeteccion.size(); i++)
//     {
//         RobotSeguridad* robot  = robots[i];
//         QGraphicsEllipseItem* circulo = itemsDeteccion[i];

//         float rd = robot->getRadioDeteccion();
//         float cx = robot->getX() + 16.f;   // Centro del sprite (32×32)
//         float cy = robot->getY() + 16.f;

//         circulo->setRect(cx - rd, cy - rd, rd * 2.f, rd * 2.f);

//         // Rojo intenso cuando está persiguiendo; gris tenue en patrullaje
//         if (robot->getEstado() == EstadoAgente::PERSECUCION)
//         {
//             circulo->setBrush(QBrush(QColor(255, 30, 30, 70)));
//             circulo->setPen(QPen(QColor(255, 40, 40, 220), 2, Qt::SolidLine));
//         }
//         else
//         {
//             circulo->setBrush(QBrush(QColor(255, 60, 60, 25)));
//             circulo->setPen(QPen(QColor(255, 60, 60, 100), 1, Qt::DashLine));
//         }
//     }
// }




// ── Detección: si un robot toca al jugador → daño + respawn ──────────────────
void Nivel_2::verificarDeteccion()
{
    // if (!jugador) return;

    // float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    // float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    // for (RobotSeguridad* robot : robots)
    // {
    //     // Colisión entre el centro del robot y el centro del jugador
    //     if (GestorFisicas::colisionCirculo(
    //             robot->getX() + 16.f, robot->getY() + 16.f,
    //             jx, jy,
    //             30.f))   // radio de contacto
    //     {
    //         jugador->recibirDanio(1);
    //         jugador->resetearPosicion(spawnX, spawnY);
    //         break;  // Un solo impacto por tick
    //     }
    // }
    if (!jugador) return;

    // ── Detectar CAMBIO de estado PATRULLAJE → PERSECUCION ────────────────
    // Se compara el estado actual con el del tick anterior.
    // Así el sonido suena UNA sola vez al detectar, no cada frame.
    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        EstadoAgente estadoActual = robots[i]->getEstado();

        if (estadosAnteriores[i] == EstadoAgente::PATRULLAJE &&
            estadoActual          == EstadoAgente::PERSECUCION)
        {
            sonidoDeteccion.play();   // ¡Robot te vio!
        }

        estadosAnteriores[i] = estadoActual;  // actualizar para el próximo tick
    }

    // ── Colisión robot → jugador : daño + respawn ─────────────────────────
    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    for (RobotSeguridad* robot : robots)
    {
        if (GestorFisicas::colisionCirculo(
                robot->getX() + 16.f, robot->getY() + 16.f,
                jx, jy, 30.f))
        {
            jugador->recibirDanio(1);
            jugador->resetearPosicion(spawnX, spawnY);

            // Si te atrapan mientras hackeabas, el progreso se pierde
            tiempoHackeo  = 0.f;
            haciendoHackeo = false;
            sonidoHackeoLoop.stop();

            break;
        }
    }
}

// ── Victoria: jugador llega a la computadora ──────────────────────────────────
void Nivel_2::verificarVictoria(float dt)
{ if (!jugador) return;

    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    bool cercaDeComputadora = GestorFisicas::colisionCirculo(
        jx, jy, objetivoX, objetivoY, objetivoRadio);

    if (cercaDeComputadora)
    {
        // Primera vez que entra al radio → arrancar sonido de hackeo
        if (!haciendoHackeo)
        {
            haciendoHackeo = true;
            sonidoHackeoLoop.play();
        }

        tiempoHackeo += dt;

        // ── Hackeo completado ─────────────────────────────────────────────
        if (tiempoHackeo >= tiempoHackeoMax)
        {
             musicaFondo.stop();
            sonidoHackeoLoop.stop();
            sonidoVictoria.play();
            completado = true;
        }
    }
    else
    {
        // El jugador se alejó antes de terminar → reiniciar progreso
        if (haciendoHackeo)
        {
            tiempoHackeo   = 0.f;
            haciendoHackeo = false;
            sonidoHackeoLoop.stop();
        }
    }
}


//────────────GETTERS────────────────
int Nivel_2::getTiempoRestante() const
{
    return tiempoRestante;
}

bool Nivel_2::isCompletado() const
{
    return completado;
}









void Nivel_2::setScene(QGraphicsScene *scene)
{
    escena = scene;
    scene->setSceneRect(0, 0, 1250, 700);

    if (Escenario && !Escenario->isNull())
        scene->setBackgroundBrush(*Escenario);
    else
        scene->setBackgroundBrush(QColor(20, 28, 40));  // Fallback oscuro

}
