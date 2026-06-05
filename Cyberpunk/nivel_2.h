#ifndef NIVEL_2_H
#define NIVEL_2_H

#include "nivel.h"
#include "robotseguridad.h"
#include <vector>
#include <QPixmap>
#include <QGraphicsScene>

#include <QSoundEffect>
#include <QUrl>
#include <QMediaPlayer>
#include <QAudioOutput>

// ============================================================
//  Nivel_2 — Vista cenital, laberinto + sigilo
//
//  Físicas activas:
//    • Inercia (MRUA) en el movimiento del personaje
//    • Colisiones AABB con obstáculos/paredes del laberinto
//    • Movimiento parabólico conservado (física base)
//
//  IA activa:
//    • RobotSeguridad: percibir → razonar → actuar (cada tick)
//    • Estado PATRULLAJE / PERSECUCION (mín. 3 s)
//    • Aprendizaje: incorpora última posición vista a waypoints
//
//  Dinámica:
//    • El jugador debe llegar a la computadora (objetivo) sin ser atrapado
//    • Si el robot toca al jugador → pierde una vida y respawnea
//  Emocion:
//    • la Inteligencia artificial entra en modo ataque cuando te percibe (Gira)
//    • el personaje sufre daño cuando la IA lo atrapa
// ============================================================

struct ZonaOculta {
    float x, y, w, h;
};
enum class EstadoZona { LIBRE, PROCESANDO, OCULTO };

struct DatoEstadoZona {
    EstadoZona estado     = EstadoZona::LIBRE;
    bool       fueUsada   = false;   // si fue usada, muestra frameOcupada al salir
    float      tiempoZona = 0.f;
    int        frame      = 0;
    float      tiempoFrame= 0.f;
};


class Nivel_2 : public Nivel
{
public:

    bool sinVidas = false;
    QPixmap *Escenario;
    Nivel_2();
    //Sobrecarga Obligatoria
    Nivel_2(const Nivel_2& otro);
    bool operator==(const Nivel_2& otro) const;


    ~Nivel_2() override;
    std::vector<RobotSeguridad*> robots;
    void inicializar(Personaje* p) override;
    void actualizar(float dt) override;

    //zonas
    std::vector<QGraphicsPixmapItem*> itemsZonaSprites;
    std::vector<DatoEstadoZona>       estadosZonas;
    std::vector<QPixmap> framesZonaApertura;   // animación al entrar
    QPixmap              frameZonaOcupada;     // primer frame de "los demás" (post-oculto)
    bool  jugadorCompletamenteOculto  = false;
    int   zonaActivaIdx               = -1;

    static constexpr float UMBRAL_QUIETO         = 15.f;  // px/s — umbral "quieto"
    static constexpr float TIEMPO_PARA_OCULTARSE = 2.f;   // segundos (ajustable)
    float                  duracionFrameZona      = 0.1f;

    // Getters de estado (para HUD externo)
    int   getTiempoRestante() const;
    bool  isCompletado()      const;
    //para el escenario
    void limpiarEscena();
    void setScene(QGraphicsScene* scene);
    void setDificultad(bool dificil);       //para escoger la dificultad
    void    cargarSpriteObjetivo(const QPixmap& hoja,
                              int srcX, int srcY,
                              int srcW, int srcH);
    void actualizarZonasOcultas(float dt);
    void cargarSpritesZonas(const QPixmap &hoja, int oxAnim, int oyAnim, int fwA, int fhA, int numAnim, int sepAnim, int oxE, int oyE, int fwE, int fhE,const std::vector<QColor>& fondos, int tolerancia = 10);
    void stopMusic();
    void playMusic();
    void addWallScene();
    void addRobotScene();
    void addObjetivoScene();
    void addHideZone();
    void loadDestAnim();
    void addHudScene();




private:
    //Escena Qt
    QGraphicsScene*              escena;          //Referencia a la escena actual
    // Sonidos por evento
    QSoundEffect sonidoDeteccion;   // robot entra en modo persecución
    QSoundEffect sonidoHackeoLoop;  // loop mientras hackeas la computadora
    QSoundEffect sonidoVictoria;    // hackeo completado
    QSoundEffect sonidoDanio;
    // ── Sonidos fondo ────────────────────────────────────────────────────
    QMediaPlayer musicaFondo;
    QAudioOutput audioFondo;



    // ── Sistema de daño con invulnerabilidad ──────────────────────────────────
    float tiempoInvulnerable = 0.f;   // contador de iframes tras recibir daño
    static constexpr float DURACION_INVULNERABLE = 2.5f;  // segundos sin poder ser dañado
    QGraphicsRectItem* debugJugadorRect = nullptr;


    // ── HUD ───────────────────────────────────────────────────────────────────
    QGraphicsTextItem*              itemHUDTimer = nullptr;
    std::vector<QGraphicsEllipseItem*> itemsCorazones;
    QGraphicsRectItem* hudFondoBoost = nullptr;
    QGraphicsRectItem* hudBarraBoost = nullptr;
    const float ANCHO_MAX_BARRA = 200.f; // El tamaño horizontal de la barra
    const float ALTO_MAX_BARRA = 50.f; // El tamaño horizontal de la barra

    void actualizarHUD();

    // seccion zona oculta:
    std::vector<ZonaOculta>  zonasOcultas;
    std::vector<QGraphicsItem*> itemsZonas;     // visuales oscuros


    bool jugadorEnSombra() const;
    // ── Lógica del hackeo ─────────────────────────────────────────────────────
    float tiempoHackeo     = 0.f;
    float tiempoHackeoMax  = 3.f;   // segundos para completar (ajustable)
    bool  haciendoHackeo   = false;

    // ── Control de cambios de estado para el sonido de detección ─────────────
    std::vector<EstadoAgente> estadosAnteriores;
    // Posición del objetivo (computadora a apagar)
    float objetivoX;
    float objetivoY;
    float objetivoRadio;
    float spawnX;
    float spawnY;
    float tiempoContador;     // Acumulador para descontar tiempoRestante cada segundo

    QGraphicsRectItem* itemBarraFondo    = nullptr;
    QGraphicsRectItem* itemBarraRelleno  = nullptr;
    static constexpr float BARRA_ANCHO   = 137.f;  // igual al ancho del sprite
    static constexpr float BARRA_ALTO    = 12.f;
    bool  modoDificil;


    // ── Animación de destrucción de la computadora ────────────────────────────
    std::vector<QPixmap> framesDestruccion;
    int   frameDestruccion   = 0;
    float tiempoFrameDestr   = 0.f;
    float duracionFrameDestr = 0.12f;   // ajustable
    bool  animandoDestruccion = false;

    // Métodos nuevos
    void cargarSpritesDestruccion(const QPixmap& hoja,
                                  int ox, int oy, int fw, int fh,
                                  int numFrames, int sep = 0,
                                  QColor fondoColor = QColor(0,0,0,0));
    void actualizarAnimDestruccion(float dt);






    std::vector<QGraphicsRectItem*> debugRobotsRect;        //hitbox robot


    // Ítems visuales (propiedad de la escena, no los borramos)
    std::vector<QGraphicsItem*>    itemsParedes;    //< Rectángulos de las paredes
    QGraphicsPixmapItem* itemObjetivo;                 //computadora
    std::vector<QGraphicsEllipseItem*> itemsDeteccion; //< Círculos de detección por robot


    void generarLaberinto();
    void generarRobots();
    void generarRobots(int dificult);
    void limpiarRobots();
    void resolverColisiones();    ///< Jugador ↔ paredes
    void verificarDeteccion();    ///< Robot alcanza al jugador → daño + respawn
    void verificarVictoria(float dt);


    void agregarItemsEscena();    ///< Crea y añade todos los QGraphicsItem a la escena
    void actualizarCirculosDeteccion(); ///< Mueve y recolorea los círculos cada tick

    static QPixmap crearSpriteRobot(int w, int h); ///< Dibuja el robot con QPainter



};

#endif // NIVEL_2_H
