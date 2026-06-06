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
    tiempoRestante = 0;
    Escenario= new QPixmap(":/Kael_nivel2/Sprites/Nivel2/Escenario_V2 sin marca de agua.png");
    // ── Cargar sonidos ────────────────────────────────────────────────────
    // Ajusta las rutas según tus recursos (qrc o ruta local)
    sonidoDeteccion.setSource(QUrl("qrc:/sonidoswav/Sonidos/persecusionwav.wav"));
    sonidoDeteccion.setVolume(0.9f);
    sonidoDanio.setSource(QUrl("qrc:/sonidoswav/Sonidos/hurtwav.wav"));
    sonidoDanio.setVolume(0.8f);

    sonidoHackeoLoop.setSource(QUrl("qrc:/sonidoswav/Sonidos/Hacking.wav"));
    sonidoHackeoLoop.setLoopCount(QSoundEffect::Infinite);  // loop mientras hackeas
    sonidoHackeoLoop.setVolume(0.2f);

    sonidoVictoria.setSource(QUrl("qrc:/sonidoswav/Sonidos/sonido_victoria.wav"));
    sonidoVictoria.setVolume(0.3f);
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
    , jugadorCompletamenteOculto(otro.jugadorCompletamenteOculto)
    , sinVidas(otro.sinVidas)
    , estadosAnteriores(otro.estadosAnteriores)
    , Escenario(otro.Escenario                        // deep copy del QPixmap del fondo
                ? new QPixmap(*otro.Escenario)
                : nullptr)
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

// ============================================================
//  Operador de asignación — Regla de los Tres
//  Libera robots y Escenario propios antes de copiar los del otro.
//  Los ítems Qt (escena, HUD) no se copian; la copia debe llamar
//  a setScene() + inicializar() para reconstruirlos.
// ============================================================
Nivel_2& Nivel_2::operator=(const Nivel_2& otro)
{
    if (this == &otro) return *this;

    // 1. Liberar recursos dinámicos propios
    musicaFondo.stop();
    sonidoHackeoLoop.stop();
    limpiarRobots();
    delete Escenario;

    // 2. Copiar parte base (plataformas incluidas)
    Nivel::operator=(otro);

    // 3. Deep copy del recurso dinámico propio
    Escenario           = otro.Escenario ? new QPixmap(*otro.Escenario) : nullptr;

    // 4. Copiar estado lógico
    objetivoX           = otro.objetivoX;
    objetivoY           = otro.objetivoY;
    objetivoRadio       = otro.objetivoRadio;
    spawnX              = otro.spawnX;
    spawnY              = otro.spawnY;
    tiempoInvulnerable  = otro.tiempoInvulnerable;
    tiempoHackeo        = otro.tiempoHackeo;
    tiempoHackeoMax     = otro.tiempoHackeoMax;
    haciendoHackeo      = otro.haciendoHackeo;
    tiempoContador      = otro.tiempoContador;
    animandoDestruccion = otro.animandoDestruccion;
    frameDestruccion    = otro.frameDestruccion;
    tiempoFrameDestr    = otro.tiempoFrameDestr;
    duracionFrameDestr  = otro.duracionFrameDestr;
    framesDestruccion   = otro.framesDestruccion;   // QPixmap copy-on-write
    jugadorCompletamenteOculto = otro.jugadorCompletamenteOculto;
    sinVidas            = otro.sinVidas;
    estadosAnteriores   = otro.estadosAnteriores;
    modoDificil         = otro.modoDificil;

    // 5. Deep copy de robots
    for (RobotSeguridad* r : otro.robots)
        robots.push_back(new RobotSeguridad(*r));

    // 6. Punteros Qt: la copia no hereda ítems de otra escena
    escena              = nullptr;
    itemObjetivo        = nullptr;
    itemBarraFondo      = nullptr;
    itemBarraRelleno    = nullptr;
    itemHUDTimer        = nullptr;
    debugJugadorRect    = nullptr;
    filtroOscuridad     = nullptr;

    return *this;
}

bool Nivel_2::operator==(const Nivel_2& otro) const
{
    // Iguales si el estado lógico del nivel coincide
    return Nivel::operator==(otro)
           && sinVidas == otro.sinVidas;
}


//------------------------------------------------------------------------


Nivel_2::~Nivel_2()
{
    musicaFondo.stop();
    sonidoHackeoLoop.stop();
    limpiarRobots();
    delete Escenario;   // liberar el QPixmap del fondo (new en constructor)
}

void Nivel_2::limpiarRobots()
{
    for (RobotSeguridad* r : robots){
        if (r) r->setItemGrafico(nullptr);  // ← nulificar ANTES del delete
        delete r;
    }
    robots.clear();
}





// ── Inicializar ──────────────────────────────────────────────────────────────
void Nivel_2::inicializar(Personaje* p)
{

    tiempoInvulnerable = 0.f;
    tiempoRestante     = 100;        // ← bug de timer resuelto de paso
    tiempoContador     = 0.f;
    haciendoHackeo     = false;
    tiempoHackeo       = 0.f;
    jugador = p;
    sinVidas           = false;
    completado         = false;
    jugador->setVidas(2);
    jugador->setHitboxOffset(20.f,15.f,50.f, 100.f);  // baja 15px, alto efectivo 90px
    loadDestAnim();         //animacion de destruccion de computadora

    generarLaberinto();
    generarRobots();


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
         {  771.f, 476.f,   20.f, 120.f, Plataforma::TipoMuro::VERTICAL   },
         {  451.f, 360.f,   20.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
         {  617.f, 217.f,   15.f, 100.f, Plataforma::TipoMuro::VERTICAL   },
         {  727.f, 217.f,   15.f, 110.f, Plataforma::TipoMuro::VERTICAL   },
         {  873.f, 217.f,   15.f, 250.f, Plataforma::TipoMuro::VERTICAL   },
         { 1034.f, 386.f,   15.f, 200.f, Plataforma::TipoMuro::VERTICAL   },
         };

    for (const auto& w : paredes)
        plataformas.push_back(
            new Plataforma(w.x, w.y, w.w, w.h, false, w.tipo));


}

// ── Generar robots ────────────────────────────────────────────────────────────


void Nivel_2::generarRobots()
{
    limpiarRobots();
    float factorScala=0.6;
    float RADIO_DETECCION;
    float RADIO_DESENGANCHE;
    float VELPATRULLA;
    float VELPERSECUSION;
    if(!modoDificil){
        RADIO_DETECCION   = 120.f;
        RADIO_DESENGANCHE = 110.f;
        VELPATRULLA = 80.f;
        VELPERSECUSION = 100.f;
    }else{
        RADIO_DETECCION   = 100.f;
        RADIO_DESENGANCHE = 90.f;
        VELPATRULLA = 100.f;
        VELPERSECUSION = 110.f;
    }

    // ── Robot 1: patrulla el sector izquierdo ─────────────────────────────
    std::vector<Punto2D> wp1 = {
        {220.f, 96.f}, {220.f, 440.f},{220.f,96.f},{1000.f,96.f}
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
        {700.f, 715.f}, {700.f, 512.f},{366.f,512.f},{366.f,412.f},{366.f,512.f},{700.f,512.f},
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
        RADIO_DETECCION*factorScala,
        RADIO_DESENGANCHE*factorScala,
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
    // ── SI ES MODO DIFÍCIL, CREAMOS EL FILTRO DE OSCURIDAD ──
    if (modoDificil)
    {
        // El rectángulo debe medir lo mismo que tu mapa (ej. 2000x2000 o lo que mida tu laberinto)
        filtroOscuridad = new QGraphicsRectItem(0, 0, 1320,870); // Ajusta al tamaño real de tu mapa si es más grande

        // Un ZValue alto asegura que tape el mapa y los robots, pero ojo: el HUD debe tener un ZValue aún mayor (ej. 200)
        filtroOscuridad->setZValue(10);

        filtroOscuridad->setPen(Qt::NoPen); // Quitamos las líneas de contorno
        escena->addItem(filtroOscuridad);
    }
    addWallScene();
    addObjetivoScene();
    addRobotScene();
    addHudScene();

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
    int vidasActuales=jugador->getVidas();

    // ── Corazones ─────────────────────────────────────────────────────────
    for (int i = 0; i < static_cast<int>(itemsCorazones.size()); i++)
    {
        if (i < vidasActuales)
            // Vida activa: rojo
            itemsCorazones[i]->setBrush(QBrush(QColor(220, 40, 40)));
        else
            // Vida perdida: gris oscuro
            itemsCorazones[i]->setBrush(QBrush(QColor(60, 60, 60)));
    }

    if (hudBarraBoost && jugador)
    {
        // Obtenemos el porcentaje actual (de 0.0 a 1.0)
        float progreso = jugador->getProgresoCooldownHabilidad();

        // Modificamos el rectángulo de la barra multiplicando el ancho máximo por el progreso
        // Mantenemos la misma posición X, Y y Alto, solo cambia el Ancho
        hudBarraBoost->setRect(hudBarraBoost->rect().x(),
                               hudBarraBoost->rect().y(),
                               ANCHO_MAX_BARRA * progreso,
                               hudBarraBoost->rect().height());

        // DETALLE PRO: Cambiar de color si ya está listo para usarse
        if (progreso >= 1.f) {
            hudBarraBoost->setBrush(QBrush(QColor(0, 255, 180))); // Verde neón (¡Listo!)
        } else {
            hudBarraBoost->setBrush(QBrush(QColor(0, 130, 230))); // Azul (Cargando...)
        }
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
    Hitbox p  = jugador->getHitbox();
    // float jw  = jugador->getAncho();
    // float jh  = jugador->getAlto();
    bool  dummy = false;

    for (Plataforma* plat : plataformas)
    {
        Hitbox hb = plat->getHitbox();
        GestorFisicas::resolverColision(
            jx, jy, p.w, p.h,
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

    bool oculto = jugador->isSigiloActivo();

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
    //verificar derrota
    if(this->getTiempoRestante()<=0){
        sinVidas=true;
        qDebug()<<"perdiste por tiempo";
    }

    // ── ACTUALIZAR LA LINTERNA / NIEBLA DE GUERRA ──
    if (modoDificil && filtroOscuridad && jugador)
    {
        // 1. Obtener el centro exacto del jugador
        float jx = jugador->getX() + jugador->getAncho() * 0.3f;
        float jy = jugador->getY() + jugador->getAlto()  * 0.3f;

        // 2. Definir el radio de visión (cuánto puede ver a su alrededor en píxeles)
        float radioVision = 180.f; // Puedes jugar con este número para tunear la dificultad

        // 3. Crear el gradiente radial centrado en el jugador
        QRadialGradient gradiente(jx, jy, radioVision);

        // En el centro exacto (0.0), es completamente transparente (Alfa = 0)
        gradiente.setColorAt(0.0, QColor(0, 0, 0, 0));

        // A partir del 60% del camino (0.6), empieza a difuminarse suavemente
        gradiente.setColorAt(0.6, QColor(5, 10, 20, 100));

        // Al llegar al límite del radio (1.0), es oscuridad absoluta (puedes usar el mismo color de tu fondo)
        gradiente.setColorAt(1.0, QColor(5, 10, 20, 255));

        // 4. Aplicar el gradiente al filtro
        filtroOscuridad->setBrush(QBrush(gradiente));
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


    // ── 1. GESTIÓN DE ALERTAS Y MÚSICA ────────────────────────────────────
    bool nuevaDeteccion = false;
    int robotsPersiguiendo = 0; // Para saber si AL MENOS UNO te persigue

    for (int i = 0; i < static_cast<int>(robots.size()); i++)
    {
        EstadoAgente estadoActual = robots[i]->getEstado();

        // Contamos si este robot está actualmente persiguiendo
        if (estadoActual == EstadoAgente::PERSECUCION) {
            robotsPersiguiendo++;
        }

        // Detectar si acaba de pasar de Patrullaje a Persecución
        if (estadosAnteriores[i] == EstadoAgente::PATRULLAJE &&
            estadoActual         == EstadoAgente::PERSECUCION)
        {
            nuevaDeteccion = true;
        }

        estadosAnteriores[i] = estadoActual;  // actualizar para el próximo tick
    }

    // ── Control del Audio basado en los contadores ──
    if (nuevaDeteccion) {
        sonidoDeteccion.play();   // ¡Un robot te vio! Suena la alerta
        musicaFondo.stop();       // Cortar música tranquila
    }
    // Si nadie te persigue, el juego no ha terminado (!sinVidas), y la música está detenida:
    else if (robotsPersiguiendo == 0 && !sinVidas &&
             musicaFondo.playbackState() != QMediaPlayer::PlayingState)
    {
        musicaFondo.play(); // ¡Reanudar la música de infiltración!
    }


    // ── 2. GESTIÓN DE DAÑO Y CAPTURA ──────────────────────────────────────
    for (RobotSeguridad* robot : robots)
    {
        if (!robot->atrapoJugador()) continue;

        robot->resetCaptura();   // siempre consumir la señal

        if (!jugador) return;
        if (tiempoInvulnerable > 0.f) continue;  // iframes activos → ignorar

        // ¡Magia del Espectro!
        if (jugador->isSigiloActivo()) {
            // Ignora al jugador como si no lo hubiera tocado
            continue;
        }

        // ── Recibir daño (Limpié las líneas duplicadas que tenías) ──
        jugador->recibirDanio(1);
        sonidoDanio.play();
        tiempoInvulnerable = DURACION_INVULNERABLE;

        // Comprobar Game Over
        if (jugador->getVidas() <= 0)
        {
            qDebug() << "Se acabo el juego";
            sinVidas = true;
            sonidoHackeoLoop.stop();
            musicaFondo.stop(); // Apagamos la música definitivamente
            break;
        }
    }
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
    // 1. Destruir los objetos C++ de las plataformas y vaciar el vector
    for (Plataforma* plat : plataformas){
        if (plat != nullptr) {
            delete plat; // ¡Esto libera la memoria de la clase Plataforma!
        }
    }
    plataformas.clear(); // Vacía el vector para que mida 0

    // 2. Destruir los objetos C++ de los robots y vaciar el vector
    for (RobotSeguridad* robot : robots){
        if (robot != nullptr) {
            delete robot; // ¡Esto libera la memoria del objeto Robot!
        }
    }
    robots.clear(); // Vacía el vector
    jugadorCompletamenteOculto = false;
    estadosAnteriores.clear(); // ¡CRÍTICO PARA EVITAR EL CRASH DE DETECCIÓN!
    itemsParedes.clear();
    itemsDeteccion.clear();
    itemObjetivo = nullptr;
    //debugRobotsRect.clear();     // ← agregar
    itemsCorazones.clear();      // ← agregar
    itemHUDTimer       = nullptr; // ← agregar
    debugJugadorRect   = nullptr; // ← agregar (reemplaza el static)
    itemBarraFondo      = nullptr;
    itemBarraRelleno    = nullptr;
    animandoDestruccion = false;
    framesDestruccion.clear();
    hudBarraBoost =nullptr;
    hudFondoBoost =nullptr;
    jugadorCompletamenteOculto = false; // ← agregar
    sonidoHackeoLoop.stop(); // Corta el bucle del hackeo si se quedó sonando
    sonidoDeteccion.stop();  // Corta el sonido de alerta de los robots
    sonidoDanio.stop();       // Corta el sonido de recibir daño
    sonidoVictoria.stop();   // Corta el sonido de victoria
    musicaFondo.stop();      // Asegura que la música de fondo se detenga
    filtroOscuridad = nullptr; // El puntero se invalida de forma segura
}

void Nivel_2::setDificultad(bool dificil)
{
    modoDificil    = dificil;
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

            plat->cargarSprite(hojaMuros, 701, 127, 231, 108);

            if (plat->getItem()) {
                escena->addItem(plat->getItem());
                itemsParedes.push_back(plat->getItem());
            }
        }
        else if (plat->tipoMuro == Plataforma::TipoMuro::VERTICAL)
        {

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
    itemHUDTimer->setZValue(200.0);
    escena->addItem(itemHUDTimer);

    // ── HUD: Corazones arriba a la izquierda ──────────────────────────────────
    itemsCorazones.clear();

    int vidasPorNivel=jugador->getVidas();
    for (int i = 0; i < vidasPorNivel; i++)
    {
        QGraphicsEllipseItem* corazon = new QGraphicsEllipseItem(0, 0, 22, 22);
        corazon->setBrush(QBrush(QColor(220, 40, 40)));    // rojo = vida activa
        corazon->setPen(QPen(QColor(255, 100, 100), 1));
        corazon->setPos(14.f + i * 30.f, 14.f);
        corazon->setZValue(200.0);
        escena->addItem(corazon);
        itemsCorazones.push_back(corazon);
    }

    float posX = 1094.f;  // Ajusta la posición en tu pantalla
    float posY = 750.f;  // Abajo del timer/corazones

    // 1. Rectángulo de Fondo (Gris estático)
    hudFondoBoost = new QGraphicsRectItem(posX, posY, ANCHO_MAX_BARRA, ALTO_MAX_BARRA);
    hudFondoBoost->setBrush(QBrush(QColor(40, 45, 50)));
    hudFondoBoost->setPen(Qt::NoPen);
    hudFondoBoost->setZValue(200.0); // Asegurar que esté al frente
    escena->addItem(hudFondoBoost);

    // 2. Rectángulo de Frente (Se va a estirar)
    hudBarraBoost = new QGraphicsRectItem(posX, posY, 0.f, ALTO_MAX_BARRA); // Empieza en ancho 0
    hudBarraBoost->setBrush(QBrush(QColor(0, 150, 255))); // Azul Boost
    hudBarraBoost->setPen(Qt::NoPen);
    hudBarraBoost->setZValue(201.0);
    escena->addItem(hudBarraBoost);



}

