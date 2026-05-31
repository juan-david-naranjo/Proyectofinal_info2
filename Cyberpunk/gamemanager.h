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
#include <vector>

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
        NIVEL_1,
        NIVEL_2,
        PAUSADO,
        NIVEL_SUPERADO,   // pantalla entre nivel 1 y nivel 2
        VICTORIA,
        DERROTA
    };

    explicit GameManager(QGraphicsScene* escena,
                         QGraphicsView*  vista,
                         QObject*        parent = nullptr);
    ~GameManager() override;

    void iniciarJuego();
    void keyPressed (QKeyEvent* event);
    void keyReleased(QKeyEvent* event);
    void aplicarEscala();

    Estado getEstado() const { return estadoActual; }

private slots:
    void gameTick();
    void onContinuar();   // botón "CONTINUAR" del menú de pausa
    void onReiniciar();   // botón "REINICIAR"
    void onIrAlMenu();    // botón "MENÚ PRINCIPAL"
    void onSiguienteNivel(); // botón "SIGUIENTE NIVEL" de pantalla nivel superado

private:
    QGraphicsScene* escena;
    QGraphicsView*  vista;
    QTimer*         timer;
    QElapsedTimer   reloj;
    static constexpr int MS_POR_TICK = 16;

    Personaje* jugador;
    Nivel_1*   nivel1;
    Nivel_2*   nivel2;

    Estado estadoActual;
    Estado estadoAntesDePausa;

    std::vector<QGraphicsItem*> itemsOverlay;

    // ── Sonidos ───────────────────────────────────────────────
    QMediaPlayer musicaMenu;      // música de fondo del menú (MP3)
    QAudioOutput audioMenu;

    QSoundEffect sonidoClick;     // clic en botón (WAV)

    void cargarSonidos();
    void detenerTodaMusica();

    // ── Transiciones ──────────────────────────────────────────
    void irAMenu();
    void irANivel1();
    void irANivel2();
    void pausar();
    void reanudar();
    void mostrarNivelSuperado();     // transición segura al completar nivel 1
    void mostrarVictoria();

    // ── UI helpers ────────────────────────────────────────────
    void limpiarOverlay();
    void mostrarMenu();
    void mostrarPantallaPausa();
    void mostrarPantallaNivelSuperado();
    void mostrarPantallaVictoria();
    void agregarFondoOverlay();

    QGraphicsTextItem* agregarTextoOverlay(const QString& texto,
                                           QColor color, int tamano,
                                           float offsetY = 0.f,
                                           bool negrita  = false);

    // Crea un BotonMenu centrado, lo agrega al overlay y lo conecta al slot
    BotonMenu* agregarBotonOverlay(const QString& texto, float offsetY);
};

#endif // GAMEMANAGER_H
