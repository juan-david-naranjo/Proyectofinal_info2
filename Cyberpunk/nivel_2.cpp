#include "nivel_2.h"
#include "gestorfisicas.h"


#include <QPainter>
#include <cmath>
#include <algorithm>

Nivel_2::Nivel_2()
    : Nivel()
    , objetivoX(1170.f)
    , objetivoY(308.f)
    , objetivoRadio(40.f)
    , spawnX(677.f)
    , spawnY(147.f)
    , escena(nullptr)
    , tiempoContador(0.f)
    ,itemObjetivo(nullptr)
{
    tiempoRestante = 180; // 3 minutos
    Escenario= new QPixmap(":/Kael_nivel2/Sprites/Nivel2/Escenario_V2.png");

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
    jugador->setHitboxOffset(20.f,15.f,50.f, 100.f);  // baja 15px, alto efectivo 90px

    generarLaberinto();
    generarRobots();

    vidasN2 = vidasN2Max;           //modificable
    tiempoInvulnerable = 0.f;

    sonidoDanio.setSource(QUrl("qrc:/sonidoswav/Sonidos/hurtwav.wav"));
    sonidoDanio.setVolume(1.0f);

    if (jugador)
        jugador->resetearPosicion(spawnX, spawnY);

    // ── Cargar sonidos ────────────────────────────────────────────────────
    // Ajusta las rutas según tus recursos (qrc o ruta local)
    sonidoDeteccion.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_persecusion_wav.wav"));
    sonidoDeteccion.setVolume(0.9f);

    sonidoHackeoLoop.setSource(QUrl("qrc:/sonidoswav/Sonidos/Hacking.wav"));
    sonidoHackeoLoop.setLoopCount(QSoundEffect::Infinite);  // loop mientras hackeas
    sonidoHackeoLoop.setVolume(0.5f);

    sonidoVictoria.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_victoria.wav"));
    sonidoVictoria.setVolume(0.4f);

    // ── Inicializar estados anteriores de los robots ──────────────────────
    // Necesario para detectar el CAMBIO de estado (no el estado en sí)
    estadosAnteriores.assign(robots.size(), EstadoAgente::PATRULLAJE);

    musicaFondo.setAudioOutput(&audioFondo);
    musicaFondo.setSource(QUrl("qrc:/sonidoswav/Sonidos/End of Line (From TRON_ LegacyScore).mp3"));
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
    plataformas.push_back(new Plataforma(  20.f,   0.f, 1295.f,  30.f,false,Plataforma::TipoMuro::HORIZONTAL)); // Techo
    plataformas.push_back(new Plataforma(  20.f, 815.f, 1295.f,  30.f,false,Plataforma::TipoMuro::HORIZONTAL)); // Suelo
    plataformas.push_back(new Plataforma(  0.f,   0.f,  30.f, 815.f,false,Plataforma::TipoMuro::VERTICAL)); // Pared izq
    plataformas.push_back(new Plataforma(1295.f,   0.f,  30.f, 815.f,false,Plataforma::TipoMuro::VERTICAL)); // Pared der

    // ── Paredes internas del laberinto (vista cenital) ────────────────────
    // Estructura: {x, y, ancho, alto}
     struct Wall { float x, y, w, h; Plataforma::TipoMuro tipo; };
    static const Wall paredes[] =
        {
        {  292.f, 202.f, 1003.f,  15.f, Plataforma::TipoMuro::HORIZONTAL },
        {   16.f, 236.f,  189.f,  16.f, Plataforma::TipoMuro::HORIZONTAL },
        {  312.f, 586.f,  334.f,  20.f, Plataforma::TipoMuro::HORIZONTAL },
        {  771.f, 586.f,  524.f,  20.f, Plataforma::TipoMuro::HORIZONTAL },
        {  451.f, 460.f,  340.f,  15.f, Plataforma::TipoMuro::HORIZONTAL },
        // Verticales
        {  292.f, 202.f,   20.f, 403.f, Plataforma::TipoMuro::VERTICAL   },
        {  771.f, 486.f,   20.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
        {  451.f, 360.f,   20.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
        {  617.f, 217.f,   15.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
        {  727.f, 217.f,   15.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
        {  873.f, 217.f,   15.f, 250.f, Plataforma::TipoMuro::VERTICAL   },
        { 1034.f, 386.f,   15.f, 200.f, Plataforma::TipoMuro::VERTICAL   },
          };

    for (const auto& w : paredes)
        plataformas.push_back(
            new Plataforma(w.x, w.y, w.w, w.h, false, w.tipo));


    zonasOcultas = {
                    { 8.f, 66.f, 150.f, 100.f },
                    { 26.f, 660.f, 150.f, 150.f },
                    { 471.f, 360.f, 100.f, 100.f },
                    };

    for (const auto& z : zonasOcultas)
    {
        auto* item = escena->addRect(z.x, z.y, z.w, z.h,
                                     QPen(Qt::NoPen),
                                     QBrush(QColor(0, 0, 0, 160))); // negro semitransparente
        item->setZValue(1);   // encima del suelo, debajo del personaje
        itemsZonas.push_back(item);
    }
}

// ── Generar robots ────────────────────────────────────────────────────────────
void Nivel_2::generarRobots()
{
    limpiarRobots();

    // ── Robot 1: patrulla el sector izquierdo ─────────────────────────────
    std::vector<Punto2D> wp1 = {
                                {220.f, 96.f}, {220.f, 440.f},{220.f,96.f},{500.f,96.f}
    };

    //{196.f, 96.f}, {956.f, 96.f},
    robots.push_back(new RobotSeguridad(
        200.f, 96.f,
        120.f,  // radioDeteccion
        160.f,  // radioDesenganche
        80.f,   // velPatrulla
        160.f,  // velPersecucion
        wp1
        ));

    // ── Robot 2: patrulla el sector central ──────────────────────────────
    std::vector<Punto2D> wp2 = {
                                {1253.f, 715.f}, {184.f, 715.f},
                                {700.f, 715.f}, {700.f, 512.f},
                                {700.f,715.f}
    };
    robots.push_back(new RobotSeguridad(
        1253.f, 718.f,
        100.f,
        140.f,
        90.f,
        180.f,
        wp2
        ));

    // ── Robot 3: guarda la computadora (mayor radio, más rápido) ─────────
    std::vector<Punto2D> wp3 = {
        {676.f, 250.f}, {676.f, 380.f},         //up-down

        {566.f,380.f},        //abajo-izquierda

        {566.f, 250.f},      //ahora-sube

        {366.f,250.f},

        {566.f,250.f},

        {566.f,380.f},

        {800.f,380.f},

        {800.f,480.f},

        {800.f,380.f},

        {676.f,380.f}

    };
    robots.push_back(new RobotSeguridad(
        676.f, 250.f,
        50.f,
        200.f,
        70.f,
        210.f,
        wp3
        ));
}





void Nivel_2::agregarItemsEscena()
{
    QPixmap hojaMuros(":/Kael_nivel2/Sprites/Nivel2/murosV2.png");
    if (hojaMuros.isNull()) qDebug() << "WARN: hoja de muros no cargó";
    if (!itemsParedes.empty()) return;
    for (Plataforma* plat : plataformas)
    {
        Hitbox hb = plat->getHitbox();

        if (plat->tipoMuro == Plataforma::TipoMuro::SIN_SPRITE)
        {
            QGraphicsRectItem* rect = new QGraphicsRectItem(
                hb.x, hb.y, hb.w, hb.h);
            rect->setBrush(QBrush(QColor(30, 50, 90, 230)));
            rect->setPen(QPen(QColor(70, 110, 180, 200), 1));
            rect->setZValue(1.0);
            escena->addItem(rect);
            itemsParedes.push_back(rect);
        }
        else if (plat->tipoMuro == Plataforma::TipoMuro::HORIZONTAL)
        {
            // ← aquí colocas tus coordenadas del sprite horizontal
            plat->cargarSprite(hojaMuros, 701, 127, 231, 108);

            if (plat->getItem()) {
                escena->addItem(plat->getItem());
                itemsParedes.push_back(plat->getItem());
            }
        }
        else if (plat->tipoMuro == Plataforma::TipoMuro::VERTICAL)
        {
            // ← aquí colocas tus coordenadas del sprite vertical
            plat->cargarSprite(hojaMuros, 965, 127, 138, 217);

            if (plat->getItem()) {
                escena->addItem(plat->getItem());
                itemsParedes.push_back(plat->getItem());
            }
        }
    }

    // ── 2. Objetivo: la computadora ───────────────────────────────────────────

    QPixmap hojaObjetivo(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");
    if (hojaObjetivo.isNull())
        qDebug() << "WARN: hoja de computadora no cargó";

    // ← el desarrollador coloca sus coordenadas aquí
    cargarSpriteObjetivo(hojaObjetivo, 731, 557, 137, 140);

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
        itemsDeteccion.push_back(circulo);
    }

    // ── 4. Personaje (encima de todo) ─────────────────────────────────────────
    if (jugador && jugador->getItem())
    {
        jugador->getItem()->setZValue(4.0);
        escena->addItem(jugador->getItem());
    }

    // ── HUD: Timer centrado arriba ────────────────────────────────────────────
    itemHUDTimer = new QGraphicsTextItem("3:00");
    itemHUDTimer->setDefaultTextColor(Qt::white);
    itemHUDTimer->setFont(QFont("Consolas", 22, QFont::Bold));

    // Centrar horizontalmente
    float timerW = itemHUDTimer->boundingRect().width();
    itemHUDTimer->setPos(escena->width() * 0.5f - timerW * 0.5f, 12.f);
    itemHUDTimer->setZValue(20.0);
    escena->addItem(itemHUDTimer);

    // ── HUD: Corazones arriba a la izquierda ──────────────────────────────────
    itemsCorazones.clear();
    for (int i = 0; i < vidasN2Max; i++)
    {
        QGraphicsEllipseItem* corazon = new QGraphicsEllipseItem(0, 0, 22, 22);
        corazon->setBrush(QBrush(QColor(220, 40, 40)));    // rojo = vida activa
        corazon->setPen(QPen(QColor(255, 100, 100), 1));
        corazon->setPos(14.f + i * 30.f, 14.f);
        corazon->setZValue(20.0);
        escena->addItem(corazon);
        itemsCorazones.push_back(corazon);
    }




}


void Nivel_2::actualizarHUD()
{
    // ── Timer ─────────────────────────────────────────────────────────────
    if (itemHUDTimer)
    {
        int mins = tiempoRestante / 60;
        int secs = tiempoRestante % 60;
        QString txt = QString("%1:%2")
                          .arg(mins)
                          .arg(secs, 2, 10, QChar('0'));  // "2:05"

        // Parpadeo rojo en los últimos 30 segundos
        if (tiempoRestante <= 30)
        {
            bool parpadea = (static_cast<int>(tiempoContador * 4.f) % 2 == 0);
            itemHUDTimer->setDefaultTextColor(parpadea ? QColor(255, 60, 60)
                                                       : QColor(255, 160, 160));
        }
        else
            itemHUDTimer->setDefaultTextColor(Qt::white);

        itemHUDTimer->setPlainText(txt);
    }

    // ── Corazones ─────────────────────────────────────────────────────────
    for (int i = 0; i < static_cast<int>(itemsCorazones.size()); i++)
    {
        if (i < vidasN2)
            // Vida activa: rojo
            itemsCorazones[i]->setBrush(QBrush(QColor(220, 40, 40)));
        else
            // Vida perdida: gris oscuro
            itemsCorazones[i]->setBrush(QBrush(QColor(60, 60, 60)));
    }
}




void Nivel_2::cargarSpriteObjetivo(const QPixmap& hoja,
                                   int srcX, int srcY,
                                   int srcW, int srcH)
{
    if (hoja.isNull()) return;

    QPixmap sprite = hoja.copy(srcX, srcY, srcW, srcH);
    if (sprite.isNull()) return;

    if (!itemObjetivo)
        itemObjetivo = new QGraphicsPixmapItem();

    itemObjetivo->setPixmap(sprite);

    // Centrar sobre objetivoX, objetivoY
    itemObjetivo->setPos(objetivoX - srcW * 0.5f,
                         objetivoY - srcH * 0.5f);
    itemObjetivo->setZValue(2.0);
    escena->addItem(itemObjetivo);
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

    if (!jugador) return;   // ← mover este guard al inicio

    bool oculto = jugadorEnSombra();

    // ── Tick robots: centro del jugador + flag de sombra ─────────────────
    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    for (int i = 0; i < (int)robots.size(); i++)
    {
        RobotSeguridad* robot = robots[i];
        robot->tick(jx, jy, dt, oculto);   // ← un solo tick con todo

        // Colisiones del robot con paredes
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
    };

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
    if (escena)
    {
        static QGraphicsRectItem* debugRect = nullptr;
        if (!debugRect) {
            debugRect = escena->addRect(0,0,1,1,
                                        QPen(QColor(255,0,0,200), 2));
            debugRect->setZValue(99);
        }
        Hitbox hb = jugador->getHitbox();
        debugRect->setRect(hb.x, hb.y, hb.w, hb.h);
    }

    // ── Resolver colisiones del jugador con las paredes ───────────────────
    resolverColisiones();


    // ── Actualizar círculos de detección en la escena ─────────────────────
    actualizarCirculosDeteccion();

    // ── Iframes: contar y hacer parpadear al personaje ────────────────────────
    if (tiempoInvulnerable > 0.f)
    {
        tiempoInvulnerable -= dt;

        // Parpadeo cada 0.1s — feedback visual de invulnerabilidad
        bool visible = (static_cast<int>(tiempoInvulnerable * 10.f) % 2 == 0);
        if (jugador->getItem()) jugador->getItem()->setVisible(visible);
    }
    else if (jugador->getItem() && !jugador->getItem()->isVisible())
    {
        jugador->getItem()->setVisible(true);  // asegurar visible al salir de iframes
    }

    // ── Actualizar HUD cada tick ──────────────────────────────────────────────
    actualizarHUD();

    // ── Comprobar condiciones de fin ──────────────────────────────────────
    verificarDeteccion();
    verificarVictoria(dt);
}




// ════════════════════════════════════════════════════════════════════════════
//  actualizarCirculosDeteccion  — mueve y recolorea los anillos de radar
//  cada tick, reflejando el estado actual de cada robot (PATRULLAJE / PERSECUCION)
// ════════════════════════════════════════════════════════════════════════════





// ── Detección: si un robot toca al jugador → daño + respawn ──────────────────
void Nivel_2::verificarDeteccion()
{
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
            // jugador->recibirDanio(1);
            // jugador->resetearPosicion(spawnX, spawnY);

            // // Si te atrapan mientras hackeabas, el progreso se pierde
            // tiempoHackeo  = 0.f;
            // haciendoHackeo = false;
            // sonidoHackeoLoop.stop();

            // break;
            // ── Recibir daño ──────────────────────────────────────────────
            vidasN2--;
            sonidoDanio.play();
            tiempoInvulnerable = DURACION_INVULNERABLE;  // activar iframes

            if (vidasN2 <= 0)
            {
                // Sin vidas → respawn y resetear todo
                vidasN2        = vidasN2Max;
                tiempoHackeo   = 0.f;
                haciendoHackeo = false;
                sonidoHackeoLoop.stop();
                jugador->resetearPosicion(spawnX, spawnY);
            }
            break;
        }
    }
}


bool Nivel_2::jugadorEnSombra() const
{
    // Usar la posición base sin offset — más predecible
    float px = jugador->getX();
    float py = jugador->getY();

    for (const auto& z : zonasOcultas)
        if (px >= z.x && px <= z.x + z.w &&
            py >= z.y && py <= z.y + z.h)
            return true;

    return false;
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

    if (Escenario && !Escenario->isNull())
    {
        // ── 1. SceneRect = dimensiones exactas de la imagen ──────────────
        // Así fitInView sabe exactamente qué área escalar.
        escena->setSceneRect(0, 0, Escenario->width(), Escenario->height());

        // ── 2. Fondo como item, NO como brush ────────────────────────────
        // setBackgroundBrush(pixmap) repite la imagen como mosaico.
        // Un QGraphicsPixmapItem en z=-1 se dibuja una sola vez en (0,0).
        QGraphicsPixmapItem* bg = new QGraphicsPixmapItem(*Escenario);
        bg->setZValue(-1);
        bg->setPos(0, 0);
        escena->addItem(bg);

        // ── 3. Color de las "barras" si la ventana tiene otra proporción ──
        // Cuando fitInView deja espacio vacío alrededor, se ve este color.
        escena->setBackgroundBrush(Qt::black);
    }
    else
    {
        // Fallback si la imagen no cargó
        escena->setSceneRect(0, 0, 1270, 650);
        escena->setBackgroundBrush(QColor(5, 10, 20));
    }

}




void Nivel_2::limpiarEscena()
{
    // Nullear punteros ANTES de que scena->clear() los destruya
    itemsParedes.clear();
    itemsZonas.clear();
    itemsDeteccion.clear();
    itemObjetivo = nullptr;

    // Nullear itemGrafico de cada plataforma — scena->clear() ya los destruyó
    for (Plataforma* plat : plataformas)
        plat->setItemNull();  // evita double-free en limpiarPlataformas

    // Nullear itemGrafico de cada robot
    for (RobotSeguridad* robot : robots)
        robot->setItemGrafico(nullptr);
}
