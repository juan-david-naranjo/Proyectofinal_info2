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
    , spawnY(100.f)
    , escena(nullptr)
    , tiempoContador(0.f)
    ,itemObjetivo(nullptr)
{
    tiempoRestante = 120;
    Escenario= new QPixmap(":/Kael_nivel2/Sprites/Nivel2/Escenario_V2 sin marca de agua.png");
    // ── Cargar sonidos ────────────────────────────────────────────────────
    // Ajusta las rutas según tus recursos (qrc o ruta local)
    sonidoDeteccion.setSource(QUrl("qrc:/sonidoswav/Sonidos/persecusionwav.wav"));
    sonidoDeteccion.setVolume(0.9f);
    sonidoDanio.setSource(QUrl("qrc:/sonidoswav/Sonidos/hurtwav.wav"));
    sonidoDanio.setVolume(1.0f);

    sonidoHackeoLoop.setSource(QUrl("qrc:/sonidoswav/Sonidos/Hacking.wav"));
    sonidoHackeoLoop.setLoopCount(QSoundEffect::Infinite);  // loop mientras hackeas
    sonidoHackeoLoop.setVolume(0.5f);

    sonidoVictoria.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_victoria.wav"));
    sonidoVictoria.setVolume(0.4f);
    musicaFondo.setAudioOutput(&audioFondo);
    musicaFondo.setSource(QUrl("qrc:/sonidoswav/Sonidos/End of Line (From TRON_ LegacyScore).mp3"));
    audioFondo.setVolume(0.35f);       // suave para no tapar los efectos
    musicaFondo.setLoops(QMediaPlayer::Infinite);

}


// ------------------- Sobrecargas Obligatorias --------------------------

Nivel_2::Nivel_2(const Nivel_2& otro)
    : Nivel(otro)
    , escena(nullptr)               // la copia no tiene escena Qt propia
    , objetivoX(otro.objetivoX)
    , objetivoY(otro.objetivoY)
    , objetivoRadio(otro.objetivoRadio)
    , spawnX(otro.spawnX)
    , spawnY(otro.spawnY)
    , vidasN2(otro.vidasN2)             //pendiente revisar
    , vidasN2Max(otro.vidasN2Max)         //pendiente
    , tiempoInvulnerable(otro.tiempoInvulnerable)
    , tiempoHackeo(otro.tiempoHackeo)
    , tiempoHackeoMax(otro.tiempoHackeoMax)
    , haciendoHackeo(otro.haciendoHackeo)
    , tiempoContador(otro.tiempoContador)
    , animandoDestruccion(otro.animandoDestruccion)
    , frameDestruccion(otro.frameDestruccion)
    , tiempoFrameDestr(otro.tiempoFrameDestr)
    , duracionFrameDestr(otro.duracionFrameDestr)
    , framesDestruccion(otro.framesDestruccion)       // QPixmap copy-on-write
    , zonasOcultas(otro.zonasOcultas)
    , estadosZonas(otro.estadosZonas)
    , jugadorCompletamenteOculto(otro.jugadorCompletamenteOculto)
    , zonaActivaIdx(otro.zonaActivaIdx)
    , framesZonaApertura(otro.framesZonaApertura)
    , frameZonaOcupada(otro.frameZonaOcupada)
    , sinVidas(otro.sinVidas)
    , estadosAnteriores(otro.estadosAnteriores)
    , Escenario(otro.Escenario)                       // QPixmap copy-on-write
    // Todos los punteros a ítems Qt quedan a nullptr
    , itemObjetivo(nullptr)
    , itemBarraFondo(nullptr)
    , itemBarraRelleno(nullptr)
    , itemHUDTimer(nullptr)
    , debugJugadorRect(nullptr)
{
    // Copia profunda de robots
    for (RobotSeguridad* r : otro.robots)
        robots.push_back(new RobotSeguridad(*r));
}

bool Nivel_2::operator==(const Nivel_2& otro) const
{
    // Iguales si el estado lógico del nivel coincide
    return Nivel::operator==(otro)
           && vidasN2 == otro.vidasN2
           && sinVidas == otro.sinVidas;
}


//------------------------------------------------------------------------


Nivel_2::~Nivel_2()
{
    musicaFondo.stop();
    sonidoHackeoLoop.stop();
    limpiarRobots();
}

void Nivel_2::limpiarRobots()
{
    for (RobotSeguridad* r : robots){
        if (r) r->setItemGrafico(nullptr);  // ← nulificar ANTES del delete
        delete r;
    }
    robots.clear();
}

void Nivel_2::setDifficult(int dificult)
{
    const int MAX_DIFICULT=90;
    const int MED_DIFICULT=140;
    const int EASY_DIFICULT=180;

    switch (dificult) {

        // 0 - facil
        // 1 - medio
        // 2 - dificil
    case 0:
        tiempoRestante = EASY_DIFICULT;
        vidasN2 = 3;
        break;
    case 1:
        tiempoRestante=MED_DIFICULT;
        vidasN2 = 3;
        break;
    case 2:
        tiempoRestante=MAX_DIFICULT;
        vidasN2 = 1;
        break;
    default:
        break;
    }



}




// ── Inicializar ──────────────────────────────────────────────────────────────
void Nivel_2::inicializar(Personaje* p)
{

    jugador = p;
    jugador->setHitboxOffset(20.f,15.f,50.f, 100.f);  // baja 15px, alto efectivo 90px
    loadDestAnim();

    generarLaberinto();
    generarRobots();

    // jugador->setVidas(vidasN2Max);      //cantidad de vidas en este nivel

    vidasN2 = vidasN2Max;           //modificable
    tiempoInvulnerable = 0.f;

    if (jugador)
        jugador->resetearPosicion(spawnX, spawnY);



    // ── Inicializar estados anteriores de los robots ──────────────────────
    // Necesario para detectar el CAMBIO de estado (no el estado en sí)
    estadosAnteriores.assign(robots.size(), EstadoAgente::PATRULLAJE);
    playMusic();

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
    estadosZonas.assign(zonasOcultas.size(), DatoEstadoZona{});
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
        200.f, 98.f,
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
        1253.f, 700.f,
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


void Nivel_2::generarRobots(int dificult)
{
    limpiarRobots();
    float RADIO_DETECCION;
    float RADIO_DESENGANCHE;
    float VELPATRULLA;
    float VELPERSECUSION;
    switch (dificult) {
    case 0: //easy
        RADIO_DETECCION   = 100.f;
        RADIO_DESENGANCHE = 110.f;
        VELPATRULLA = 80.f;
        VELPERSECUSION = 100.f;

        break;
    case 1: //medium

        RADIO_DETECCION   = 120.f;
        RADIO_DESENGANCHE = 160.f;
        VELPATRULLA = 80.f;
        VELPERSECUSION = 120.f;
        break;
    case 2: //hard
        RADIO_DETECCION   = 120.f;
        RADIO_DESENGANCHE = 160.f;
        VELPATRULLA = 100.f;
        VELPERSECUSION = 160.f;
        break;

    default:
        break;
    }

    // ── Robot 1: patrulla el sector izquierdo ─────────────────────────────
    std::vector<Punto2D> wp1 = {
        {220.f, 96.f}, {220.f, 440.f},{220.f,96.f},{500.f,96.f}
    };

    //{196.f, 96.f}, {956.f, 96.f},
    robots.push_back(new RobotSeguridad(
        200.f, 96.f,
        RADIO_DETECCION,  // radioDeteccion
        RADIO_DESENGANCHE,  // radioDesenganche
        VELPATRULLA,   // velPatrulla
        VELPERSECUSION,  // velPersecucion
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
        RADIO_DETECCION,
        RADIO_DESENGANCHE,
        VELPATRULLA,
        VELPERSECUSION,
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
        RADIO_DETECCION,
        RADIO_DESENGANCHE,
        VELPATRULLA,
        VELPERSECUSION,
        wp3
        ));
}





void Nivel_2::agregarItemsEscena()
{
    if (jugador && jugador->getItem())
    {
        jugador->getItem()->setZValue(4.0);
        escena->addItem(jugador->getItem());
    }
    addWallScene();
    addObjetivoScene();
    addRobotScene();
    addHudScene();
    addHideZone();
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

    auto quitarFondo = [](const QPixmap& src, QColor fondo, int tol) -> QPixmap {
        if (fondo.alpha() == 0) return src;  // sin color de fondo → no procesar
        QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
        int r = fondo.red(), g = fondo.green(), b = fondo.blue();
        for (int py = 0; py < img.height(); py++)
            for (int px = 0; px < img.width(); px++) {
                QColor p = img.pixelColor(px, py);
                if (std::abs(p.red()-r)<=tol && std::abs(p.green()-g)<=tol && std::abs(p.blue()-b)<=tol)
                    img.setPixelColor(px, py, Qt::transparent);
            }
        return QPixmap::fromImage(img);
    };


    if (!itemObjetivo)
        itemObjetivo = new QGraphicsPixmapItem();

    itemObjetivo->setPixmap(quitarFondo(sprite,QColor(255,0,255),8));

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

    if (!jugador) return;

    bool oculto = jugadorEnSombra();

    // // ── Tick robots: centro del jugador + flag de sombra ─────────────────

    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;


    std::vector<Hitbox> hbParedes;
    hbParedes.reserve(plataformas.size());
    for (const Plataforma* p : plataformas)
        hbParedes.push_back(p->getHitbox());

    // Luego en el bucle:
    int i = 0;
    for (RobotSeguridad* robot : robots)
    {
        // qDebug() << "Robot N: " << i;
        //qDebug() << "mostrando robot antes de tick: " << robot->getX() << " y: " << robot->getY();
        // 1. Ejecutar el ciclo de movimiento e IA del agente
        robot->tick(jx, jy, dt, oculto, hbParedes);

        // 2. Extraer los datos iniciales tras el movimiento
        float rx  = robot->getX();
        float ry  = robot->getY();
        float rvx = robot->getVx();
        float rvy = robot->getVy();
        bool dummy = false;

        //qDebug() << "mostrando robot despues de tick " << rx << " y: " << ry;
        //qDebug() << "mostrando robot antes de colisiones x: " << rx << " y: " << ry;
        // 3. ¡LA CLAVE! Creamos una Hitbox temporal para el Robot en este frame.
        // De esta manera, cada modificación se aplica sobre la caja contenedora
        // de forma consistente antes de evaluar la siguiente plataforma.
        Hitbox hbRobot(rx, ry, 32.f, 32.f);

        for (Plataforma* plat : plataformas)
        {
            if (!plat) continue;
            Hitbox hbPlat = plat->getHitbox();

            // Optimizador de distancia: Si la plataforma está lejísimos, no la calcules
            if (std::abs(hbPlat.x - rx) > 150.f || std::abs(hbPlat.y - ry) > 150.f)
                continue;

            //Usamos tu segunda función (la sobrecarga ergonómica de Hitbox)
            GestorFisicas::resolverColision(hbRobot, rvx, rvy, dummy, hbPlat);
        }
        //qDebug() << "mostrando robot despues de todas las colisiones x: " << rx << " y: " << ry;
        // 4. Una vez que salió del bucle y rebotó contra todo de forma estable,
        // extraemos la posición final real de la Hitbox.
        rx = hbRobot.x;
        ry = hbRobot.y;



        // 5. Guardar en el objeto real
        robot->setPosicion(rx, ry);
        robot->setVelocidad(rvx, rvy);

        i++;
    }
    // // ── 3. HITBOX ROBOTS: actualizar posición de los rects debug ─────────────
    // for (int i = 0; i < static_cast<int>(robots.size()) &&
    //                 i < static_cast<int>(debugRobotsRect.size()); i++)
    // {
    //     debugRobotsRect[i]->setRect(
    //         robots[i]->getX(),
    //         robots[i]->getY(),
    //         32.f, 32.f
    //         );
    // }

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
    // if (escena)
    // {
    //     if (!debugJugadorRect)
    //     {
    //         debugJugadorRect = escena->addRect(0, 0, 1, 1,
    //                                            QPen(QColor(255, 0, 0, 200), 2));
    //         debugJugadorRect->setZValue(99);
    //     }
    //     Hitbox hb = jugador->getHitbox();
    //     debugJugadorRect->setRect(hb.x, hb.y, hb.w, hb.h);
    //     debugJugadorRect->setVisible(!oculto);
    // }

    // ── Resolver colisiones del jugador con las paredes ───────────────────
    resolverColisiones();


    // ── Actualizar círculos de detección en la escena ─────────────────────
    actualizarCirculosDeteccion();

    // ── Actualizar HUD cada tick ──────────────────────────────────────────────
    actualizarHUD();

    // 1. Agregar llamada a actualizarZonasOcultas (antes de verificarDeteccion):
        actualizarZonasOcultas(dt);

    // 2. Corregir el parpadeo de iframes para que no interfiera con el ocultamiento:
    if (tiempoInvulnerable > 0.f)
    {
        tiempoInvulnerable -= dt;
        if (!jugadorCompletamenteOculto)   // ← solo parpadea si NO está oculto
        {
            bool visible = (static_cast<int>(tiempoInvulnerable * 10.f) % 2 == 0);
            if (jugador->getItem()) jugador->getItem()->setVisible(visible);
        }
    }
    else if (jugador->getItem() && !jugador->getItem()->isVisible()
             && !jugadorCompletamenteOculto)   // ← solo restaura si no está oculto
    {
        jugador->getItem()->setVisible(true);
    }





    // ── Comprobar condiciones de fin ──────────────────────────────────────
    verificarDeteccion();
    verificarVictoria(dt);
    if (animandoDestruccion)
        actualizarAnimDestruccion(dt);
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

            // ── Recibir daño ──────────────────────────────────────────────
            vidasN2--;
            sonidoDanio.play();
            tiempoInvulnerable = DURACION_INVULNERABLE;  // activar iframes

            if (vidasN2 <= 0)
            {
                // Sin vidas → respawn y resetear todo
                sinVidas = true;
                sonidoHackeoLoop.stop();
                musicaFondo.stop();
            }
            break;
        }
    }
}


bool Nivel_2::jugadorEnSombra() const
{
    // // Usar la posición base sin offset — más predecible
    // float px = jugador->getX();
    // float py = jugador->getY();

    // for (const auto& z : zonasOcultas)
    //     if (px >= z.x && px <= z.x + z.w &&
    //         py >= z.y && py <= z.y + z.h)
    //         return true;

    // return false;
     return jugadorCompletamenteOculto;   // solo oculto REAL, no solo estar en la zona
}



// ── Victoria: jugador llega a la computadora ──────────────────────────────────


void Nivel_2::verificarVictoria(float dt)
{
    if (!jugador) return;

    // Si ya está animando la destrucción, no re-entrar
    if (animandoDestruccion) return;

    float jx = jugador->getX() + jugador->getAncho() * 0.5f;
    float jy = jugador->getY() + jugador->getAlto()  * 0.5f;

    bool cercaDeComputadora = GestorFisicas::colisionCirculo(
        jx, jy, objetivoX, objetivoY, objetivoRadio);

    if (cercaDeComputadora)
    {
        if (!haciendoHackeo)
        {
            haciendoHackeo = true;
            sonidoHackeoLoop.play();
            // Mostrar barra
            if (itemBarraFondo)   itemBarraFondo->setVisible(true);
            if (itemBarraRelleno) itemBarraRelleno->setVisible(true);
        }

        tiempoHackeo += dt;

        // ── Actualizar barra ──────────────────────────────────────────────
        if (itemBarraFondo && itemBarraRelleno)
        {
            float progreso = std::min(tiempoHackeo / tiempoHackeoMax, 1.f);
            QRectF r = itemBarraFondo->rect();
            itemBarraRelleno->setRect(r.x(), r.y(),
                                      BARRA_ANCHO * progreso, BARRA_ALTO);

            // La barra se pone verde al completar
            if (progreso >= 1.f)
                itemBarraRelleno->setBrush(QBrush(QColor(40, 220, 80)));
        }

        // ── Hackeo completado → lanzar animación de destrucción ───────────
        if (tiempoHackeo >= tiempoHackeoMax)
        {
            musicaFondo.stop();
            sonidoHackeoLoop.stop();
            sonidoVictoria.play();

            if (!framesDestruccion.empty())
            {
                // Arrancar animación; completado se seteará al terminar
                animandoDestruccion = true;
                frameDestruccion    = 0;
                tiempoFrameDestr    = 0.f;
                if (itemObjetivo && !framesDestruccion.empty())
                    itemObjetivo->setPixmap(framesDestruccion[0]);
            }
            else
            {
                // Sin animación → pasar directo a victoria
                completado = true;
            }
        }
    }
    else
    {
        if (haciendoHackeo)
        {
            tiempoHackeo   = 0.f;
            haciendoHackeo = false;
            sonidoHackeoLoop.stop();

            // Ocultar y resetear barra
            if (itemBarraFondo)   itemBarraFondo->setVisible(false);
            if (itemBarraRelleno) itemBarraRelleno->setVisible(false);
            if (itemBarraRelleno)
            {
                QRectF r = itemBarraFondo->rect();
                itemBarraRelleno->setRect(r.x(), r.y(), 0.f, BARRA_ALTO);
                itemBarraRelleno->setBrush(QBrush(QColor(40, 140, 255)));
            }
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



    // Nullear itemGrafico de cada plataforma — scena->clear() ya los destruyó
    for (Plataforma* plat : plataformas){
        if(!plat) continue;
        plat->setItemNull();  // evita double-free en limpiarPlataformas
    }
    // Nullear itemGrafico de cada robot
    for (RobotSeguridad* robot : robots){
        if(!robot) continue;
        robot->setItemGrafico(nullptr);
    }
    itemsParedes.clear();
    itemsZonas.clear();
    itemsDeteccion.clear();
    itemObjetivo = nullptr;
    debugRobotsRect.clear();     // ← agregar
    itemsCorazones.clear();      // ← agregar
    itemHUDTimer       = nullptr; // ← agregar
    debugJugadorRect   = nullptr; // ← agregar (reemplaza el static)
    itemBarraFondo      = nullptr;
    itemBarraRelleno    = nullptr;
    animandoDestruccion = false;
    framesDestruccion.clear();

    itemsZonaSprites.clear();         // ← agregar
    estadosZonas.clear();             // ← agregar
    jugadorCompletamenteOculto = false; // ← agregar
    zonaActivaIdx = -1;               // ← agregar
    framesZonaApertura.clear();       // ← agregar (se recargan en agregarItemsEscena)
}



void Nivel_2::cargarSpritesDestruccion(const QPixmap& hoja,
                                       int ox, int oy,
                                       int fw, int fh,
                                       int numFrames, int sep,
                                       QColor fondoColor)
{
    framesDestruccion.clear();

    // Helper inline para quitar fondo
    auto quitarFondo = [](const QPixmap& src, QColor fondo, int tol) -> QPixmap {
        if (fondo.alpha() == 0) return src;  // sin color de fondo → no procesar
        QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
        int r = fondo.red(), g = fondo.green(), b = fondo.blue();
        for (int py = 0; py < img.height(); py++)
            for (int px = 0; px < img.width(); px++) {
                QColor p = img.pixelColor(px, py);
                if (std::abs(p.red()-r)<=tol && std::abs(p.green()-g)<=tol && std::abs(p.blue()-b)<=tol)
                    img.setPixelColor(px, py, Qt::transparent);
            }
        return QPixmap::fromImage(img);
    };

    for (int i = 0; i < numFrames; i++)
    {
        int x = ox + i * (fw + sep);
        if (x + fw > hoja.width() || oy + fh > hoja.height()) {
            QPixmap ph(fw, fh);
            ph.fill(Qt::transparent);
            framesDestruccion.push_back(ph);
            continue;
        }
        QPixmap frame = hoja.copy(x, oy, fw, fh);
        framesDestruccion.push_back(quitarFondo(frame, fondoColor, 8));
    }
}

void Nivel_2::actualizarAnimDestruccion(float dt)
{
    if (!animandoDestruccion || framesDestruccion.empty()) return;

    tiempoFrameDestr += dt;
    if (tiempoFrameDestr < duracionFrameDestr) return;

    tiempoFrameDestr = 0.f;
    frameDestruccion++;

    if (frameDestruccion >= static_cast<int>(framesDestruccion.size()))
    {
        // Animación terminada → victoria
        animandoDestruccion = false;
        if (itemObjetivo)      itemObjetivo->setVisible(false);
        if (itemBarraFondo)    itemBarraFondo->setVisible(false);
        if (itemBarraRelleno)  itemBarraRelleno->setVisible(false);
        completado = true;
        return;
    }

    if (itemObjetivo)
        itemObjetivo->setPixmap(framesDestruccion[frameDestruccion]);
}

void Nivel_2::cargarSpritesZonas(const QPixmap& hoja,
                                 int oxAnim,  int oyAnim,  int fwA, int fhA,
                                 int numAnim, int sepAnim,
                                 int oxE, int oyE, int fwE, int fhE,
                                 const std::vector<QColor>& fondos,int tolerancia)
{
    auto quitarFondos = [](const QPixmap& src,
                           const std::vector<QColor>& fondos,
                           int tol) -> QPixmap
    {
        if (fondos.empty()) return src;

        QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);

        for (int py = 0; py < img.height(); py++)
        {
            for (int px = 0; px < img.width(); px++)
            {
                QColor p = img.pixelColor(px, py);
                for (const QColor& f : fondos)
                {
                    if (std::abs(p.red()   - f.red())   <= tol &&
                        std::abs(p.green() - f.green()) <= tol &&
                        std::abs(p.blue()  - f.blue())  <= tol)
                    {
                        img.setPixelColor(px, py, Qt::transparent);
                        break;   // ya encontró un color que coincide, pasar al siguiente pixel
                    }
                }
            }
        }
        return QPixmap::fromImage(img);
    };

    // ── Frames de animación (apertura) ────────────────────────────────────
    framesZonaApertura.clear();
    for (int i = 0; i < numAnim; i++)
    {
        int y = oyAnim + i * (fhA + sepAnim);
        QPixmap frame;
        if (y + fhA <= hoja.height() && oyAnim + fhA <= hoja.height())
            frame = hoja.copy(oxAnim, y, fwA, fhA);
        else { frame = QPixmap(fwA, fhA); frame.fill(Qt::transparent); }
        framesZonaApertura.push_back(quitarFondos(frame, fondos,tolerancia));
    }

    // ── Frame estático "ocupado / post-salida" ────────────────────────────
    if (oxE + fwE <= hoja.width() && oyE + fhE <= hoja.height())
        frameZonaOcupada = quitarFondos(hoja.copy(oxE, oyE, fwE, fhE), fondos, tolerancia);
    else { frameZonaOcupada = QPixmap(fwE, fhE); frameZonaOcupada.fill(Qt::transparent); }

    // ── Aplicar frame inicial a cada zona ─────────────────────────────────
    for (int i = 0; i < (int)itemsZonaSprites.size(); i++)
    {
        if (!itemsZonaSprites[i]) continue;
        const auto& z = zonasOcultas[i];
        if (!framesZonaApertura.empty())
        {
            QPixmap p = framesZonaApertura[0].scaled(
                (int)z.w, (int)z.h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            itemsZonaSprites[i]->setPixmap(p);
        }
    }
}

void Nivel_2::stopMusic()
{
    musicaFondo.stop();
}

void Nivel_2::playMusic()
{
    musicaFondo.play();
}

void Nivel_2::addWallScene()
{
    QPixmap hojaMuros(":/Kael_nivel2/Sprites/Nivel2/murosV2.png");
    if (hojaMuros.isNull()) qDebug() << "WARN: hoja de muros no cargó";
    // if (!itemsParedes.empty()) return;
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
}

void Nivel_2::addRobotScene()
{
    // ── 3. Robots de seguridad ────────────────────────────────────────────────
    //
    //  Se carga la hoja de sprites UNA SOLA VEZ y se pasa a cada robot.
    //  Si los sprites del robot están en una hoja distinta a la del personaje,
    //  cambia la ruta en QPixmap robotSheet("...").
    //
    // ─────────────────────────────────────────────────────────────────────────
    QPixmap robotSheet(":/Kael_nivel2/Sprites/Nivel2/Robot_V2.png");
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
        spriteItem->setZValue(4.0);
        // Asignar el ítem al robot para que actuar() lo mueva y anime
        robot->setItemGrafico(spriteItem);
        escena->addItem(spriteItem);




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
    // ── Debug: hitbox visible de cada robot (naranja) ─────────────────────────
    // debugRobotsRect.clear();
    // for (int i = 0; i < static_cast<int>(robots.size()); i++)
    // {
    //     QGraphicsRectItem* dr = escena->addRect(
    //         0, 0, 32.f, 32.f,
    //         QPen(QColor(255, 140, 0, 220), 2),   // naranja
    //         QBrush(QColor(255, 140, 0, 30))       // relleno muy tenue
    //         );
    //     dr->setZValue(98);
    //     debugRobotsRect.push_back(dr);
    // }
}

void Nivel_2::addObjetivoScene()
{
    // ── 2. Objetivo: la computadora ───────────────────────────────────────────

    QPixmap hojaObjetivo(":/Kael_nivel2/Sprites/Nivel2/computer_.png");
    if (hojaObjetivo.isNull())
        qDebug() << "WARN: hoja de computadora no cargó";

    cargarSpriteObjetivo(hojaObjetivo, 0,1573, 127, 181);
    // ── Barra de progreso de hackeo ───────────────────────────────────────────
    // La posición es encima del sprite de la computadora.
    // cargarSpriteObjetivo usa srcH=140, así que el techo del sprite está en:
    // objetivoY - 140/2 = objetivoY - 70. La barra va 18px más arriba.
    float barraX = objetivoX - BARRA_ANCHO * 0.5f;
    float barraY = objetivoY - 70.f - BARRA_ALTO - 6.f;

    // Fondo oscuro
    itemBarraFondo = new QGraphicsRectItem(barraX, barraY, BARRA_ANCHO, BARRA_ALTO);
    itemBarraFondo->setBrush(QBrush(QColor(20, 20, 40, 210)));
    itemBarraFondo->setPen(QPen(QColor(80, 80, 180, 200), 1));
    itemBarraFondo->setZValue(6.0);
    itemBarraFondo->setVisible(false);   // oculta hasta que empiece el hackeo
    escena->addItem(itemBarraFondo);

    // Relleno azul
    itemBarraRelleno = new QGraphicsRectItem(barraX, barraY, 0.f, BARRA_ALTO);
    itemBarraRelleno->setBrush(QBrush(QColor(40, 140, 255)));
    itemBarraRelleno->setPen(Qt::NoPen);
    itemBarraRelleno->setZValue(6.1);
    itemBarraRelleno->setVisible(false);
    escena->addItem(itemBarraRelleno);
}

void Nivel_2::addHideZone()
{
    // ── Cargar hoja de sprites de zonas ──────────────────────────────────────
    QPixmap hojaZonas(":/Kael_nivel2/Sprites/Nivel2/Zonas_ocultas.png"); // ← tu ruta
    std::vector<QColor> fondosZona = {
        QColor("#223f45"),  // Fondo predominante
        QColor("#2d3f47"),  // Ruido de compresión 1
        QColor("#293e47")   // Ruido de compresión 2
    };
    if (!hojaZonas.isNull())
    {
        cargarSpritesZonas(hojaZonas,
                           //  animación apertura: ox, oy, fw, fh, numFrames, separación
                           602,   168,   112, 90, 5,  22,
                           //  frame estático (ocupado/salida): ox, oy, fw, fh
                           0, 800,   100, 100,
                           fondosZona,12);  // color de fondo a eliminar
    }
    else qDebug() << "WARN: hoja de zonas ocultas no cargó";

}


void Nivel_2::loadDestAnim(){
    // ── Cargar animación de destrucción ──────────────────────────────────────
    // Cambia la ruta y coordenadas por las de tu hoja de sprites
    QPixmap hojaDestruccion(":/Kael_nivel2/Sprites/Nivel2/computer_.png");
    if (!hojaDestruccion.isNull())
    {
        // Ajusta: ox, oy, fw, fh, numFrames, separacion, colorFondo
        cargarSpritesDestruccion(hojaDestruccion,
                                 0, 1573   ,     // ox, oy
                                 127, 181, // fw, fh (mismo tamaño que el sprite normal)
                                 13,        // número de frames
                                 73,        // separación entre frames
                                 QColor(255, 0, 255)); // color de fondo a eliminar
    }
    else
    {
        qDebug() << "WARN: hoja de destrucción no cargó";
    }
}

void Nivel_2::addHudScene()
{
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

    // ── Sprites de zonas ocultas ──────────────────────────────────────────────
    itemsZonaSprites.clear();
    for (const auto& z : zonasOcultas)
    {
        QGraphicsPixmapItem* item = new QGraphicsPixmapItem();
        item->setPos(z.x, z.y);
        item->setZValue(5.0);   // encima del jugador (z=4) para cubrirlo al ocultarse
        escena->addItem(item);
        itemsZonas.push_back(item);        // para cleanup en limpiarEscena
        itemsZonaSprites.push_back(item);
    }
}

void Nivel_2::actualizarZonasOcultas(float dt)
{
    if (!jugador || itemsZonaSprites.empty()) return;

    float jx = jugador->getX();
    float jy = jugador->getY();
    float velTotal = std::sqrt(jugador->getVx() * jugador->getVx() +
                               jugador->getVy() * jugador->getVy());
    bool estaQuieto = velTotal < UMBRAL_QUIETO;

    // ── Encontrar en qué zona está el jugador ─────────────────────────────
    int zonaActual = -1;
    for (int i = 0; i < (int)zonasOcultas.size(); i++) {
        const auto& z = zonasOcultas[i];
        if (jx >= z.x && jx <= z.x + z.w &&
            jy >= z.y && jy <= z.y + z.h)
        { zonaActual = i; break; }
    }

    for (int i = 0; i < (int)zonasOcultas.size(); i++)
    {
        auto& dato = estadosZonas[i];
        // ← NUNCA tocamos el pixmap del item de zona.
        //   Siempre muestra el frame 0 cargado al inicio.

        if (i == zonaActual)
        {
            switch (dato.estado)
            {
            case EstadoZona::LIBRE:
                if (estaQuieto) {
                    dato.estado     = EstadoZona::PROCESANDO;
                    dato.tiempoZona = 0.f;
                }
                break;

            case EstadoZona::PROCESANDO:
                if (!estaQuieto) {
                    dato.estado     = EstadoZona::LIBRE;
                    dato.tiempoZona = 0.f;
                } else {
                    dato.tiempoZona += dt;
                    if (dato.tiempoZona >= TIEMPO_PARA_OCULTARSE)
                    {
                        dato.estado            = EstadoZona::OCULTO;
                        jugadorCompletamenteOculto = true;
                        zonaActivaIdx          = i;

                        // Solo el jugador desaparece
                        // La zona NO cambia — sigue con su frame 0
                        if (jugador->getItem())
                            jugador->getItem()->setVisible(false);
                    }
                }
                break;

            case EstadoZona::OCULTO:
                jugadorCompletamenteOculto = true;
                if (!estaQuieto) {
                    // Salir del escondite
                    dato.estado            = EstadoZona::LIBRE;
                    jugadorCompletamenteOculto = false;
                    zonaActivaIdx          = -1;

                    // Solo el jugador reaparece
                    // La zona NO cambia — sigue con su frame 0
                    if (jugador->getItem())
                        jugador->getItem()->setVisible(true);
                } else {
                    if (jugador->getItem())
                        jugador->getItem()->setVisible(false);
                }
                break;
            }
        }
        else
        {
            if (dato.estado != EstadoZona::LIBRE)
            {
                dato.estado     = EstadoZona::LIBRE;
                dato.tiempoZona = 0.f;
                if (i == zonaActivaIdx) {
                    jugadorCompletamenteOculto = false;
                    zonaActivaIdx = -1;
                    if (jugador->getItem())
                        jugador->getItem()->setVisible(true);
                }
            }
        }
    }
}