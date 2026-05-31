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
    , escena(nullptr)
    , vista(nullptr)
    , fondoHUD(nullptr)
    , fondoBarraTiempo(nullptr)
    , barraTiempo(nullptr)
    , hudTiempo(nullptr)
    , hudVidas(nullptr)
    , hudPuerta(nullptr)
    , debugHitboxItem(nullptr)
{
    tiempoRestante = TIEMPO_NIVEL;
    Escenario = new QPixmap(":/Kael_nivel1/Sprites/Nivel1/Escenario1P.png");
}

Nivel_1::~Nivel_1()
{
    delete Escenario;
}

// ============================================================
//  setScene
// ============================================================
void Nivel_1::setScene(QGraphicsScene* scene, QGraphicsView* view)
{
    escena = scene;
    vista  = view;

    // Escena lógica: 800 × 1433
    escena->setSceneRect(0, 0, ESCENA_W, ESCENA_H);

    // Fondo escalado al tamaño lógico
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

    // ── Configurar la view ────────────────────────────────────
    vista->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Escalar para que los 800px lógicos quepan exactamente en
    // el ancho de la view, manteniendo esa escala para el alto.
    // Así la cámara mueve solo en Y con centerOn().
    aplicarEscalaView();
}

// ============================================================
//  aplicarEscalaView
//  Calcula la escala uniforme para que ESCENA_W llene el ancho
//  disponible de la view, sin distorsión.
// ============================================================
void Nivel_1::aplicarEscalaView()
{
    if (!vista) return;

    vista->resetTransform();

    int viewW = vista->viewport()->width();
    if (viewW <= 0) viewW = 800;

    // Escala: cuántos px de pantalla por px lógico
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
    jugador         = p;
    puertaCerrada   = false;
    completado      = false;
    tiempoRestante  = TIEMPO_NIVEL;
    timerAcumulado  = 0.f;
    tiempoAcumulado = 0.f;

    generarPlataformas();

    if (escena)
    {
        // Cargar sprite de plataforma (debe estar registrado en el .qrc)
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

    // ── Rect debug de hitbox ──────────────────────────────────
    debugHitboxItem = nullptr;
    if (DEBUG_HITBOX && escena)
    {
        debugHitboxItem = escena->addRect(0, 0, 1, 1,
            QPen(QColor(0, 255, 80), 1.5f, Qt::SolidLine),
            QBrush(QColor(0, 255, 80, 30)));
        debugHitboxItem->setZValue(50);   // encima de todo
    }

    // Cámara inicial en el spawn
    if (vista)
        vista->centerOn(ESCENA_W / 2.f, spawnY);
}

// ============================================================
//  generarPlataformas  — espacio lógico 800 × 1433
// ============================================================
void Nivel_1::generarPlataformas()
{
    limpiarPlataformas();

    // Suelo base
    //plataformas.push_back(new Plataforma(  0.f, 1390.f, 800.f, 30.f, false));

    // Puerta 1 — zona baja
    plataformas.push_back(new Plataforma( 13.f, 1414.f, 137.f, 18.f, false)); //spawn
    plataformas.push_back(new Plataforma(252.f, 1357.f,  68.f, 12.f, false));
    plataformas.push_back(new Plataforma(456.f, 1315.f, 116.f, 23.f, false));
    plataformas.push_back(new Plataforma(358.f, 1260.f,  49.f, 20.f, false));
    //plataformas.push_back(new Plataforma(359.f, 1343.f, 122.f, 73.f, false)); //ventilador
    plataformas.push_back(new Plataforma(430.f, 1198.f, 114.f, 27.f, false));
    plataformas.push_back(new Plataforma(265.f, 1058.f, 277.f, 26.f, false));

    //Puerta 2 - zona media
    plataformas.push_back(new Plataforma( 13.f,  955.f, 127.f, 22.f, false)); //Punto de control
    plataformas.push_back(new Plataforma(265.f,  955.f, 113.f, 19.f, false));
    plataformas.push_back(new Plataforma(219.f,  810.f, 107.f, 20.f, false));
    plataformas.push_back(new Plataforma( 13.f,  688.f, 118.f, 25.f, false));
    plataformas.push_back(new Plataforma(264.f,  701.f,  99.f, 28.f, false));
    plataformas.push_back(new Plataforma(413.f,  701.f, 131.f, 28.f, false));
    plataformas.push_back(new Plataforma(358.f,  585.f,  35.f, 18.f, false));
    plataformas.push_back(new Plataforma( 13.f,  546.f, 346.f, 15.f, false));
    plataformas.push_back(new Plataforma(483.f,  469.f, 303.f, 24.f, false));
    plataformas.push_back(new Plataforma( 13.f,  394.f, 127.f, 22.f, false)); //Puerta 3

    // META — plataforma dorada
    plataformas.push_back(new Plataforma(266.f,  297.f, 276.f, 18.f, false));
}

// ============================================================
//  crearItemsPlataformas
//  Usa spriteplataforma.png escalado al tamaño de cada plataforma.
//  Si el sprite no cargó, cae back a rectángulos de color.
// ============================================================
void Nivel_1::crearItemsPlataformas()
{
    limpiarItemsPlataformas();

    const bool usarSprite = !spritePlataforma.isNull();

    // Colores de fallback (cuando no hay sprite)
    const QColor colorNormal(0x1a, 0x8a, 0xff, 200);
    const QColor colorMovil (0x00, 0xff, 0xcc, 180);
    const QColor colorMeta  (0xff, 0xd7, 0x00, 255);

    for (int i = 0; i < static_cast<int>(plataformas.size()); ++i)
    {
        Plataforma* p = plataformas[i];
        const bool  esMeta = (i == static_cast<int>(plataformas.size()) - 1);

        if (usarSprite && !esMeta)
        {
            // Escalar el sprite al tamaño exacto de la plataforma
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
            // Plataforma meta: dorada; fallback sin sprite: color según tipo
            QColor color = esMeta ? colorMeta
                         : p->esMovil ? colorMovil : colorNormal;

            // Creamos un pixmap del color sólido para mantener el mismo tipo
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
//  crearHUD
// ============================================================
void Nivel_1::crearHUD()
{
    fondoHUD = escena->addRect(0, 0, ESCENA_W, 38,
                               QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 160)));
    fondoHUD->setZValue(20);

    fondoBarraTiempo = escena->addRect(6, 26, ESCENA_W - 12, 8,
                                       QPen(QColor(60, 80, 100)), QBrush(QColor(20, 30, 40)));
    fondoBarraTiempo->setZValue(21);

    barraTiempo = escena->addRect(6, 26, ESCENA_W - 12, 8,
                                  QPen(Qt::NoPen), QBrush(QColor(0, 220, 255)));
    barraTiempo->setZValue(22);

    QFont fHUD("Consolas", 11, QFont::Bold);

    hudTiempo = escena->addText(QString("TIEMPO: %1").arg(TIEMPO_NIVEL), fHUD);
    hudTiempo->setDefaultTextColor(QColor(0, 220, 255));
    hudTiempo->setZValue(23);

    hudVidas = escena->addText("VIDAS: 3", fHUD);
    hudVidas->setDefaultTextColor(QColor(255, 80, 80));
    hudVidas->setZValue(23);

    QFont fAlerta("Consolas", 16, QFont::Bold);
    hudPuerta = escena->addText("¡PUERTA CERRADA!", fAlerta);
    hudPuerta->setDefaultTextColor(QColor(255, 50, 50));
    hudPuerta->setZValue(30);
    hudPuerta->setVisible(false);
}

// ============================================================
//  actualizarHUD — ancla al borde superior visible de la cámara
// ============================================================
void Nivel_1::actualizarHUD()
{
    if (!barraTiempo || !hudTiempo || !hudVidas || !vista) return;

    QPointF topLeft = vista->mapToScene(0, 0);
    float camTop  = topLeft.y();
    float camLeft = topLeft.x();

    fondoHUD->setRect(camLeft, camTop, ESCENA_W, 38);
    fondoBarraTiempo->setRect(camLeft + 6, camTop + 26, ESCENA_W - 12, 8);

    float prop     = static_cast<float>(tiempoRestante) / TIEMPO_NIVEL;
    float anchoMax = ESCENA_W - 12.f;
    barraTiempo->setRect(camLeft + 6, camTop + 26, anchoMax * prop, 8);

    if (prop > 0.4f)       barraTiempo->setBrush(QBrush(QColor(0, 220, 255)));
    else if (prop > 0.15f) barraTiempo->setBrush(QBrush(QColor(255, 180, 0)));
    else                   barraTiempo->setBrush(QBrush(QColor(255, 60, 60)));

    hudTiempo->setPlainText(QString("TIEMPO: %1").arg(tiempoRestante));
    hudTiempo->setPos(camLeft + 6, camTop + 2);
    if (tiempoRestante > 30)      hudTiempo->setDefaultTextColor(QColor(0, 220, 255));
    else if (tiempoRestante > 10) hudTiempo->setDefaultTextColor(QColor(255, 200, 0));
    else                          hudTiempo->setDefaultTextColor(QColor(255, 50, 50));

    if (jugador)
        hudVidas->setPlainText(QString("VIDAS: %1").arg(jugador->getVidas()));
    hudVidas->setPos(camLeft + ESCENA_W - 130, camTop + 2);

    if (hudPuerta)
    {
        hudPuerta->setVisible(puertaCerrada);
        if (puertaCerrada)
        {
            QPointF center = vista->mapToScene(
                vista->viewport()->width()  / 2,
                vista->viewport()->height() / 2);
            hudPuerta->setPos(center.x() - 130, center.y() - 12);
        }
    }
}

// ============================================================
//  actualizarCamara
//  Mueve la cámara en Y siguiendo al jugador.
//  X siempre fija en el centro de la escena (400 px).
// ============================================================
void Nivel_1::actualizarCamara()
{
    if (!vista || !jugador) return;

    float camX = ESCENA_W / 2.f;
    float camY = jugador->getY() + CAM_OFFSET_Y;

    // Calcular cuántos px lógicos son visibles en Y
    QPointF topScene    = vista->mapToScene(0, 0);
    QPointF bottomScene = vista->mapToScene(0, vista->viewport()->height());
    float visibleH = bottomScene.y() - topScene.y();
    float halfVis  = visibleH / 2.f;

    // Clamping: evitar mostrar fuera de los bordes de la escena.
    // Primero clampeamos el límite inferior, luego el superior,
    // con std::max para que ambos no entren en conflicto.
    float camYMin = halfVis;
    float camYMax = ESCENA_H - halfVis;
    if (camYMax < camYMin) camYMax = camYMin;   // escena más pequeña que la vista

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

    // 4. Colisiones
    Nivel::resolverColisiones();

    // 5. Límite horizontal
    {
        float nx = jugador->getX();
        float vx = jugador->getVx();
        if (nx < 0.f)                                 { nx = 0.f;                          vx = 0.f; }
        else if (nx + jugador->getAncho() > ESCENA_W) { nx = ESCENA_W - jugador->getAncho(); vx = 0.f; }
        jugador->setPosicion(nx, jugador->getY());
        jugador->setVelocidad(vx, jugador->getVy());
    }

    // 6. Verificaciones
    verificarCaida();
    verificarVictoria();

    // 7. Debug hitbox
    if (DEBUG_HITBOX && debugHitboxItem && jugador)
    {
        Hitbox hb = jugador->getHitbox();
        debugHitboxItem->setRect(hb.x, hb.y, hb.w, hb.h);
    }

    // 8. Cámara y HUD
    actualizarCamara();
    actualizarHUD();
}

void Nivel_1::verificarCaida()
{
    if (!jugador) return;

    float jy = jugador->getY();

    // ── Caída de ≥4 plataformas → activar animación ──────────
    // Diferencia de altura entre la Y más alta y la actual
    // Cada plataforma ocupa ~120px de espacio vertical
    static constexpr float ALTURA_4_PLATAFORMAS = 120.f * 4;  // 480px

    float caida = jy - jugador->getYMasAlta();
    bool caioMucho = (caida > ALTURA_4_PLATAFORMAS);

    // Si cae al límite inferior del mapa
    if (jy > LIMITE_CAIDA)
    {
        if (caioMucho && !jugador->caidaFinalTerminada())
        {
            jugador->activarCaidaFinal();
            return;   // esperar animación
        }
        // Sin caída grande, o animación terminada → respawn
        jugador->recibirDanio(1);
        jugador->resetearPosicion(spawnX, spawnY);
    }
}

void Nivel_1::verificarVictoria()
{
    if (!jugador || puertaCerrada) return;
    // Solo se activa cuando el jugador aterriza sobre la plataforma meta,
    // no en el aire durante el salto
    if (jugador->isEnSuelo() && jugador->getY() <= META_Y)
        completado = true;
}
