#ifndef NIVEL_1_H
#define NIVEL_1_H

#include "nivel.h"

//para el fondo
#include <QPixmap>
#include <QGraphicsScene>

// ============================================================
//  Nivel_1 — Vista lateral, Jump-King style
//
//  Físicas activas:
//    • Movimiento parabólico (gravedad en Personaje::actualizarNivel1)
//    • Fricción con el viento (oscilatorio via GestorFisicas)
//    • Movimiento oscilatorio del viento
//    • Colisiones AABB con plataformas
//
//  Dinámica:
//    • Subir plataformas antes de que se acabe el tiempo
//    • Al agotarse el tiempo, se cierra la puerta (puertatCerrada=true)
//    • Si el jugador cae, se reinicia desde el inicio
// ============================================================
class Nivel_1 : public Nivel
{
public:

    QPixmap *Escenario;


    Nivel_1();
    ~Nivel_1() override;

    float velocidadScroll;      // Desplazamiento de la cámara
    bool  puertaCerrada;        // true = tiempo agotado, jugador no pasa

    void inicializar(Personaje* p) override;
    void actualizar(float dt) override;



    //para el escenario
    void setScene(QGraphicsScene* scene);

private:
    static constexpr int TIEMPO_NIVEL = 90;   // segundos para llegar arriba
    float timerAcumulado;

    // Posición de spawn del jugador
    float spawnX;
    float spawnY;

    // Genera las plataformas del nivel (disposición vertical)
    void generarPlataformas();

    // Comprueba si el jugador cayó por debajo del nivel
    void verificarCaida();


};

#endif // NIVEL_1_H
