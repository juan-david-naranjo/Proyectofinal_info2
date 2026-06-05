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
    , estadoAntesDePausa(Estado::NIVEL_1)
    , dificultadActual(Dificultad::FACIL)
{
    jugador = new Personaje(100.f, 200.f);
    nivel1  = new Nivel_1();
    nivel2  = new Nivel_2();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameManager::gameTick);

    cargarSonidos();
}

bool GameManager::operator==(const GameManager& otro) const
{
    return estadoActual == otro.estadoActual;
}

GameManager::~GameManager()
{
    timer->stop();
    musicaMenu.stop();
    delete nivel1;
    delete nivel2;
    delete jugador;
}

// ════════════════════════════════════════════════════════════════════════════
//  Sonidos
// ════════════════════════════════════════════════════════════════════════════
void GameManager::cargarSonidos()
{
    musicaMenu.setAudioOutput(&audioMenu);
    musicaMenu.setSource(QUrl("qrc:/sonidoswav/Sonidos/End of Line (From TRON_ LegacyScore).mp3"));
    audioMenu.setVolume(0.5f);
    musicaMenu.setLoops(QMediaPlayer::Infinite);

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
        if (nivel1->completado)
            mostrarNivel1Completado();
        else if (nivel1->puertaCerrada && nivel1->tiempoRestante <= 0)
            mostrarPuertaCerrada();
        break;

    case Estado::NIVEL_2:
        nivel2->actualizar(dt);
        if (nivel2->completado)  mostrarVictoria();
        else if (nivel2->sinVidas) mostrarGameOver();
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

    // Vaciar la lista ANTES de clear() para evitar dangling pointers
    itemsOverlay.clear();

    if (estadoActual == Estado::NIVEL_1 ||
        estadoActual == Estado::NIVEL_1_COMPLETADO ||
        estadoActual == Estado::PUERTA_CERRADA)
        nivel1->limpiarEscena();
    else if (estadoActual == Estado::NIVEL_2 ||
             estadoActual == Estado::DERROTA  ||
             estadoActual == Estado::VICTORIA)
        nivel2->limpiarEscena();

    if (jugador) jugador->invalidarItem();
    escena->clear();
    estadoActual = Estado::MENU;
    mostrarMenu();

    musicaMenu.play();
    timer->start(MS_POR_TICK);
}

// ── Pantalla de selección de dificultad ──────────────────────────────────
// NO se limpia la escena: el fondo del menú (setBackgroundBrush + sceneRect)
// ya está puesto por mostrarMenu(). Solo se reemplaza el overlay encima.
void GameManager::irASeleccionDificultad()
{
    sonidoClick.play();

    // Remover solo los ítems del overlay anterior de forma segura
    for (QGraphicsItem* item : itemsOverlay)
    {
        if (item->scene() == escena)
            escena->removeItem(item);
    }
    qDeleteAll(itemsOverlay);
    itemsOverlay.clear();

    estadoActual = Estado::SELECCION_DIFICULTAD;
    mostrarPantallaSeleccionDificultad();
}

void GameManager::irANivel1()
{
    sonidoClick.play();
    detenerTodaMusica();
    itemsOverlay.clear();

    nivel1->limpiarEscena();
    if (jugador) jugador->invalidarItem();
    escena->clear();
    if (jugador) jugador->recrearItem();

    // Aplicar dificultad al nivel 1
    nivel1->setDificultad(dificultadActual == Dificultad::DIFICIL);
    nivel1->setScene(escena, vista);   // configura sceneRect 800×1433 y fondo
    nivel1->inicializar(jugador);
    jugador->cargarSpritesNivel1();

    vista->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    nivel1->aplicarEscalaView();

    estadoActual = Estado::NIVEL_1;
}

void GameManager::irANivel2()
{
    detenerTodaMusica();

    itemsOverlay.clear();   // escena->clear() destruirá estos ítems

    nivel2->limpiarEscena();
    if (jugador) jugador->invalidarItem();
    escena->clear();
    if (jugador) jugador->recrearItem();
    nivel2->setScene(escena);
    nivel2->inicializar(jugador);
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
    if (estadoAntesDePausa == Estado::NIVEL_2)
        nivel2->stopMusic();

    mostrarPantallaPausa();
}

void GameManager::reanudar()
{
    if (estadoActual != Estado::PAUSADO) return;
    if (estadoAntesDePausa == Estado::NIVEL_2)
        nivel2->playMusic();
    sonidoClick.play();
    limpiarOverlay();
    estadoActual = estadoAntesDePausa;
}

void GameManager::mostrarVictoria()
{
    timer->stop();
    detenerTodaMusica();
    nivel2->limpiarEscena();
    if (jugador) jugador->invalidarItem();
    escena->clear();
    estadoActual = Estado::VICTORIA;
    mostrarPantallaVictoria();
}

void GameManager::mostrarPuertaCerrada()
{
    timer->stop();
    detenerTodaMusica();
    estadoActual = Estado::PUERTA_CERRADA;
    mostrarPantallaPuertaCerrada();
}

// ── Nivel 1 completado: para el juego y muestra pantalla de transición ────
void GameManager::mostrarNivel1Completado()
{
    timer->stop();
    nivel1->stopMusic();
    estadoActual = Estado::NIVEL_1_COMPLETADO;
    mostrarPantallaNivel1Completado();
    timer->start(MS_POR_TICK);   // el timer sigue corriendo para que la escena
    // responda al ratón (botones BotonMenu)
}

// ════════════════════════════════════════════════════════════════════════════
//  Slots de botones
// ════════════════════════════════════════════════════════════════════════════
void GameManager::onContinuar() { reanudar(); }

void GameManager::onReiniciar()
{
    timer->stop();
    sonidoClick.play();
    Estado ref = (estadoActual == Estado::PAUSADO)
                     ? estadoAntesDePausa
                     : estadoActual;

    switch (ref)
    {
    case Estado::NIVEL_1:
    case Estado::NIVEL_1_COMPLETADO:
    case Estado::PUERTA_CERRADA:
        irANivel1();
        break;
    case Estado::NIVEL_2:
    case Estado::DERROTA:
    case Estado::VICTORIA:
        nivel2->completado = false;
        nivel2->sinVidas   = false;
        irANivel2();
        break;
    default:
        irAMenu();
        break;
    }
    timer->start(MS_POR_TICK);
}

void GameManager::onReintentar_N1()
{
    // Reinicia el nivel 1 con la dificultad ya seleccionada
    timer->stop();
    sonidoClick.play();
    irANivel1();
    timer->start(MS_POR_TICK);
}

// ── Nuevo slot: continuar al nivel 2 desde la pantalla NIVEL_1_COMPLETADO ─
void GameManager::onIrANivel2()
{
    timer->stop();
    sonidoClick.play();

    // limpiarOverlay no es necesario porque irANivel2 llama escena->clear()
    // pero vaciamos la lista para evitar dangling pointers
    itemsOverlay.clear();

    nivel1->restaurarView();   // restaura la view al estado neutro antes de N2
    irANivel2();
    timer->start(MS_POR_TICK);
}

void GameManager::onIrAlMenu()
{
    timer->stop();
    detenerTodaMusica();

    itemsOverlay.clear();

    if (estadoActual == Estado::NIVEL_1        ||
        estadoActual == Estado::NIVEL_1_COMPLETADO ||
        estadoActual == Estado::PUERTA_CERRADA)
    {
        nivel1->limpiarEscena();
        if (jugador) jugador->invalidarItem();
    }
    else if (estadoActual == Estado::NIVEL_2)
    {
        nivel2->limpiarEscena();
        if (jugador) jugador->invalidarItem();
    }
    else if (estadoActual == Estado::DERROTA ||
             estadoActual == Estado::VICTORIA)
    {
        if (jugador) jugador->invalidarItem();
    }

    escena->clear();
    estadoActual = Estado::MENU;
    mostrarMenu();
    musicaMenu.play();
    timer->start(MS_POR_TICK);
}

void GameManager::onSeleccionarFacil()
{
    sonidoClick.play();
    dificultadActual = Dificultad::FACIL;
    irANivel1();
    timer->start(MS_POR_TICK);
}

void GameManager::onSeleccionarDificil()
{
    sonidoClick.play();
    dificultadActual = Dificultad::DIFICIL;
    irANivel1();
    timer->start(MS_POR_TICK);
}

void GameManager::irASeleccionClase()           //tipo de personalidad de kael
{
    limpiarOverlay();
    //escena->clear();
    escena->setBackgroundBrush(QColor(5, 10, 20));
    escena->setSceneRect(0, 0, 1250, 700);
    agregarTextoOverlay("CARGANDO PERFIL TÁCTICO", QColor(0, 255, 120), 40, -150.f, true);
    agregarTextoOverlay("Selecciona la especialidad del traje de Kael", Qt::white, 16, -90.f);

    BotonMenu* btnVelocista = agregarBotonOverlay("Clase: VELOCISTA", -20.f);
    agregarTextoOverlay("Habilidad: Dash de velocidad extrema (3s).", QColor(150,150,150), 12, 15.f);

    BotonMenu* btnEspectro = agregarBotonOverlay("Clase: ESPECTRO", 70.f);
    agregarTextoOverlay("Habilidad: Camuflaje óptico indetectable (2s).", QColor(150,150,150), 12, 105.f);

    connect(btnVelocista, &BotonMenu::clicked, this, [this](){
        sonidoClick.play();
        jugador->setClase(Personaje::ClaseActiva::VELOCISTA);
        irANivel2(); // ¡Ahora sí vamos al nivel!
    });

    connect(btnEspectro, &BotonMenu::clicked, this, [this](){
        sonidoClick.play();
        jugador->setClase(Personaje::ClaseActiva::ESPECTRO);
        irANivel2(); // ¡Ahora sí vamos al nivel!
    });
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
            irASeleccionDificultad();
        }
        return;

    case Estado::SELECCION_DIFICULTAD:
        // Los botones usan ratón; ESC vuelve al menú
        if (event->key() == Qt::Key_Escape) irAMenu();
        return;

    case Estado::PAUSADO:
        if (event->key() == Qt::Key_Escape) reanudar();
        return;

    case Estado::VICTORIA:
        if (event->key() == Qt::Key_R) { onReiniciar(); }
        if (event->key() == Qt::Key_M) { onIrAlMenu();  }
        return;

    case Estado::PUERTA_CERRADA:
        if (event->key() == Qt::Key_R) { onReintentar_N1(); }
        if (event->key() == Qt::Key_M) { onIrAlMenu(); }
        return;

    // ── Nuevo: atajos de teclado para la pantalla de transición N1→N2 ──
    case Estado::NIVEL_1_COMPLETADO:
        if (event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter  ||
            event->key() == Qt::Key_Space)  { onIrANivel2(); }
        if (event->key() == Qt::Key_M)      { onIrAlMenu();  }
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
        case Qt::Key_Space:                 jugador->usarHabilidadEspecial(); break;
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
    {
        // Protección: solo remover si el ítem pertenece a esta escena
        if (item && item->scene() == escena)
            escena->removeItem(item);
    }
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

// ── Menú principal ────────────────────────────────────────────────────────
void GameManager::mostrarMenu()
{
    escena->setBackgroundBrush(QColor(5, 10, 20));
    escena->setSceneRect(0, 0, 1250, 700);

    agregarTextoOverlay("OPERACIÓN KAEL",
                        QColor(0, 255, 120), 52, -150.f, true);
    agregarTextoOverlay("Infiltración nivel máximo",
                        QColor(100, 180, 140), 20, -85.f);

    BotonMenu* btnIniciar = agregarBotonOverlay("►  INICIAR MISIÓN", 10.f);
    connect(btnIniciar, &BotonMenu::clicked, this, [this](){
        irASeleccionDificultad();
    }, Qt::QueuedConnection);

    agregarTextoOverlay(
        "WASD — Mover  |  ESPACIO — Saltar/Boost  |  ESC — Pausa",
        QColor(90, 110, 100), 13, 110.f);
}

// ── Selección de dificultad ───────────────────────────────────────────────
void GameManager::mostrarPantallaSeleccionDificultad()
{
    // Reutiliza el fondo oscuro del menú (ya está puesto por mostrarMenu)
    agregarTextoOverlay("SELECCIONA DIFICULTAD",
                        QColor(0, 220, 255), 36, -160.f, true);
    agregarTextoOverlay("¿Qué tan difícil quieres que sople el viento?",
                        QColor(120, 180, 160), 16, -100.f);

    // Botón FÁCIL
    BotonMenu* btnFacil = agregarBotonOverlay("◎  FÁCIL  — viento suave", -20.f);
    connect(btnFacil, &BotonMenu::clicked,
            this, &GameManager::onSeleccionarFacil, Qt::QueuedConnection);

    // Botón DIFÍCIL
    BotonMenu* btnDificil = agregarBotonOverlay("◈  DIFÍCIL — viento fuerte", 55.f);
    connect(btnDificil, &BotonMenu::clicked,
            this, &GameManager::onSeleccionarDificil, Qt::QueuedConnection);

    agregarTextoOverlay("ESC — Volver al menú", QColor(80, 100, 90), 12, 150.f);
}

// ── Pausa ─────────────────────────────────────────────────────────────────
void GameManager::mostrarPantallaPausa()
{
    agregarFondoOverlay();
    agregarTextoOverlay("— PAUSA —", QColor(255, 220, 80), 36, -130.f, true);

    BotonMenu* btnContinuar = agregarBotonOverlay("►  CONTINUAR", -40.f);
    connect(btnContinuar, &BotonMenu::clicked,
            this, &GameManager::onContinuar, Qt::QueuedConnection);

    BotonMenu* btnReiniciar = agregarBotonOverlay("↺  REINICIAR NIVEL", 30.f);
    connect(btnReiniciar, &BotonMenu::clicked,
            this, &GameManager::onReiniciar, Qt::QueuedConnection);

    BotonMenu* btnMenu = agregarBotonOverlay("⌂  MENÚ PRINCIPAL", 100.f);
    connect(btnMenu, &BotonMenu::clicked,
            this, &GameManager::onIrAlMenu, Qt::QueuedConnection);

    agregarTextoOverlay("ESC — Continuar", QColor(80, 100, 90), 13, 175.f);
}

// ── Victoria ──────────────────────────────────────────────────────────────
void GameManager::mostrarPantallaVictoria()
{
    agregarFondoOverlay();
    agregarTextoOverlay("HACKEO COMPLETADO",
                        QColor(0, 255, 120), 42, -130.f, true);
    agregarTextoOverlay("Misión cumplida",
                        QColor(100, 220, 160), 20, -70.f);

    BotonMenu* btnReiniciar = agregarBotonOverlay("↺  REINTENTAR", 10.f);
    connect(btnReiniciar, &BotonMenu::clicked,
            this, &GameManager::onReiniciar, Qt::QueuedConnection);

    BotonMenu* btnMenu = agregarBotonOverlay("⌂  MENÚ PRINCIPAL", 80.f);
    connect(btnMenu, &BotonMenu::clicked,
            this, &GameManager::onIrAlMenu, Qt::QueuedConnection);
}

// ── Game Over (nivel 2) ───────────────────────────────────────────────────
void GameManager::mostrarGameOver()
{
    timer->stop();
    detenerTodaMusica();
    nivel2->limpiarEscena();
    if (jugador) jugador->invalidarItem();
    escena->clear();
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

// ── Puerta Cerrada (tiempo agotado en nivel 1) ────────────────────────────
// Usa helpers _Cam porque se muestra sobre la escena scrolleable del N1.
void GameManager::mostrarPantallaPuertaCerrada()
{
    agregarFondoOverlay_Cam();
    agregarTextoOverlay_Cam("PUERTA  CERRADA",
                            QColor(255, 60, 60), 48, -140.f, true);
    agregarTextoOverlay_Cam("Se acabó el tiempo — el acceso fue bloqueado",
                            QColor(220, 130, 130), 17, -75.f);

    BotonMenu* btnReintentar = agregarBotonOverlay_Cam("↺  REINTENTAR", 10.f);
    connect(btnReintentar, &BotonMenu::clicked,
            this, &GameManager::onReintentar_N1, Qt::QueuedConnection);

    BotonMenu* btnMenu = agregarBotonOverlay_Cam("⌂  MENÚ PRINCIPAL", 80.f);
    connect(btnMenu, &BotonMenu::clicked,
            this, &GameManager::onIrAlMenu, Qt::QueuedConnection);

    agregarTextoOverlay_Cam("R — Reintentar  |  M — Menú",
                            QColor(80, 100, 90), 12, 158.f);
}

// ════════════════════════════════════════════════════════════════════════════
//  UI helpers — coordenadas relativas al viewport visible de la cámara
//  Necesarios cuando la escena tiene scroll (nivel 1: 800×1433).
//  Calculan posiciones a partir del rect que la view realmente muestra.
// ════════════════════════════════════════════════════════════════════════════

// Devuelve el rectángulo del viewport en coordenadas de escena.
QRectF GameManager::viewportEnEscena() const
{
    QPointF tl = vista->mapToScene(0, 0);
    QPointF br = vista->mapToScene(vista->viewport()->width(),
                                   vista->viewport()->height());
    return QRectF(tl, br);
}

void GameManager::agregarFondoOverlay_Cam()
{
    QRectF vp = viewportEnEscena();
    QGraphicsRectItem* fondo = new QGraphicsRectItem(vp);
    fondo->setBrush(QBrush(QColor(0, 0, 0, 200)));
    fondo->setPen(Qt::NoPen);
    fondo->setZValue(10.0);
    escena->addItem(fondo);
    itemsOverlay.append(fondo);
}

QGraphicsTextItem* GameManager::agregarTextoOverlay_Cam(const QString& texto,
                                                        QColor color, int tamano,
                                                        float offsetY, bool negrita)
{
    QRectF vp = viewportEnEscena();

    QGraphicsTextItem* item = new QGraphicsTextItem(texto);
    item->setDefaultTextColor(color);
    item->setFont(QFont("Consolas", tamano,
                        negrita ? QFont::Bold : QFont::Normal));
    item->setZValue(11.0);

    QRectF r = item->boundingRect();
    item->setPos(vp.left() + vp.width()  * 0.5f - r.width()  * 0.5f,
                 vp.top()  + vp.height() * 0.5f - r.height() * 0.5f + offsetY);

    escena->addItem(item);
    itemsOverlay.append(item);
    return item;
}

BotonMenu* GameManager::agregarBotonOverlay_Cam(const QString& texto, float offsetY)
{
    QRectF vp = viewportEnEscena();
    const float btnAncho = 340.f;
    const float btnAlto  =  52.f;

    BotonMenu* btn = new BotonMenu(texto, btnAncho, btnAlto);
    btn->setZValue(11.0);
    btn->setPos(vp.left() + vp.width()  * 0.5f - btnAncho * 0.5f,
                vp.top()  + vp.height() * 0.5f - btnAlto  * 0.5f + offsetY);

    escena->addItem(btn);
    itemsOverlay.append(btn);
    return btn;
}

// ── Nivel 1 Completado — usa helpers _Cam para cuadrar con el viewport ────
void GameManager::mostrarPantallaNivel1Completado()
{
    agregarFondoOverlay_Cam();

    agregarTextoOverlay_Cam("ACCESO  CONCEDIDO",
                            QColor(0, 255, 120), 52, -150.f, true);
    agregarTextoOverlay_Cam("Nivel 1 superado — la puerta está abierta",
                            QColor(100, 180, 140), 20, -85.f);

    BotonMenu* btnContinuar = agregarBotonOverlay_Cam("►  CONTINUAR MISIÓN", 10.f);
    connect(btnContinuar, &BotonMenu::clicked,
            this, &GameManager::irASeleccionClase, Qt::QueuedConnection);

    BotonMenu* btnReintentar = agregarBotonOverlay_Cam("↺  REINTENTAR NIVEL 1", 80.f);
    connect(btnReintentar, &BotonMenu::clicked,
            this, &GameManager::onReintentar_N1, Qt::QueuedConnection);

    agregarTextoOverlay_Cam("ESPACIO — Continuar misión  |  M — Menú principal",
                            QColor(90, 110, 100), 13, 175.f);
}

