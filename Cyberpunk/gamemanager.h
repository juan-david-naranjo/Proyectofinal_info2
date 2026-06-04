#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QList>

// Sonidos
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QUrl>

#include "personaje.h"
#include "nivel_1.h"
#include "nivel_2.h"
#include "botonmenu.h"

// ============================================================
//  GameManager  —  cerebro del juego
// ============================================================
class GameManager : public QObject
{
    Q_OBJECT

public:
    enum class Estado {
        MENU,
        SELECCION_DIFICULTAD,
        NIVEL_1,
        NIVEL_1_COMPLETADO,     // nivel 1 superado, elige continuar al 2
        NIVEL_2,
        PAUSADO,
        VICTORIA,
        DERROTA,
        PUERTA_CERRADA          // tiempo agotado en nivel 1
    };

    enum class Dificultad { FACIL = 0, DIFICIL = 1 };

    bool operator==(const GameManager& otro) const;

    explicit GameManager(QGraphicsScene* escena,
                         QGraphicsView*  vista,
                         QObject*        parent = nullptr);
    ~GameManager() override;

    void iniciarJuego();
    void keyPressed (QKeyEvent* event);
    void keyReleased(QKeyEvent* event);
    void aplicarEscala();

    Estado     getEstado()      const { return estadoActual; }
    Dificultad getDificultad()  const { return dificultadActual; }

private slots:
    void gameTick();
    void onContinuar();
    void onReiniciar();
    void onIrAlMenu();
    void onSeleccionarFacil();
    void onSeleccionarDificil();
    void onReintentar_N1();
    void onIrANivel2();

private:
    GameManager(const GameManager&) = delete;

    QGraphicsScene* escena;
    QGraphicsView*  vista;
    QTimer*         timer;
    QElapsedTimer   reloj;
    static constexpr int MS_POR_TICK = 16;

    Personaje*  jugador;
    Nivel_1*    nivel1;
    Nivel_2*    nivel2;

    Estado      estadoActual;
    Estado      estadoAntesDePausa;
    Dificultad  dificultadActual;

    QList<QGraphicsItem*> itemsOverlay;

    // ── Sonidos ───────────────────────────────────────────────
    QMediaPlayer musicaMenu;
    QAudioOutput audioMenu;
    QSoundEffect sonidoClick;

    void cargarSonidos();
    void detenerTodaMusica();

    // ── Transiciones ──────────────────────────────────────────
    void irAMenu();
    void irASeleccionDificultad();
    void irANivel1();
    void irANivel2();
    void pausar();
    void reanudar();
    void mostrarVictoria();
    void mostrarGameOver();
    void mostrarPuertaCerrada();
    void mostrarNivel1Completado();

    // ── UI helpers — coordenadas absolutas de escena ──────────
    // Usados cuando sceneRect == viewport (menú, N2, victoria…)
    void limpiarOverlay();
    void mostrarMenu();
    void mostrarPantallaSeleccionDificultad();
    void mostrarPantallaPausa();
    void mostrarPantallaVictoria();
    void mostrarPantallaPuertaCerrada();
    void agregarFondoOverlay();

    QGraphicsTextItem* agregarTextoOverlay(const QString& texto,
                                           QColor color, int tamano,
                                           float offsetY = 0.f,
                                           bool negrita  = false);
    BotonMenu* agregarBotonOverlay(const QString& texto, float offsetY);

    // ── UI helpers — coordenadas relativas al viewport de cámara ─
    // Usados en pantallas que se muestran sobre la escena scrolleable
    // del nivel 1 (sceneRect 800×1433, cámara en posición arbitraria).
    void mostrarPantallaNivel1Completado();

    QRectF          viewportEnEscena() const;   // rect visible en coords de escena
    void            agregarFondoOverlay_Cam();
    QGraphicsTextItem* agregarTextoOverlay_Cam(const QString& texto,
                                               QColor color, int tamano,
                                               float offsetY = 0.f,
                                               bool negrita  = false);
    BotonMenu*      agregarBotonOverlay_Cam(const QString& texto, float offsetY);
};

#endif // GAMEMANAGER_H
