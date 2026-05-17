#include "personaje.h"
#include "gestorfisicas.h"
#include <algorithm>
#include <cmath>

Personaje::Personaje() : EntidadJuego(0, 0)
{
    vidas          = 3;
    energia        = 100.f;
    velMax         = 200.f;      // px/s
    enSuelo        = false;
    std::fill(std::begin(keys), std::end(keys), false);

    puedeDoubleSalto = true;
    fuerzaSalto      = 450.f;   // px/s inicial hacia arriba
    saltosRestantes  = 2;        // doble salto habilitado

    deslizando       = false;
    tiempoDesliz     = 0.f;

    boostActivo      = false;
    tiempoBoost      = 0.f;

    factorSigilo     = 1.f;

    Sprite       = nullptr;
    itemGrafico  = nullptr;
}

Personaje::Personaje(float X, float Y) : Personaje()
{
    x = X;
    y = Y;

    itemGrafico = new QGraphicsPixmapItem();
    // Sprite      = new QPixmap(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");

    cargarSprites();




    // if (!Sprite->isNull())
    // {
    //     /**/
    //     /**/
    //     itemGrafico->setPixmap(Sprite->copy(0,0, (int)ANCHO, (int)ALTO));
    //     // qDebug("aqui crasheo");
    // }
    itemGrafico->setPixmap(QPixmap(50, 50));
    itemGrafico->pixmap().fill(Qt::red);
    itemGrafico->setPos(x, y);
}

Personaje::~Personaje()
{
    delete Sprite;
}

// ── Entrada ──────────────────────────────────────────────────────────────────
void Personaje::keyPressed(int key)
{
    if (key >= 0 && key < 4) keys[key] = true;

}

void Personaje::keyReleased(int key)
{
    if (key >= 0 && key < 4) keys[key] = false;
}

// ── Salto ─────────────────────────────────────────────────────────────────────
void Personaje::saltar()
{
    if (saltosRestantes <= 0) return;

    Vy = -fuerzaSalto;       // Negativo: sube en Qt (Y crece hacia abajo)
    enSuelo = false;
    saltosRestantes--;

    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Boost ─────────────────────────────────────────────────────────────────────
void Personaje::activarBoost()
{
    boostActivo  = true;
    tiempoBoost  = DURACION_BOOST;
}


// ── Deslizamiento ─────────────────────────────────────────────────────────────
void Personaje::activarDesliz()
{
    if (enSuelo && !deslizando)
    {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;
    }
}

// ── actualizar genérico (usa nivel 1 por defecto) ─────────────────────────────
void Personaje::actualizar(float dt)
{
    // actualizarNivel1(dt, 0.f);
    actualizarNivel2(dt);
}

// ── Nivel 1 ───────────────────────────────────────────────────────────────────
void Personaje::actualizarNivel1(float dt, float tiempoTotal)
{
    // --- Velocidad horizontal según input ---
    float velHoriz = 0.f;
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    if (keys[0]) velHoriz = -velEfectiva;
    if (keys[1]) velHoriz =  velEfectiva;

    // Aplicar fricción solo si está en suelo y no hay input
    if (enSuelo && std::abs(velHoriz) < 0.01f)
    {
        GestorFisicas::aplicarFriccion(Vx, dt);
    }
    else
    {
        Vx = velHoriz;
    }

    // --- Viento oscilatorio (empuja horizontalmente) ---
    // Solo si está en el aire (el suelo amortigua el viento)
    if (!enSuelo)
        GestorFisicas::aplicarViento(Vx, tiempoTotal, dt);

    // --- Gravedad + posición vertical ---
    // El nivel resuelve colisiones DESPUÉS de aplicarGravedad
    GestorFisicas::aplicarGravedad(Vy, y, dt);

    // --- Posición horizontal ---
    x += Vx * dt;


    // --- Boost: descontar tiempo ---
    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f)
        {
            boostActivo = false;
            tiempoBoost = 0.f;
        }
    }

    // --- Sincronizar sprite ---
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Nivel 2 ───────────────────────────────────────────────────────────────────
// ── actualizarNivel2() CON ANIMACIONES ───────────────────────────────────────
void Personaje::actualizarNivel2(float dt)
{
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    // ── 1. MOVIMIENTO CON INERCIA ─────────────────────────────────────────────
    if (deslizando)
    {
        // Durante el desliz: inercia pura, sin input de dirección
        GestorFisicas::aplicarInercia(Vx, 0.f, dt);
        GestorFisicas::aplicarInercia(Vy, 0.f, dt);

        tiempoDesliz -= dt;
        if (tiempoDesliz <= 0.f)
        {
            deslizando   = false;
            tiempoDesliz = 0.f;
        }
    }
    else
    {
        // Movimiento normal: aplica inercia hacia la velocidad objetivo
        float velObjetivoX = 0.f;
        if (keys[0]) { velObjetivoX = -velEfectiva; miraDerecha = false; }
        if (keys[1]) { velObjetivoX =  velEfectiva; miraDerecha = true;  }
        GestorFisicas::aplicarInercia(Vx, velObjetivoX, dt);

        float velObjetivoY = 0.f;
        if (keys[2]) velObjetivoY = -velEfectiva;
        if (keys[3]) velObjetivoY =  velEfectiva;
        GestorFisicas::aplicarInercia(Vy, velObjetivoY, dt);
    }

    x += Vx * dt;
    y += Vy * dt;

    // ── 2. FACTOR DE SIGILO ───────────────────────────────────────────────────
    float velTotal = std::sqrt(Vx*Vx + Vy*Vy);
    factorSigilo   = (velTotal < velMax * 0.4f) ? 0.5f : 1.f;

    // ── 3. TIMER DE BOOST ─────────────────────────────────────────────────────
    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }

    // ── 4. DETERMINAR ESTADO DE ANIMACIÓN ────────────────────────────────────
    //  Prioridad: Boost > Desliz > Corriendo > Idle
    EstadoAnim nuevoEstado;

    if (boostActivo)
        nuevoEstado = EstadoAnim::BOOST;
    else if (deslizando)
        nuevoEstado = EstadoAnim::DESLIZANDO;
    else if (std::abs(Vx) > 10.f || std::abs(Vy) > 10.f)
        nuevoEstado = EstadoAnim::CORRIENDO;
    else
        nuevoEstado = EstadoAnim::IDLE;

    // Si cambió de estado, reinicia el frame para no quedar a mitad de animación
    if (nuevoEstado != estadoAnim)
    {
        estadoAnim  = nuevoEstado;
        frameActual = 0;
        tiempoFrame = 0.f;
    }

    // ── 5. SELECCIONAR EL VECTOR DE FRAMES ACTIVO ────────────────────────────
    QVector<QPixmap>* framesActivos = nullptr;

    switch (estadoAnim)
    {
    case EstadoAnim::IDLE:        framesActivos = &framesIdle;        break;
    case EstadoAnim::CORRIENDO:   framesActivos = &framesCorriendo;   break;
    case EstadoAnim::DESLIZANDO:  framesActivos = &framesDeslizando;  break;
    case EstadoAnim::BOOST:       framesActivos = &framesBoost;       break;
    }

    // ── 6. AVANZAR EL FRAME ───────────────────────────────────────────────────
    if (framesActivos && !framesActivos->isEmpty())
    {
        tiempoFrame += dt;
        if (tiempoFrame >= duracionFrame)
        {
            tiempoFrame  = 0.f;
            frameActual  = (frameActual + 1) % framesActivos->size();
        }

        // ── 7. APLICAR FRAME AL SPRITE (con flip horizontal) ─────────────────
        QPixmap frame = framesActivos->at(frameActual);

        // if (!miraDerecha)
        //     frame = frame.transformed(QTransform().scale(-1, 1));

        if (itemGrafico)
            itemGrafico->setPixmap(frame);
    }

    // ── 8. SINCRONIZAR POSICIÓN ───────────────────────────────────────────────
    if (itemGrafico)
        itemGrafico->setPos(x, y);
}




// ── Colisión con suelo/plataforma ─────────────────────────────────────────────
void Personaje::aterrizarEnSuelo(float suloY)
{
    y       = suloY - ALTO;
    Vy      = 0.f;
    enSuelo = true;
    saltosRestantes = 2;    // Recargar doble salto
    if (itemGrafico) itemGrafico->setPos(x, y);
}

void Personaje::despegarSuelo()
{
    enSuelo = false;
}

// ── cargarSprites() COMPLETO ──────────────────────────────────────────────────
//
//  INSTRUCCIONES: Ajusta SOLO los valores de origen (origenX, origenY)
//  y cantidad de frames (numFrames) para cada acción según tu spritesheet.
//  El resto del código no cambia.
//
void Personaje::cargarSprites()
{
    QPixmap sheet(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");

    if (sheet.isNull()) {
        qDebug() << "ERROR: No se pudo cargar la hoja de sprites.";
        return;
    }

    // ── Dimensiones de cada frame (igual para todas las acciones) ────────────
    const int frameW = 70;
    const int frameH = 70;

    // ── Lambda helper: recorta N frames horizontales desde (ox, oy) ──────────
    // Úsala para cada acción, solo cambia ox, oy y numFrames
    // auto recortar = [&](QVector<QPixmap>& destino, int ox, int oy, int numFrames)
    // {
    //     int separacion= 9; //separacion entre animaciones
    //     destino.clear();
    //     for (int i = 0; i < numFrames; i++)
    //     {
    //         int xFinal = ox + (i * (frameW+separacion));

    //         if (xFinal + frameW <= sheet.width() &&
    //             oy  + frameH <= sheet.height())
    //         {
    //             destino.append(sheet.copy(xFinal, oy, frameW, frameH));
    //         }
    //         else
    //         {
    //             qDebug() << "ERROR: Frame" << i << "fuera de la imagen en acción";
    //             QPixmap placeholder(frameW, frameH);
    //             placeholder.fill(Qt::magenta);   // magenta = error visible
    //             destino.append(placeholder);
    //         }
    //     }
    // };
    auto recortar = [&](QVector<QPixmap>& destino, int ox, int oy, int numFrames)
    {
        destino.clear();
        QColor fondoColor(0x31, 0x4d, 0x58); // color base del fondo
        int tolerancia = 5; // cubre las variaciones 314b58, 324a56, 2f4b57

        for (int i = 0; i < numFrames; i++)
        {
            int xFinal = ox + (i * (frameW + 8)); // +7 por la separación entre frames

            if (xFinal + frameW <= sheet.width() && oy + frameH <= sheet.height())
            {
                QPixmap frame = sheet.copy(xFinal, oy, frameW, frameH);
                destino.append(eliminarFondo(frame, fondoColor, tolerancia));
            }
            else
            {
                QPixmap placeholder(frameW, frameH);
                placeholder.fill(Qt::transparent);
                destino.append(placeholder);
            }
        }
    };

    // ── Carga cada acción ─────────────────────────────────────────────────────
    // TODO: Reemplaza los valores de origenX y origenY con los de tu spritesheet
    //       Puedes usar una herramienta como LibreSprite o simplemente
    //       abrir la imagen en paint y ver las coordenadas del pixel

    //             destino           origenX  origenY  numFrames
    recortar(framesIdle,         40,    136,     5);   // <-- ajusta tú
    recortar(framesCorriendo,    40,   136,   5);   // ya lo tenías
    recortar(framesDeslizando,   53,    335,     5);   // <-- ajusta tú
    recortar(framesBoost,        450,    331,     2);   // <-- ajusta tú

    // ── Estado inicial ────────────────────────────────────────────────────────
    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.1f;   // 10 FPS de animación, ajusta a gusto
    miraDerecha   = true;
}



QPixmap Personaje::eliminarFondo(const QPixmap& source, QColor colorFondo, int tolerancia)
{
    // Convertir a imagen para manipular píxel a píxel
    QImage img = source.toImage().convertToFormat(QImage::Format_ARGB32);

    int r = colorFondo.red();
    int g = colorFondo.green();
    int b = colorFondo.blue();

    for (int y = 0; y < img.height(); y++)
    {
        for (int x = 0; x < img.width(); x++)
        {
            QColor pixel = img.pixelColor(x, y);

            // Diferencia con el color de fondo
            int dr = std::abs(pixel.red()   - r);
            int dg = std::abs(pixel.green() - g);
            int db = std::abs(pixel.blue()  - b);

            // Si está dentro de la tolerancia, lo hace transparente
            if (dr <= tolerancia && dg <= tolerancia && db <= tolerancia)
                img.setPixelColor(x, y, Qt::transparent);
        }
    }

    return QPixmap::fromImage(img);
}



// ── Daño / reset ──────────────────────────────────────────────────────────────
void Personaje::recibirDanio(int cantidad)
{
    vidas -= cantidad;
    if (vidas < 0) vidas = 0;
}

void Personaje::resetearPosicion(float rx, float ry)
{
    x = rx; y = ry;
    Vx = 0; Vy = 0;
    enSuelo = false;
    saltosRestantes = 2;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ── Getters ───────────────────────────────────────────────────────────────────
int   Personaje::getVidas()        const { return vidas;        }
float Personaje::getEnergia()      const { return energia;      }
bool  Personaje::isEnSuelo()       const { return enSuelo;      }
bool  Personaje::isBoostActivo()   const { return boostActivo;  }
bool  Personaje::isDeslizando()    const { return deslizando;   }
float Personaje::getFactorSigilo() const { return factorSigilo; }
