#ifndef NIVEL_1_H
#define NIVEL_1_H

#include "nivel.h"
#include <vector>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>

// Sonidos
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QUrl>

// ============================================================
//  Nivel_1 — Vista lateral, Jump-King style
//
//  Sprite de fondo real: 1536 × 2752 px
//  Escena lógica:         800 × 1433 px  (escala 0.5208)
//
//  La view escala uniformemente para que los 800 px lógicos
//  llenen el ancho disponible. La cámara solo se mueve en Y
//  mediante centerOn(), siguiendo al jugador.
//  Esta configuración es EXCLUSIVA del nivel 1.
// ============================================================
class Nivel_1 : public Nivel
{
public:
    QPixmap* Escenario;
    bool     puertaCerrada;

    Nivel_1();
    ~Nivel_1() override;

    void inicializar(Personaje* p) override;
    void actualizar(float dt)      override;

    // Asigna escena y view; llamar ANTES de inicializar()
    void setScene(QGraphicsScene* scene, QGraphicsView* view);

    // Recalcula la escala cuando la ventana cambia de tamaño
    void aplicarEscalaView();

    // Restaura la view al estado neutro antes de pasar al nivel 2
    void restaurarView();

    // ── Dificultad ────────────────────────────────────────────
    void setDificultad(bool dificil);

    // ── Sonidos ───────────────────────────────────────────────
    void stopMusic();
    void playMusic();

private:
    // ── Dimensiones lógicas ───────────────────────────────────
    static constexpr float ESCENA_W     = 800.f;
    static constexpr float ESCENA_H     = 1433.f;

    // ── Constantes del juego ──────────────────────────────────
    static constexpr int   TIEMPO_NIVEL = 90;
    static constexpr float LIMITE_CAIDA = ESCENA_H + 50.f;
    static constexpr float META_Y       = 310.f;
    static constexpr float CAM_OFFSET_Y = 150.f;

    // Amplitud del viento en modo difícil
    static constexpr float VIENTO_AMPLITUD_DIFICIL = 240.f;

    // ── Spawn ─────────────────────────────────────────────────
    float spawnX;
    float spawnY;

    // ── Tiempo ────────────────────────────────────────────────
    float timerAcumulado;

    // ── Dificultad ────────────────────────────────────────────
    bool  modoDificil;
    float vientoAmplitud;

    // ── Referencias Qt ────────────────────────────────────────
    QGraphicsScene* escena;
    QGraphicsView*  vista;

    // ── Items gráficos de plataformas ─────────────────────────
    QPixmap spritePlataforma;
    std::vector<QGraphicsPixmapItem*> itemsPlataformas;

    // ── HUD ───────────────────────────────────────────────────
    // Timer estilo N2: fondo + texto centrado, sin barra ni vidas
    QGraphicsRectItem* fondoHUD;        // banda semitransparente en la cima
    QGraphicsTextItem* hudTiempo;       // "1:30" centrado
    QGraphicsTextItem* hudDificultad;   // "FÁCIL" / "DIFÍCIL" a la derecha
    QGraphicsTextItem* hudPuerta;       // aviso "¡PUERTA CERRADA!"

    // ── Debug ─────────────────────────────────────────────────
    static constexpr bool DEBUG_HITBOX = true;
    QGraphicsRectItem* debugHitboxItem;

    // ── Sonidos ───────────────────────────────────────────────
    QMediaPlayer musicaFondo;
    QAudioOutput audioFondo;
    QSoundEffect sonidoSalto;

    bool saltandoAnterior;

    // ── Helpers ───────────────────────────────────────────────
    void generarPlataformas();
    void crearItemsPlataformas();
    void limpiarItemsPlataformas();
    void crearHUD();
    void actualizarHUD();
    void actualizarCamara();
    void verificarCaida();
    void verificarVictoria();
};

#endif // NIVEL_1_H
