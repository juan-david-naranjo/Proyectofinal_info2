#ifndef PLATAFORMA_H
#define PLATAFORMA_H

#include "entidadjuego.h"
#include <QPixmap>
#include <vector>
#include <QPainter>
// ============================================================
//  Plataforma
//  Objeto estático del nivel 1 (y obstáculo en nivel 2).
//  Tiene una Hitbox explícita para colisiones (sin Qt).
//  esMovil: si true, la plataforma oscila horizontalmente
//           (plataforma móvil opcional).
// ============================================================




class Plataforma : public EntidadJuego
{
public:
    // ── Tipo de muro para seleccionar sprite ──────────────────
    enum class TipoMuro {
        SIN_SPRITE,   // borde del nivel → color sólido
        HORIZONTAL,   // sprite tileado en X
        VERTICAL      // sprite tileado en Y
    };



    float ancho;
    float alto;
    bool  esMovil;
    TipoMuro tipoMuro;

    // Para plataformas móviles
    float amplitudMovimiento;   // px de desplazamiento máx.
    float velocidadMovimiento;  // px/s
    float origenX;              // X inicial (centro del recorrido)

    Plataforma();
    Plataforma(float x, float y, float w, float h, bool movil = false,TipoMuro tipo  = TipoMuro::SIN_SPRITE);
    Plataforma(const Plataforma& otro);
    Plataforma& operator=(const Plataforma& otro);  // Regla de los Tres
    bool operator==(const Plataforma& otro) const;

    // Hitbox explícita para el sistema de colisiones (sin Qt).
    // Siempre refleja la posición actual (x,y) y el tamaño de la plataforma.
    Hitbox getHitbox() const override { return Hitbox(x, y, ancho, alto); }

    // EntidadJuego::actualizar — mueve la plataforma si es móvil
    void actualizar(float dt) override;

    ~Plataforma() override = default;

    void cargarSprite(const QPixmap& hoja,
                                  int srcX, int srcY,
                                  int srcW, int srcH);

    //hoja de pixmap para agregar los sprites de los muros

    // QPixmap datasheet;

    std::vector<QPixmap>laberintos;
private:
    QPixmap tilearSprite(const QPixmap& fuente,
                         int anchoFinal, int altoFinal);
};


#endif // PLATAFORMA_H
