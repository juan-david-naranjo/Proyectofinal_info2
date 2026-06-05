#include "nivel_1.h"
#include "gestorfisicas.h"
#include <QFont>
#include <QBrush>
#include <QPen>
#include <QColor>

// ============================================================
//  Constructor / Destructor
// ============================================================
Nivel_1::Nivel_1()
    : Nivel()
    , Escenario(nullptr)
    , puertaCerrada(false)
    , spawnX(2.f)
    , spawnY(1271.f)
    , timerAcumulado(0.f)
    , modoDificil(false)
    , vientoAmplitud(GestorFisicas::VIENTO_AMPLITUD)
    , escena(nullptr)
    , vista(nullptr)
    , fondoHUD(nullptr)
    , hudTiempo(nullptr)
    , hudDificultad(nullptr)
    , hudPuerta(nullptr)
    , debugHitboxItem(nullptr)
    , saltandoAnterior(false)
{
    tiempoRestante = TIEMPO_NIVEL;
    Escenario = new QPixmap(":/Kael_nivel1/Sprites/Nivel1/Escenario1P.png");

    musicaFondo.setAudioOutput(&audioFondo);
    musicaFondo.setSource(QUrl("qrc:/sonidoswav/Sonidos/End of Line (From TRON_ LegacyScore).mp3"));
    audioFondo.setVolume(0.2f);
    musicaFondo.setLoops(QMediaPlayer::Infinite);

    sonidoSalto.setSource(QUrl("qrc:/Sonidos/Sonidos/sonido_salto_V2.mp3"));
    sonidoSalto.setVolume(0.7f);
}

Nivel_1::~Nivel_1()
{
    limpiarEscena();   // limpia ítems Qt antes de liberar el objeto
    delete Escenario;
}

// ============================================================
//  setDificultad
// ============================================================
void Nivel_1::setDificultad(bool dificil)
{
    modoDificil    = dificil;
    vientoAmplitud = dificil ? VIENTO_AMPLITUD_DIFICIL
                             : GestorFisicas::VIENTO_AMPLITUD;
}

// ============================================================
//  Sonidos
// ============================================================
void Nivel_1::stopMusic() { musicaFondo.stop(); }
void Nivel_1::playMusic() { musicaFondo.play(); }

// ============================================================
//  setScene
// ============================================================
void Nivel_1::setScene(QGraphicsScene* scene, QGraphicsView* view)
{
    escena = scene;
    vista  = view;

    escena->setSceneRect(0, 0, ESCENA_W, ESCENA_H);

    if (Escenario && !Escenario->isNull())
    {
        QPixmap fondo = Escenario->scaled(
            static_cast<int>(ESCENA_W),
            static_cast<int>(ESCENA_H),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
        escena->setBackgroundBrush(fondo);
    }
    else
    {
        escena->setBackgroundBrush(QColor(10, 15, 30));
    }

    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    aplicarEscalaView();
}

// ============================================================
//  aplicarEscalaView
// ============================================================
void Nivel_1::aplicarEscalaView()
{
    if (!vista) return;

    vista->resetTransform();

    int viewW = vista->viewport()->width();
    if (viewW <= 0) viewW = 800;

    float escala = static_cast<float>(viewW) / ESCENA_W;
    vista->scale(escala, escala);
}

// ============================================================
//  restaurarView
// ============================================================
void Nivel_1::restaurarView()
{
    if (!vista) return;
    vista->resetTransform();
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// ============================================================
//  inicializar
// ============================================================
void Nivel_1::inicializar(Personaje* p)
{
    jugador          = p;
    puertaCerrada    = false;
    completado       = false;
    tiempoRestante   = TIEMPO_NIVEL;
    timerAcumulado   = 0.f;
    tiempoAcumulado  = 0.f;
    saltandoAnterior = false;

    jugador->setHitboxOffset(10.f,2.f,40.f,60.f);


    generarPlataformas();

    if (escena)
    {
        spritePlataforma = QPixmap(":/Kael_nivel1/Sprites/Nivel1/spriteplataforma.png");
        if (spritePlataforma.isNull())
            qDebug() << "WARN: no se cargó spriteplataforma.png, usando color sólido";

        crearItemsPlataformas();
        crearHUD();
    }

    if (jugador)
    {
        jugador->resetearPosicion(spawnX, spawnY);
        if (escena && jugador->getItem() && !jugador->getItem()->scene())
            escena->addItem(jugador->getItem());
        jugador->getItem()->setZValue(10);
    }

    debugHitboxItem = nullptr;
    if (DEBUG_HITBOX && escena)
    {
        debugHitboxItem = escena->addRect(0, 0, 1, 1,
                                          QPen(QColor(0, 255, 80), 1.5f, Qt::SolidLine),
                                          QBrush(QColor(0, 255, 80, 30)));
        debugHitboxItem->setZValue(50);
    }

    if (vista)
        vista->centerOn(ESCENA_W / 2.f, spawnY);

    playMusic();
}

// ============================================================
//  generarPlataformas
// ============================================================
void Nivel_1::generarPlataformas()
{
    limpiarPlataformas();

    plataformas.push_back(new Plataforma( 13.f, 1414.f, 137.f, 18.f, false)); //spawn
    plataformas.push_back(new Plataforma(252.f, 1357.f,  68.f, 12.f, false));
    plataformas.push_back(new Plataforma(456.f, 1315.f, 116.f, 23.f, false));
    plataformas.push_back(new Plataforma(358.f, 1260.f,  49.f, 20.f, false));
    plataformas.push_back(new Plataforma(430.f, 1198.f, 114.f, 27.f, false));
    plataformas.push_back(new Plataforma(265.f, 1058.f, 277.f, 26.f, false));

    plataformas.push_back(new Plataforma( 13.f,  955.f, 127.f, 22.f, false));
    plataformas.push_back(new Plataforma(265.f,  955.f, 113.f, 19.f, false));
    plataformas.push_back(new Plataforma(219.f,  810.f, 107.f, 20.f, false));
    plataformas.push_back(new Plataforma( 13.f,  688.f, 118.f, 25.f, false));
    plataformas.push_back(new Plataforma(264.f,  701.f,  99.f, 28.f, false));
    plataformas.push_back(new Plataforma(413.f,  701.f, 131.f, 28.f, false));
    plataformas.push_back(new Plataforma(358.f,  585.f,  35.f, 18.f, false));
    plataformas.push_back(new Plataforma( 13.f,  546.f, 346.f, 15.f, false));
    plataformas.push_back(new Plataforma(483.f,  469.f, 303.f, 24.f, false));
    plataformas.push_back(new Plataforma( 13.f,  394.f, 127.f, 22.f, false));

    // META — plataforma dorada
    plataformas.push_back(new Plataforma(266.f,  297.f, 276.f, 18.f, false));
}

// ============================================================
//  crearItemsPlataformas
// ============================================================
void Nivel_1::crearItemsPlataformas()
{
    limpiarItemsPlataformas();

    const bool usarSprite = !spritePlataforma.isNull();

    const QColor colorNormal(0x1a, 0x8a, 0xff, 200);
    const QColor colorMovil (0x00, 0xff, 0xcc, 180);
    const QColor colorMeta  (0xff, 0xd7, 0x00, 255);

    for (int i = 0; i < static_cast<int>(plataformas.size()); ++i)
    {
        Plataforma* p = plataformas[i];
        const bool  esMeta = (i == static_cast<int>(plataformas.size()) - 1);

        if (usarSprite && !esMeta)
        {
            QPixmap tile = spritePlataforma.scaled(
                static_cast<int>(p->ancho),
                static_cast<int>(p->alto),
                Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation);

            QGraphicsPixmapItem* item = escena->addPixmap(tile);
            item->setPos(p->getX(), p->getY());
            item->setZValue(5);
            itemsPlataformas.push_back(item);
        }
        else
        {
            QColor color = esMeta ? colorMeta
                           : p->esMovil ? colorMovil : colorNormal;

            QPixmap pxFallback(static_cast<int>(p->ancho),
                               static_cast<int>(p->alto));
            pxFallback.fill(color);

            QGraphicsPixmapItem* item = escena->addPixmap(pxFallback);
            item->setPos(p->getX(), p->getY());
            item->setZValue(5);
            itemsPlataformas.push_back(item);
        }
    }
}

void Nivel_1::limpiarItemsPlataformas()
{
    itemsPlataformas.clear();
}

// ============================================================
//  crearHUD — timer M:SS centrado arriba, sin fondo, estilo N2
// ============================================================
void Nivel_1::crearHUD()
{
    // ── Timer centrado — formato M:SS igual que N2 ─────────────
    // Sin banda de fondo: el texto flota limpio sobre el escenario
    QFont fTimer("Consolas", 22, QFont::Bold);
    hudTiempo = escena->addText("1:30", fTimer);
    hudTiempo->setDefaultTextColor(Qt::white);
    hudTiempo->setZValue(22);
    // posición inicial; se recalcula cada tick en actualizarHUD()

    // ── Indicador de dificultad (derecha) ─────────────────────
    QFont fDif("Consolas", 11, QFont::Bold);
    hudDificultad = escena->addText(modoDificil ? "DIFÍCIL" : "FÁCIL", fDif);
    hudDificultad->setDefaultTextColor(modoDificil ? QColor(255, 160, 0)
                                                   : QColor(0, 200, 100));
    hudDificultad->setZValue(22);

    // fondoHUD y hudPuerta quedan a nullptr — no se usan
    fondoHUD  = nullptr;
    hudPuerta = nullptr;
}

// ============================================================
//  actualizarHUD — ancla al borde superior visible de la cámara
//  Sin fondo: timer y dificultad flotan sobre el escenario.
//  Padding de 12 px desde el borde superior (igual que N2).
// ============================================================
void Nivel_1::actualizarHUD()
{
    if (!hudTiempo || !vista) return;

    // Esquina superior-izquierda del viewport en coordenadas de escena
    QPointF topLeft = vista->mapToScene(0, 0);
    float camTop  = topLeft.y();
    float camLeft = topLeft.x();

    // ── Timer M:SS centrado ────────────────────────────────────
    int mins = tiempoRestante / 60;
    int secs = tiempoRestante % 60;
    QString txt = QString("%1:%2")
                      .arg(mins)
                      .arg(secs, 2, 10, QChar('0'));
    hudTiempo->setPlainText(txt);

    // Color: blanco normal → amarillo bajo 30 s → rojo parpadeante bajo 10 s
    if (tiempoRestante > 30)
    {
        hudTiempo->setDefaultTextColor(Qt::white);
    }
    else if (tiempoRestante > 10)
    {
        hudTiempo->setDefaultTextColor(QColor(255, 200, 0));
    }
    else
    {
        bool parpadea = (static_cast<int>(timerAcumulado * 4.f) % 2 == 0);
        hudTiempo->setDefaultTextColor(parpadea ? QColor(255, 60, 60)
                                                : QColor(255, 160, 160));
    }

    // Centrado horizontal, 12 px desde el borde superior (igual que N2)
    float timerW = hudTiempo->boundingRect().width();
    hudTiempo->setPos(camLeft + ESCENA_W * 0.5f - timerW * 0.5f,
                      camTop  + 12.f);

    // ── Indicador de dificultad (derecha) ──────────────────────
    if (hudDificultad)
    {
        float difW = hudDificultad->boundingRect().width();
        hudDificultad->setPos(camLeft + ESCENA_W - difW - 8.f,
                              camTop  + 14.f);
    }

}

// ============================================================
//  actualizarCamara
// ============================================================
void Nivel_1::actualizarCamara()
{
    if (!vista || !jugador) return;

    float camX = ESCENA_W / 2.f;
    float camY = jugador->getY() + CAM_OFFSET_Y;

    QPointF topScene    = vista->mapToScene(0, 0);
    QPointF bottomScene = vista->mapToScene(0, vista->viewport()->height());
    float visibleH = bottomScene.y() - topScene.y();
    float halfVis  = visibleH / 2.f;

    float camYMin = halfVis;
    float camYMax = ESCENA_H - halfVis;
    if (camYMax < camYMin) camYMax = camYMin;

    if (camY < camYMin) camY = camYMin;
    if (camY > camYMax) camY = camYMax;

    vista->centerOn(camX, camY);
}

// ============================================================
//  actualizar — game loop
// ============================================================
void Nivel_1::actualizar(float dt)
{
    if (!jugador) return;

    // 1. Temporizador
    timerAcumulado  += dt;
    tiempoAcumulado += dt;
    if (timerAcumulado >= 1.f)
    {
        int seg = static_cast<int>(timerAcumulado);
        tiempoRestante -= seg;
        timerAcumulado -= static_cast<float>(seg);
        if (tiempoRestante <= 0) { tiempoRestante = 0; puertaCerrada = true; }
    }

    // 2. Plataformas móviles
    for (int i = 0; i < static_cast<int>(plataformas.size()); ++i)
    {
        plataformas[i]->actualizar(dt);
        if (i < static_cast<int>(itemsPlataformas.size()) && itemsPlataformas[i])
            itemsPlataformas[i]->setPos(
                plataformas[i]->getX(), plataformas[i]->getY());
    }

    // 3. Física
    jugador->actualizarNivel1(dt, tiempoAcumulado);

    // ── Viento extra en modo difícil ──────────────────────────
    if (modoDificil)
    {
        float fuerzaExtra =
            (VIENTO_AMPLITUD_DIFICIL - GestorFisicas::VIENTO_AMPLITUD)
            * std::sin(2.f * M_PI * GestorFisicas::VIENTO_FRECUENCIA * tiempoAcumulado);
        float vx = jugador->getVx() + fuerzaExtra * dt;
        jugador->setVelocidad(vx, jugador->getVy());
    }

    // 4. Colisiones
    Nivel::resolverColisiones();

    // 5. Límite horizontal
    {
        float nx = jugador->getX();
        float vx = jugador->getVx();
        if (nx < 0.f)                                 { nx = 0.f;                            vx = 0.f; }
        else if (nx + jugador->getAncho() > ESCENA_W) { nx = ESCENA_W - jugador->getAncho(); vx = 0.f; }
        jugador->setPosicion(nx, jugador->getY());
        jugador->setVelocidad(vx, jugador->getVy());
    }

    // 6. Sonido de salto
    {
        bool saltandoAhora = !jugador->isEnSuelo();
        if (saltandoAhora && !saltandoAnterior)
            sonidoSalto.play();
        saltandoAnterior = saltandoAhora;
    }

    // 7. Verificaciones
    verificarCaida();
    verificarVictoria();

    // 8. Debug hitbox
    if (DEBUG_HITBOX && debugHitboxItem && jugador)
    {
        Hitbox hb = jugador->getHitbox();
        debugHitboxItem->setRect(hb.x, hb.y, hb.w, hb.h);
    }

    // 9. Cámara y HUD
    actualizarCamara();
    actualizarHUD();
}

// ============================================================
//  verificarCaida
// ============================================================
void Nivel_1::verificarCaida()
{
    if (!jugador) return;

    float jy = jugador->getY();

    static constexpr float ALTURA_4_PLATAFORMAS = 120.f * 4;

    float caida    = jy - jugador->getYMasAlta();
    bool caioMucho = (caida > ALTURA_4_PLATAFORMAS);

    if (jy > LIMITE_CAIDA)
    {
        if (caioMucho && !jugador->caidaFinalTerminada())
        {
            jugador->activarCaidaFinal();
            return;
        }
        jugador->recibirDanio(1);
        jugador->resetearPosicion(spawnX, spawnY);
    }
}

// ============================================================
//  verificarVictoria
// ============================================================
void Nivel_1::verificarVictoria()
{
    if (!jugador || puertaCerrada) return;
    if (jugador->isEnSuelo() && jugador->getY() <= META_Y)
        completado = true;
}

void Nivel_1::limpiarEscena()
{
    limpiarItemsPlataformas();

    fondoHUD      = nullptr;
    hudTiempo     = nullptr;
    hudDificultad = nullptr;
    hudPuerta     = nullptr;
    debugHitboxItem = nullptr;

    musicaFondo.stop();
}
