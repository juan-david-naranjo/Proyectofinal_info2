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
    , spawnX(37.f)
    , spawnY(1341.f)
    , timerAcumulado(0.f)
    , escena(nullptr)
    , vista(nullptr)
    , fondoHUD(nullptr)
    , fondoBarraTiempo(nullptr)
    , barraTiempo(nullptr)
    , hudTiempo(nullptr)
    , hudVidas(nullptr)
    , hudPuerta(nullptr)
{
    tiempoRestante = TIEMPO_NIVEL;
    Escenario = new QPixmap(":/Kael_nivel1/Sprites/Nivel1/Escenario1P.png");
}

// ---------------- Sobrecarga Obligatoria -------------------
Nivel_1::Nivel_1(const Nivel_1& otro)
    : Nivel(otro)                       // copia plataformas, tiempoRestante, etc.
    , spawnX(otro.spawnX)
    , spawnY(otro.spawnY)
    , timerAcumulado(otro.timerAcumulado)
    , puertaCerrada(otro.puertaCerrada)
    , Escenario(otro.Escenario)          // QPixmap: copy-on-write, es seguro
    // Ítems Qt (escena, vista, HUD) quedan a nullptr — la copia es lógica
    , escena(nullptr)
    , vista(nullptr)
    , fondoHUD(nullptr)
    , fondoBarraTiempo(nullptr)
    , barraTiempo(nullptr)
    , hudTiempo(nullptr)
    , hudVidas(nullptr)
    , hudPuerta(nullptr)
{}

bool Nivel_1::operator==(const Nivel_1& otro) const
{
    // Iguales si el estado de juego lógico coincide
    return Nivel::operator==(otro) && puertaCerrada == otro.puertaCerrada;
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
    plataformas.push_back(new Plataforma(  13.f, 1414.f, 137.f, 18.f, false));

    // Fila 1 — zona baja
    plataformas.push_back(new Plataforma( 252.f, 1357.f, 68.f, 12.f, false));
    plataformas.push_back(new Plataforma(456.f, 1315.f, 116.f, 23.f, false));
    plataformas.push_back(new Plataforma(358.f, 1260.f, 49.f, 20.f, false));

    // Ventilador
    plataformas.push_back(new Plataforma(359.f, 1343.f, 122.f, 73.f, false));

    // Fila 2
    //plataformas.push_back(new Plataforma(150.f, 1170.f, 130.f, 18.f, true ));
    //plataformas.push_back(new Plataforma(450.f, 1150.f, 120.f, 18.f, false));
    //plataformas.push_back(new Plataforma( 20.f, 1130.f, 110.f, 18.f, false));

    // Fila 3
    plataformas.push_back(new Plataforma(290.f, 1050.f, 140.f, 18.f, false));
    plataformas.push_back(new Plataforma(560.f, 1020.f, 120.f, 18.f, true ));
    plataformas.push_back(new Plataforma( 70.f,  990.f, 100.f, 18.f, false));

    // Fila 4 — zona media
    plataformas.push_back(new Plataforma(190.f,  880.f, 130.f, 18.f, true ));
    plataformas.push_back(new Plataforma(480.f,  860.f, 110.f, 18.f, false));
    plataformas.push_back(new Plataforma( 30.f,  830.f, 120.f, 18.f, false));

    // Fila 5
    plataformas.push_back(new Plataforma(320.f,  730.f, 100.f, 18.f, false));
    plataformas.push_back(new Plataforma(570.f,  700.f, 120.f, 18.f, true ));
    plataformas.push_back(new Plataforma( 90.f,  680.f, 110.f, 18.f, false));

    // Fila 6
    plataformas.push_back(new Plataforma(240.f,  570.f, 130.f, 18.f, false));
    plataformas.push_back(new Plataforma(500.f,  540.f, 100.f, 18.f, true ));
    plataformas.push_back(new Plataforma( 50.f,  510.f, 120.f, 18.f, false));

    // Fila 7
    plataformas.push_back(new Plataforma(310.f,  400.f, 130.f, 18.f, false));
    plataformas.push_back(new Plataforma(560.f,  370.f, 110.f, 18.f, true ));
    plataformas.push_back(new Plataforma( 80.f,  350.f, 120.f, 18.f, false));

    // Fila 8
    plataformas.push_back(new Plataforma(200.f,  250.f, 130.f, 18.f, false));
    plataformas.push_back(new Plataforma(490.f,  220.f, 100.f, 18.f, true ));
    plataformas.push_back(new Plataforma( 60.f,  200.f, 110.f, 18.f, false));

    // META — plataforma dorada
    plataformas.push_back(new Plataforma(270.f,  110.f, 260.f, 18.f, false));
}

// ============================================================
//  crearItemsPlataformas
// ============================================================
void Nivel_1::crearItemsPlataformas()
{
    limpiarItemsPlataformas();

    const QColor colorSuelo (0x33, 0x44, 0x55, 255);
    const QColor colorNormal(0x1a, 0x8a, 0xff, 200);
    const QColor colorMovil (0x00, 0xff, 0xcc, 180);
    const QColor colorMeta  (0xff, 0xd7, 0x00, 255);
    const QPen   sinBorde(Qt::NoPen);

    for (int i = 0; i < static_cast<int>(plataformas.size()); ++i)
    {
        Plataforma* p = plataformas[i];
        QColor color;

        if (i == 0)
            color = colorSuelo;
        else if (i == static_cast<int>(plataformas.size()) - 1)
            color = colorMeta;
        else if (p->esMovil)
            color = colorMovil;
        else
            color = colorNormal;

        QGraphicsRectItem* item = escena->addRect(
            p->getX(), p->getY(), p->ancho, p->alto,
            sinBorde, QBrush(color));
        item->setZValue(5);
        itemsPlataformas.append(item);
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

    // Clamping: calcular cuántos px lógicos son visibles en Y
    QPointF topScene    = vista->mapToScene(0, 0);
    QPointF bottomScene = vista->mapToScene(0, vista->viewport()->height());
    float visibleH = bottomScene.y() - topScene.y();
    float halfVis  = visibleH / 2.f;

    if (camY - halfVis < 0.f)      camY = halfVis;
    if (camY + halfVis > ESCENA_H) camY = ESCENA_H - halfVis;

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
        if (i < itemsPlataformas.size() && itemsPlataformas[i])
            itemsPlataformas[i]->setRect(
                plataformas[i]->getX(), plataformas[i]->getY(),
                plataformas[i]->ancho,  plataformas[i]->alto);
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

    // 7. Cámara y HUD
    actualizarCamara();
    actualizarHUD();
}

void Nivel_1::verificarCaida()
{
    if (!jugador) return;
    if (jugador->getY() > LIMITE_CAIDA)
    {
        jugador->recibirDanio(1);
        jugador->resetearPosicion(spawnX, spawnY);
    }
}

void Nivel_1::verificarVictoria()
{
    if (!jugador || puertaCerrada) return;
    if (jugador->getY() < META_Y)
        completado = true;
}
