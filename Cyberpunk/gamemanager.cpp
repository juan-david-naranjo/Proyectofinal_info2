#include "gamemanager.h"
#include <algorithm>
#include <QFont>
#include <QBrush>
#include <QPen>

// ════════════════════════════════════════════════════════════════════════════
//  Constructor / Destructor
// ════════════════════════════════════════════════════════════════════════════
GameManager::GameManager(QGraphicsScene* escena,
                         QGraphicsView*  vista,
                         QObject*        parent)
    : QObject(parent)
    , escena(escena)
    , vista(vista)
    , estadoActual(Estado::MENU)
    , estadoAntesDePausa(Estado::NIVEL_2)
{
    jugador = new Personaje(100.f, 200.f);
    nivel1  = new Nivel_1();
    nivel2  = new Nivel_2();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameManager::gameTick);

    cargarSonidos();
}

GameManager::~GameManager()
{
    delete nivel1;
    delete nivel2;
    delete jugador;
}

// ════════════════════════════════════════════════════════════════════════════
//  Sonidos
// ════════════════════════════════════════════════════════════════════════════
void GameManager::cargarSonidos()
{
    // ── Música de fondo del menú (MP3, loop infinito) ─────────────────────
    musicaMenu.setAudioOutput(&audioMenu);
    musicaMenu.setSource(QUrl("qrc:/sonidoswav/Sonidos/End of Line (From TRON_ LegacyScore).mp3"));
    audioMenu.setVolume(0.5f);
    musicaMenu.setLoops(QMediaPlayer::Infinite);

    // ── Clic de botón (WAV corto) ─────────────────────────────────────────
    sonidoClick.setSource(QUrl("qrc:/sonidoswav/Sonidos/clickwav.wav"));
    sonidoClick.setVolume(0.8f);
}

void GameManager::detenerTodaMusica()
{
    musicaMenu.stop();

}

// ════════════════════════════════════════════════════════════════════════════
//  iniciarJuego
// ════════════════════════════════════════════════════════════════════════════
void GameManager::iniciarJuego()
{
    irAMenu();
    reloj.start();
    timer->start(MS_POR_TICK);
}

// ════════════════════════════════════════════════════════════════════════════
//  gameTick
// ════════════════════════════════════════════════════════════════════════════
void GameManager::gameTick()
{
    float dt = static_cast<float>(reloj.restart()) / 1000.f;
    dt = std::clamp(dt, 0.001f, 0.1f);

    switch (estadoActual)
    {
    case Estado::NIVEL_1:
        nivel1->actualizar(dt);
        if (nivel1->completado) irANivel2();
        break;

    case Estado::NIVEL_2:
        nivel2->actualizar(dt);
        if (nivel2->completado) mostrarVictoria();
        if (nivel2->sinVidas)   mostrarGameOver();   // ← agregar
        break;

    default: break;
    }

    escena->update();
}

// ════════════════════════════════════════════════════════════════════════════
//  Transiciones
// ════════════════════════════════════════════════════════════════════════════
void GameManager::irAMenu()
{
    timer->stop();
    detenerTodaMusica();
    escena->clear();
    limpiarOverlay();

    estadoActual = Estado::MENU;
    mostrarMenu();

    musicaMenu.play();   // ← música del menú arranca aquí
    timer->start(MS_POR_TICK);
}

void GameManager::irANivel1()
{
    sonidoClick.play();
    detenerTodaMusica();
    limpiarOverlay();
    escena->clear();

    //nivel1->setScene(escena);
    nivel1->inicializar(jugador);
    jugador->cargarSpritesNivel1();

    vista->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    nivel1->aplicarEscalaView();

    estadoActual = Estado::NIVEL_1;

}

void GameManager::irANivel2()
{

    detenerTodaMusica();
    limpiarOverlay();

    if (jugador && jugador->getItem())          //proteccion para evitar comportamientos raros en memoria
        escena->removeItem(jugador->getItem());


    nivel2->limpiarEscena();
    escena->clear();
    nivel2->setScene(escena);
    nivel2->inicializar(jugador);
    jugador->cargarSpritesNivel2();
    vista->setAlignment(Qt::AlignCenter);
    vista->fitInView(escena->sceneRect(), Qt::KeepAspectRatio);
    estadoActual = Estado::NIVEL_2;


}

void GameManager::pausar()
{
    if (estadoActual != Estado::NIVEL_1 &&
        estadoActual != Estado::NIVEL_2) return;

    estadoAntesDePausa = estadoActual;
    estadoActual       = Estado::PAUSADO;

    // Bajar volumen en vez de cortar (más elegante)
    audioMenu.setVolume(0.15f);


    mostrarPantallaPausa();
}

void GameManager::reanudar()
{
    if (estadoActual != Estado::PAUSADO) return;

    sonidoClick.play();
    limpiarOverlay();
    estadoActual = estadoAntesDePausa;

    // Restaurar volumen original
    audioMenu.setVolume(0.5f);

}

void GameManager::mostrarVictoria()
{
    timer->stop();
    detenerTodaMusica();
    estadoActual = Estado::VICTORIA;
    mostrarPantallaVictoria();
}



// ════════════════════════════════════════════════════════════════════════════
//  Slots de botones del menú de pausa (conectados en mostrarPantallaPausa)
// ════════════════════════════════════════════════════════════════════════════
void GameManager::onContinuar() { reanudar();  }

void GameManager::onReiniciar()
{
    sonidoClick.play();
    nivel2->completado = false;
    nivel2->sinVidas   = false;
    irANivel2();
    timer->start(MS_POR_TICK);
    //qDebug("no hubo error!");
}
void GameManager::onIrAlMenu()
{
    timer->stop();
    detenerTodaMusica();
    // ── Misma protección ──────────────────────────────────────────────────
    if (jugador && jugador->getItem())
        escena->removeItem(jugador->getItem());



    nivel2->limpiarEscena();   // ← PRIMERO nullear punteros
    escena->clear();            // ← LUEGO destruir
    limpiarOverlay();

    estadoActual = Estado::MENU;
    mostrarMenu();
    musicaMenu.play();
    timer->start(MS_POR_TICK);
}

// ════════════════════════════════════════════════════════════════════════════
//  Entrada de teclado
// ════════════════════════════════════════════════════════════════════════════
void GameManager::keyPressed(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;

    switch (estadoActual)
    {
    case Estado::MENU:
        if (event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter  ||
            event->key() == Qt::Key_Space)
        {
            sonidoClick.play();
            irANivel2();   // cambiar a irANivel1() cuando esté listo
        }
        return;

    case Estado::PAUSADO:
        // El menú de pausa usa ratón, pero ESC sigue funcionando
        if (event->key() == Qt::Key_Escape) reanudar();
        return;

    case Estado::VICTORIA:
        if (event->key() == Qt::Key_R) { onReiniciar(); }
        if (event->key() == Qt::Key_M) { onIrAlMenu();  }
        return;

    case Estado::NIVEL_1:
        if (event->key() == Qt::Key_Escape) { pausar(); return; }
        switch (event->key())
        {
        case Qt::Key_A: case Qt::Key_Left:  jugador->keyPressed(0); break;
        case Qt::Key_D: case Qt::Key_Right: jugador->keyPressed(1); break;
        case Qt::Key_W: case Qt::Key_Up:
        case Qt::Key_Space:                 jugador->saltar();       break;
        default: break;
        }
        return;

    case Estado::NIVEL_2:
        if (event->key() == Qt::Key_Escape) { pausar(); return; }
        switch (event->key())
        {
        case Qt::Key_A: case Qt::Key_Left:  jugador->keyPressed(0);        break;
        case Qt::Key_D: case Qt::Key_Right: jugador->keyPressed(1);        break;
        case Qt::Key_W: case Qt::Key_Up:    jugador->keyPressed(2);        break;
        case Qt::Key_S: case Qt::Key_Down:  jugador->keyPressed(3);        break;
        case Qt::Key_Space:                 jugador->activarBoost();        break;
        case Qt::Key_Shift:                 jugador->activarDeslizNivel2(); break;
        default: break;
        }
        return;
    case Estado::DERROTA:
        if (event->key() == Qt::Key_R) { onReiniciar(); }
        if (event->key() == Qt::Key_M) { onIrAlMenu();  }
        return;
    default: break;
    }
}

void GameManager::keyReleased(QKeyEvent* event)
{
    if (!jugador || event->isAutoRepeat()) return;
    if (estadoActual != Estado::NIVEL_1 &&
        estadoActual != Estado::NIVEL_2) return;

    switch (event->key())
    {
    case Qt::Key_A: case Qt::Key_Left:  jugador->keyReleased(0); break;
    case Qt::Key_D: case Qt::Key_Right: jugador->keyReleased(1); break;
    case Qt::Key_W: case Qt::Key_Up:    jugador->keyReleased(2); break;
    case Qt::Key_S: case Qt::Key_Down:  jugador->keyReleased(3); break;
    default: break;
    }
}

void GameManager::aplicarEscala()
{
    if (estadoActual == Estado::NIVEL_1)
        nivel1->aplicarEscalaView();
    else
        vista->fitInView(escena->sceneRect(), Qt::KeepAspectRatio);
}

// ════════════════════════════════════════════════════════════════════════════
//  UI helpers
// ════════════════════════════════════════════════════════════════════════════
void GameManager::limpiarOverlay()
{
    for (QGraphicsItem* item : itemsOverlay)
        escena->removeItem(item);
    qDeleteAll(itemsOverlay);
    itemsOverlay.clear();
}

void GameManager::agregarFondoOverlay()
{
    QGraphicsRectItem* fondo = new QGraphicsRectItem(escena->sceneRect());
    fondo->setBrush(QBrush(QColor(0, 0, 0, 185)));
    fondo->setPen(Qt::NoPen);
    fondo->setZValue(10.0);
    escena->addItem(fondo);
    itemsOverlay.append(fondo);
}

QGraphicsTextItem* GameManager::agregarTextoOverlay(const QString& texto,
                                                    QColor color, int tamano,
                                                    float offsetY, bool negrita)
{
    QGraphicsTextItem* item = new QGraphicsTextItem(texto);
    item->setDefaultTextColor(color);
    item->setFont(QFont("Consolas", tamano,
                        negrita ? QFont::Bold : QFont::Normal));
    item->setZValue(11.0);

    QRectF r = item->boundingRect();
    item->setPos(escena->width()  * 0.5f - r.width()  * 0.5f,
                 escena->height() * 0.5f - r.height()  * 0.5f + offsetY);

    escena->addItem(item);
    itemsOverlay.append(item);
    return item;
}

// Crea un BotonMenu centrado horizontalmente con el offsetY dado
BotonMenu* GameManager::agregarBotonOverlay(const QString& texto, float offsetY)
{
    const float btnAncho = 340.f;
    const float btnAlto  =  52.f;

    BotonMenu* btn = new BotonMenu(texto, btnAncho, btnAlto);
    btn->setZValue(11.0);
    btn->setPos(escena->width()  * 0.5f - btnAncho * 0.5f,
                escena->height() * 0.5f - btnAlto   * 0.5f + offsetY);

    escena->addItem(btn);
    itemsOverlay.append(btn);
    return btn;
}

// ── Pantalla de menú ──────────────────────────────────────────────────────
void GameManager::mostrarMenu()
{
    escena->setBackgroundBrush(QColor(5, 10, 20));
    escena->setSceneRect(0, 0, 1250, 700);

    agregarTextoOverlay("OPERACIÓN KAEL",
                        QColor(0, 255, 120), 52, -150.f, true);
    agregarTextoOverlay("Infiltración nivel máximo",
                        QColor(100, 180, 140), 20, -85.f);

    // Botón de inicio (también responde a Enter/Space por teclado)
    BotonMenu* btnIniciar = agregarBotonOverlay("►  INICIAR MISIÓN", 10.f);
    connect(btnIniciar, &BotonMenu::clicked, this, [this](){
        sonidoClick.play();
        irANivel2();   // cambiar a irANivel1() cuando esté listo
    }, Qt::QueuedConnection);

    agregarTextoOverlay(
        "WASD — Mover  |  ESPACIO — Boost  |  SHIFT — Deslizar  |  ESC — Pausa",
        QColor(90, 110, 100), 13, 110.f);
}

// ── Pantalla de pausa ─────────────────────────────────────────────────────
// Los tres botones usan ratón. ESC también funciona para continuar.
void GameManager::mostrarPantallaPausa()
{
    agregarFondoOverlay();
    agregarTextoOverlay("— PAUSA —", QColor(255, 220, 80), 36, -130.f, true);

    // Botón CONTINUAR
    BotonMenu* btnContinuar = agregarBotonOverlay("►  CONTINUAR", -40.f);
    connect(btnContinuar, &BotonMenu::clicked, this, &GameManager::onContinuar, Qt::QueuedConnection);

    // Botón REINICIAR
    BotonMenu* btnReiniciar = agregarBotonOverlay("↺  REINICIAR NIVEL", 30.f);
    connect(btnReiniciar, &BotonMenu::clicked, this, &GameManager::onReiniciar ,Qt::QueuedConnection);

    // Botón MENÚ PRINCIPAL
    BotonMenu* btnMenu = agregarBotonOverlay("⌂  MENÚ PRINCIPAL", 100.f);
    connect(btnMenu, &BotonMenu::clicked, this, &GameManager::onIrAlMenu, Qt::QueuedConnection);

    agregarTextoOverlay("ESC — Continuar", QColor(80, 100, 90), 13, 175.f);
}

// ── Pantalla de victoria ──────────────────────────────────────────────────
void GameManager::mostrarPantallaVictoria()
{
    agregarFondoOverlay();
    agregarTextoOverlay("HACKEO COMPLETADO",
                        QColor(0, 255, 120), 42, -130.f, true);
    agregarTextoOverlay("Misión cumplida",
                        QColor(100, 220, 160), 20, -70.f);

    BotonMenu* btnReiniciar = agregarBotonOverlay("↺  REINTENTAR", 10.f);
    connect(btnReiniciar, &BotonMenu::clicked, this, &GameManager::onReiniciar);

    BotonMenu* btnMenu = agregarBotonOverlay("⌂  MENÚ PRINCIPAL", 80.f);
    connect(btnMenu, &BotonMenu::clicked, this, &GameManager::onIrAlMenu);
}

void GameManager::mostrarGameOver()
{
    timer->stop();
    detenerTodaMusica();
    estadoActual = Estado::DERROTA;

    agregarFondoOverlay();

    agregarTextoOverlay("GAME  OVER",
                        QColor(220, 40, 40), 52, -130.f, true);
    agregarTextoOverlay("El equipo de seguridad te atrapó",
                        QColor(200, 120, 120), 18, -60.f);

    BotonMenu* btnReintentar = agregarBotonOverlay("↺  REINTENTAR", 10.f);
    connect(btnReintentar, &BotonMenu::clicked,
            this, &GameManager::onReiniciar, Qt::QueuedConnection);

    BotonMenu* btnMenu = agregarBotonOverlay("⌂  MENÚ PRINCIPAL", 80.f);
    connect(btnMenu, &BotonMenu::clicked,
            this, &GameManager::onIrAlMenu, Qt::QueuedConnection);
}