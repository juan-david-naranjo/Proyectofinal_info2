#ifndef NIVEL_2_H
#define NIVEL_2_H

// ── 1. Inclusiones Base y del Proyecto ──────────────────────────────────────
#include "nivel.h"
#include "robotseguridad.h"
#include <vector>

// ── 2. Inclusiones de Qt Core / GUI / Widgets ───────────────────────────────
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QFont>

// ── 3. Inclusiones de Qt Multimedia ─────────────────────────────────────────
#include <QSoundEffect>
#include <QUrl>
#include <QMediaPlayer>
#include <QAudioOutput>

// ============================================================================
//  Nivel_2 — Vista cenital, laberinto + sigilo
// ============================================================================


class Nivel_2 : public Nivel
{
public:
    // ════════════════════════════════════════════════════════════════════════
    //  CONSTRUCTORES, DESTRUCTOR Y OPERADORES
    // ════════════════════════════════════════════════════════════════════════
    Nivel_2();
    Nivel_2(const Nivel_2& otro); // Sobrecarga Obligatoria
    Nivel_2& operator=(const Nivel_2& otro); // Regla de los Tres: asignación profunda
    bool operator==(const Nivel_2& otro) const;
    ~Nivel_2() override;

    // ════════════════════════════════════════════════════════════════════════
    //  MÉTODOS HEREDADOS (OVERRIDES)
    // ════════════════════════════════════════════════════════════════════════
    void inicializar(Personaje* p) override;
    void actualizar(float dt) override;

    // ════════════════════════════════════════════════════════════════════════
    //  MÉTODOS DE CONTROL DE ESCENA Y FLUJO
    // ════════════════════════════════════════════════════════════════════════
    void setScene(QGraphicsScene* scene);
    void setDificultad(bool dificil);
    void limpiarEscena();

    // Control de Audio
    void stopMusic();
    void playMusic();

    // ════════════════════════════════════════════════════════════════════════
    //  GETTERS DE ESTADO (PARA INTERFAZ / HUD EXTERNO)
    // ════════════════════════════════════════════════════════════════════════
    int  getTiempoRestante() const;
    bool isCompletado()      const;

    // ════════════════════════════════════════════════════════════════════════
    //  MÉTODOS DE CARGA DE ASSETS Y CONFIGURACIÓN GRAPHICS ITEMS
    // ════════════════════════════════════════════════════════════════════════
    void cargarSpriteObjetivo(const QPixmap& hoja, int srcX, int srcY, int srcW, int srcH);
    void cargarSpritesZonas(const QPixmap &hoja, int oxAnim, int oyAnim, int fwA, int fhA, int numAnim, int sepAnim, int oxE, int oyE, int fwE, int fhE, const std::vector<QColor>& fondos, int tolerancia = 10);

    // Inicializadores de elementos en escena
    void addWallScene();
    void addRobotScene();
    void addObjetivoScene();
    void addHideZone();
    void loadDestAnim();
    void addHudScene();

    // ════════════════════════════════════════════════════════════════════════
    //  ATRIBUTOS PÚBLICOS
    // ════════════════════════════════════════════════════════════════════════
    bool sinVidas = false;
    QPixmap* Escenario = nullptr;
    std::vector<RobotSeguridad*> robots;

    // Elementos y lógica de Zonas Ocultas
    bool jugadorCompletamenteOculto   = false;

private:

    QGraphicsRectItem* filtroOscuridad = nullptr;


    // ════════════════════════════════════════════════════════════════════════
    //  MÉTODOS PRIVADOS INTERNOS (HELPERS DE LOGICA)
    // ════════════════════════════════════════════════════════════════════════

    // Inicialización y Generación
    void generarLaberinto();
    void generarRobots();
    void limpiarRobots();
    void agregarItemsEscena();

    // Motor de Físicas y Colisiones
    void resolverColisiones();    ///< Jugador ↔ paredes
    void verificarDeteccion();    ///< Robot alcanza al jugador → daño + respawn
    void verificarVictoria(float dt);

    // Interfaz y Visibilidad
    void actualizarHUD();
    bool jugadorEnSombra() const;
    void actualizarCirculosDeteccion();

    // Animación de Destrucción y Soporte Gráfico
    void cargarSpritesDestruccion(const QPixmap& hoja, int ox, int oy, int fw, int fh, int numFrames, int sep = 0, QColor fondoColor = QColor(0,0,0,0));
    void actualizarAnimDestruccion(float dt);
    static QPixmap crearSpriteRobot(int w, int h);

    // ════════════════════════════════════════════════════════════════════════
    //  VARIABLES MIEMBRO PRIVADAS (ESTADO INTERNO)
    // ════════════════════════════════════════════════════════════════════════

    // Puntero Base del Framework
    QGraphicsScene* escena = nullptr;

    // Componentes del Sistema de Audio
    QSoundEffect sonidoDeteccion;
    QSoundEffect sonidoHackeoLoop;
    QSoundEffect sonidoVictoria;
    QSoundEffect sonidoDanio;
    QMediaPlayer musicaFondo;
    QAudioOutput audioFondo;

    // Control de Daño e Invulnerabilidad (i-frames)
    float tiempoInvulnerable = 0.f;
    static constexpr float DURACION_INVULNERABLE = 2.5f;

    // Variables de Tiempo y Spawns del Nivel
    float spawnX;
    float spawnY;
    float tiempoContador; // Acumulador para descontar tiempoRestante cada segundo
    bool  modoDificil;

    // Sistema de Hackeo (Objetivo Final)
    float tiempoHackeo    = 0.f;
    float tiempoHackeoMax = 3.f;
    bool  haciendoHackeo  = false;
    float objetivoX;
    float objetivoY;
    float objetivoRadio;

    // Componentes HUD de la Barra de Hackeo
    QGraphicsRectItem* itemBarraFondo   = nullptr;
    QGraphicsRectItem* itemBarraRelleno = nullptr;
    static constexpr float BARRA_ANCHO  = 137.f;
    static constexpr float BARRA_ALTO   = 12.f;

    // Animación por Frames de Destrucción del Objetivo
    std::vector<QPixmap> framesDestruccion;
    int   frameDestruccion    = 0;
    float tiempoFrameDestr    = 0.f;
    float duracionFrameDestr  = 0.12f;
    bool  animandoDestruccion = false;

    // IA y Seguimiento de Agentes
    std::vector<EstadoAgente> estadosAnteriores;

    // Elementos Gráficos del HUD Principal
    QGraphicsTextItem* itemHUDTimer = nullptr;
    std::vector<QGraphicsEllipseItem*> itemsCorazones;
    QGraphicsRectItem* hudFondoBoost = nullptr;
    QGraphicsRectItem* hudBarraBoost = nullptr;
    const float ANCHO_MAX_BARRA = 200.f;
    const float ALTO_MAX_BARRA  = 50.f;

    // Ítems Visuales de la Escena (Pertenezcan a Qt, se limpian vía escena)
    std::vector<QGraphicsItem*>        itemsParedes;
    QGraphicsPixmapItem* itemObjetivo = nullptr; // Computadora
    std::vector<QGraphicsEllipseItem*> itemsDeteccion;         // Círculos de los robots
    QGraphicsRectItem* debugJugadorRect = nullptr;
};

#endif // NIVEL_2_H
