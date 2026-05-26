#include "personaje.h"
#include "gestorfisicas.h"
#include <algorithm>
#include <cmath>

// ============================================================
//  Constructores
// ============================================================
Personaje::Personaje() : EntidadJuego(0, 0)
{
    vidas          = 3;
    energia        = 100.f;
    velMax         = 200.f;
    enSuelo        = false;
    std::fill(std::begin(keys), std::end(keys), false);

    puedeDoubleSalto = true;
    fuerzaSalto      = 460.f;
    saltosRestantes  = 2;

    tiempoViento     = 0.f;

    deslizando       = false;
    tiempoDesliz     = 0.f;
    boostActivo      = false;
    tiempoBoost      = 0.f;
    factorSigilo     = 1.f;

    Sprite           = nullptr;
    itemGrafico      = nullptr;

    estadoAnim       = EstadoAnim::IDLE;
    frameActual      = 0;
    tiempoFrame      = 0.f;
    duracionFrame    = 0.1f;
    miraDerecha      = true;
}

Personaje::Personaje(float X, float Y) : Personaje()
{
    x = X;
    y = Y;

    itemGrafico = new QGraphicsPixmapItem();

   //  // Intentar cargar sprites nivel 1; si falla, placeholder rojo
   //  cargarSpritesNivel1();





   //  if (n1_framesIdle.isEmpty())
   //  {
   //      QPixmap ph(static_cast<int>(ANCHO), static_cast<int>(ALTO));
   //      ph.fill(QColor(255, 80, 80));
   //      itemGrafico->setPixmap(ph);
   //  }
   //  else
   //  {
   //      itemGrafico->setPixmap(n1_framesIdle[0]);
   //  }


    //intentar caragr sprites para el nivel 2

    cargarSpritesNivel2();


     if (framesIdle.isEmpty())
     {
         QPixmap ph(static_cast<int>(ANCHO), static_cast<int>(ALTO));
         ph.fill(QColor(255, 80, 80));
         itemGrafico->setPixmap(ph);
     }
     else
     {
         itemGrafico->setPixmap(framesIdle[0]);
     }



    itemGrafico->setPos(x, y);
}

Personaje::~Personaje()
{
    delete Sprite;
}

// ============================================================
//  Entrada
// ============================================================
void Personaje::keyPressed(int key)
{
    if (key >= 0 && key < 4) keys[key] = true;
}

void Personaje::keyReleased(int key)
{
    if (key >= 0 && key < 4) keys[key] = false;
}

// ============================================================
//  Salto
// ============================================================
void Personaje::saltar()
{
    if (saltosRestantes <= 0) return;

    bool esDobleSalto = (saltosRestantes == 1);   // ya usó el primero

    Vy = -fuerzaSalto;
    enSuelo = false;
    saltosRestantes--;

    // Cambiar animación inmediatamente
    if (esDobleSalto)
    {
        estadoAnim  = EstadoAnim::DOBLE_SALTO;
        frameActual = 0;
        tiempoFrame = 0.f;
    }
    else
    {
        estadoAnim  = EstadoAnim::SALTANDO;
        frameActual = 0;
        tiempoFrame = 0.f;
    }

    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  Boost / Deslizamiento (Nivel 2)
// ============================================================
void Personaje::activarBoost()
{
    boostActivo = true;
    tiempoBoost = DURACION_BOOST;
}

void Personaje::activarDesliz()
{
    if (enSuelo && !deslizando)
    {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;
    }
}

// ============================================================
//  actualizar genérico
// ============================================================
void Personaje::actualizar(float dt)
{
    actualizarNivel2(dt);
}

// ============================================================
//  actualizarNivel1
// ============================================================
void Personaje::actualizarNivel1(float dt, float tiempoTotal)
{
    // ── 1. Velocidad horizontal ───────────────────────────────
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;
    float velHoriz    = 0.f;

    if (keys[0]) { velHoriz = -velEfectiva; miraDerecha = false; }
    if (keys[1]) { velHoriz =  velEfectiva; miraDerecha = true;  }

    if (enSuelo && std::abs(velHoriz) < 0.01f)
        GestorFisicas::aplicarFriccion(Vx, dt);
    else
        Vx = velHoriz;

    // ── 2. Viento (solo en el aire) ───────────────────────────
    float fuerzaViento = 0.f;
    if (!enSuelo)
    {
        fuerzaViento = GestorFisicas::calcularFuerzaViento(tiempoTotal);
        GestorFisicas::aplicarViento(Vx, tiempoTotal, dt);
        tiempoViento += dt;
    }
    else
    {
        tiempoViento = 0.f;
    }

    // ── 3. Gravedad + posición vertical ───────────────────────
    GestorFisicas::aplicarGravedad(Vy, y, dt);

    // ── 4. Posición horizontal ────────────────────────────────
    x += Vx * dt;

    // ── 5. Timer de boost ─────────────────────────────────────
    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }

    // ── 6. Determinar estado de animación ─────────────────────
    EstadoAnim nuevoEstado;

    bool vientoFuerte = (std::abs(fuerzaViento) > GestorFisicas::VIENTO_AMPLITUD * 0.6f)
                        && (tiempoViento > UMBRAL_VIENTO);

    if (!enSuelo && vientoFuerte)
        nuevoEstado = EstadoAnim::VIENTO_CAIDA;
    else if (!enSuelo && estadoAnim == EstadoAnim::DOBLE_SALTO && frameActual < n1_framesDobleSalto.size() - 1)
        nuevoEstado = EstadoAnim::DOBLE_SALTO;   // mantener hasta que termine
    else if (!enSuelo && estadoAnim == EstadoAnim::SALTANDO && Vy < 0.f)
        nuevoEstado = EstadoAnim::SALTANDO;
    else if (!enSuelo)
        nuevoEstado = EstadoAnim::COLISION;      // cayendo sin control
    else if (std::abs(Vx) > 10.f)
        nuevoEstado = EstadoAnim::CORRIENDO;
    else
        nuevoEstado = EstadoAnim::IDLE;

    if (nuevoEstado != estadoAnim)
    {
        estadoAnim  = nuevoEstado;
        frameActual = 0;
        tiempoFrame = 0.f;
    }

    // ── 7. Seleccionar frames activos y avanzar animación ─────
    QVector<QPixmap>* frames = nullptr;
    switch (estadoAnim)
    {
    case EstadoAnim::IDLE:         frames = &n1_framesIdle;        break;
    case EstadoAnim::CORRIENDO:    frames = &n1_framesCorriendo;   break;
    case EstadoAnim::SALTANDO:     frames = &n1_framesSaltando;    break;
    case EstadoAnim::DOBLE_SALTO:  frames = &n1_framesDobleSalto;  break;
    case EstadoAnim::VIENTO_CAIDA: frames = &n1_framesVientoCalda; break;
    case EstadoAnim::COLISION:     frames = &n1_framesColision;    break;
    default:                       frames = &n1_framesIdle;        break;
    }

    bool loop = (estadoAnim != EstadoAnim::DOBLE_SALTO); // doble salto no looea
    if (frames) tickAnimacion(dt, *frames, loop);

    // ── 8. Flip horizontal ────────────────────────────────────
    // (Aplicado dentro de tickAnimacion)

    // ── 9. Sincronizar posición gráfica ───────────────────────
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  actualizarNivel2
// ============================================================
void Personaje::actualizarNivel2(float dt)
{
    float velEfectiva = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    if (deslizando)
    {
        GestorFisicas::aplicarInercia(Vx, 0.f, dt);
        GestorFisicas::aplicarInercia(Vy, 0.f, dt);
        tiempoDesliz -= dt;
        if (tiempoDesliz <= 0.f) { deslizando = false; tiempoDesliz = 0.f; }
    }
    else
    {
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

    float velTotal = std::sqrt(Vx*Vx + Vy*Vy);
    factorSigilo   = (velTotal < velMax * 0.4f) ? 0.5f : 1.f;

    if (boostActivo)
    {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }

    // Animación nivel 2
    EstadoAnim nuevoEstado;
    if (boostActivo)
        nuevoEstado = EstadoAnim::BOOST;
    else if (deslizando)
        nuevoEstado = EstadoAnim::DESLIZANDO;
    else if (std::abs(Vx) > 10.f || std::abs(Vy) > 10.f)
        nuevoEstado = EstadoAnim::CORRIENDO;
    else
        nuevoEstado = EstadoAnim::IDLE;

    if (nuevoEstado != estadoAnim)
    {
        estadoAnim  = nuevoEstado;
        frameActual = 0;
        tiempoFrame = 0.f;
    }

    QVector<QPixmap>* frames = nullptr;
    switch (estadoAnim)
    {
    case EstadoAnim::IDLE:       frames = &framesIdle;       break;
    case EstadoAnim::CORRIENDO:  frames = &framesCorriendo;  break;
    case EstadoAnim::DESLIZANDO: frames = &framesDeslizando; break;
    case EstadoAnim::BOOST:      frames = &framesBoost;      break;
    default:                     frames = &framesIdle;       break;
    }

    if (frames) tickAnimacion(dt, *frames, true);

    if (itemGrafico) itemGrafico->setPos(x, y);
}













// ============================================================
//  tickAnimacion  — avanza frame y aplica al item gráfico
// ============================================================
void Personaje::tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop)
{
    if (frames.isEmpty() || !itemGrafico) return;

    tiempoFrame += dt;
    if (tiempoFrame >= duracionFrame)
    {
        tiempoFrame = 0.f;
        if (loop)
            frameActual = (frameActual + 1) % frames.size();
        else
            frameActual = std::min(frameActual + 1, (int)frames.size() - 1);
    }

    QPixmap frame = frames[frameActual];

    if (!miraDerecha)
        frame = frame.transformed(QTransform().scale(-1, 1));

    itemGrafico->setPixmap(frame);
}

// ============================================================
//  Colisión con suelo
// ============================================================
void Personaje::aterrizarEnSuelo(float /*suloY*/)
{
    // La posición Y ya fue corregida por resolverColisiones()
    Vy      = 0.f;
    enSuelo = true;
    saltosRestantes = 2;
    tiempoViento    = 0.f;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

void Personaje::despegarSuelo()
{
    enSuelo = false;
}

// ============================================================
//  cargarSpritesNivel1
//  Spritesheet: ":/Kael_nivel1/Sprites/Nivel1/sprites nivel 1 kael.png"
//
//  Diseño observado en la imagen (aprox. 1400×730 px):
//  ┌─────────────────────────────────────────────────────┐
//  │ Fila 1 (y≈55):  IDLE(1f) + CORRER(8f)              │
//  │ Fila 2 (y≈230): SALTAR(5f)      | DOBLE SALTO(5f)  │
//  │ Fila 3 (y≈430): VIENTO CAIDA(5f)| COLISION(4f)     │
//  └─────────────────────────────────────────────────────┘
//
//  IMPORTANTE: estos valores son aproximados a partir del
//  spritesheet de diseño. Ajústalos con un editor de imágenes
//  si los frames no coinciden exactamente.
// ============================================================
void Personaje::cargarSpritesNivel1()
{
    QPixmap sheet(":/Kael_nivel1/Sprites/Nivel1/sprites nivel 1 kael.png");
    if (sheet.isNull())
    {
        qDebug() << "ERROR Nivel1: No se pudo cargar sprites nivel 1 kael.png";
        return;
    }

    qDebug() << "Spritesheet N1 cargada:" << sheet.width() << "x" << sheet.height();

    // ── Dimensiones de cada frame ────────────────────────────
    const int FW = 120;   // ancho de cada frame en la hoja
    const int FH = 160;   // alto  de cada frame en la hoja
    const int SEP = 0;    // separación horizontal entre frames

    // Color de fondo a eliminar (ajustar según la imagen real)
    QColor fondoColor(0x0d, 0x0e, 0x1a);
    int    tolerancia = 12;

    // Lambda recortar
    auto recortar = [&](QVector<QPixmap>& dest, int ox, int oy, int nFrames)
    {
        dest.clear();
        for (int i = 0; i < nFrames; ++i)
        {
            int sx = ox + i * (FW + SEP);
            if (sx + FW <= sheet.width() && oy + FH <= sheet.height())
            {
                QPixmap f = sheet.copy(sx, oy, FW, FH);
                // Escalar al tamaño lógico del personaje
                f = f.scaled(static_cast<int>(ANCHO),
                             static_cast<int>(ALTO),
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
                dest.append(eliminarFondo(f, fondoColor, tolerancia));
            }
            else
            {
                QPixmap ph(static_cast<int>(ANCHO), static_cast<int>(ALTO));
                ph.fill(Qt::transparent);
                dest.append(ph);
                qDebug() << "WARN N1: frame" << i << "fuera de la hoja en ox=" << ox << "oy=" << oy;
            }
        }
    };

    // ── Cargar cada animación ─────────────────────────────────
    //   Ajusta ox/oy según tu spritesheet real
    //   (Valores basados en la imagen de diseño proporcionada)

    // Fila 1 — IDLE: 1 frame al inicio, luego CORRER 8 frames
    recortar(n1_framesIdle,      0,   55, 1);
    recortar(n1_framesCorriendo, 130, 55, 8);

    // Fila 2 — SALTAR (5f) a la izquierda, DOBLE SALTO (5f) a la derecha
    recortar(n1_framesSaltando,   0,  230, 5);
    recortar(n1_framesDobleSalto, 720, 230, 5);

    // Fila 3 — VIENTO CAIDA (5f) + impacto (1f) | COLISION (4f)
    recortar(n1_framesVientoCalda, 0,   430, 5);
    recortar(n1_framesColision,    720, 430, 4);

    // Estado inicial
    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.08f;   // ~12 FPS de animación
    miraDerecha   = true;

    qDebug() << "N1 sprites cargados — Idle:" << n1_framesIdle.size()
             << "Correr:" << n1_framesCorriendo.size()
             << "Saltar:" << n1_framesSaltando.size()
             << "DobleSalto:" << n1_framesDobleSalto.size()
             << "Viento:" << n1_framesVientoCalda.size()
             << "Colision:" << n1_framesColision.size();
}

// ============================================================
//  cargarSpritesNivel2  (igual que antes)
// ============================================================
void Personaje::cargarSpritesNivel2()
{
    // QPixmap sheet(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");
    QPixmap sheet (":/Kael_nivel2/Sprites/Nivel2/Sprites_kael_movimientos.png");
    if (sheet.isNull())
    {
        qDebug() << "ERROR: No se pudo cargar la hoja de sprites.";
        return;
    }

    const int frameW = 70;
    const int frameH = 70;

    // ── Lambda: recorta N frames y elimina AMBOS colores de fondo ────────────
    //
    //  Fondo 1 → #314d58  (animaciones idle / corriendo / boost)
    //  Fondo 2 → #0e1527  (animación deslizando)
    //
    //  Se aplican en cadena: un pixel sólo necesita coincidir con UNO de los dos
    //  fondos para volverse transparente, así se limpia toda la hoja por igual.
    // ─────────────────────────────────────────────────────────────────────────
    auto recortar = [&](QVector<QPixmap>& destino, int ox, int oy, int numFrames)
    {
        destino.clear();
        for (int i = 0; i < numFrames; i++)
        {
            int xFinal = ox + i * (frameW + 8);   // 8 px de separación entre frames

            QPixmap frame;
            if (xFinal + frameW <= sheet.width() && oy + frameH <= sheet.height())
            {
                frame = sheet.copy(xFinal, oy, frameW, frameH);
            }
            else
            {
                qDebug() << "WARN: frame" << i << "fuera de la imagen";
                frame = QPixmap(frameW, frameH);
                frame.fill(Qt::transparent);
                destino.append(frame);
                continue;
            }

            // ── Eliminar fondo normal (#314d58) ──────────────────────────────
            frame = eliminarFondo(frame, QColor(0x31, 0x4d, 0x58), 8);
            // ── Eliminar fondo desliz (#0e1527) ──────────────────────────────
            frame = eliminarFondo(frame, QColor(0x0e, 0x15, 0x27), 8);

            destino.append(frame);
        }
    };

    //             destino           origenX  origenY  numFrames
    recortar(framesIdle,         40,   140,    4);
    recortar(framesCorriendo,    40,   336,    8);   // ajusta origenX/Y si están en otra fila
    recortar(framesDeslizando,   40,   515,    4);
    recortar(framesBoost,       450,   331,    2);

    // ── Estado inicial ────────────────────────────────────────────────────────
    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.1f;
    miraDerecha   = true;

    // ── Aplicar primer frame y fijar el pivote de rotación en el centro ──────
    // Imprescindible para que la rotación en actualizarNivel2 gire alrededor
    // del centro del sprite en vez de la esquina superior-izquierda.
    if (itemGrafico)
    {
        if (!framesIdle.isEmpty())
            itemGrafico->setPixmap(framesIdle.at(0));

        itemGrafico->setTransformOriginPoint(frameW / 2.0, frameH / 2.0);
    }
}

// ============================================================
//  eliminarFondo  — hace transparente el color de fondo
// ============================================================
QPixmap Personaje::eliminarFondo(const QPixmap& source, QColor colorFondo, int tolerancia)
{
    QImage img = source.toImage().convertToFormat(QImage::Format_ARGB32);
    int r = colorFondo.red(), g = colorFondo.green(), b = colorFondo.blue();

    for (int py = 0; py < img.height(); ++py)
    {
        for (int px = 0; px < img.width(); ++px)
        {
            QColor pixel = img.pixelColor(px, py);
            if (std::abs(pixel.red()   - r) <= tolerancia &&
                std::abs(pixel.green() - g) <= tolerancia &&
                std::abs(pixel.blue()  - b) <= tolerancia)
            {
                img.setPixelColor(px, py, Qt::transparent);
            }
        }
    }
    return QPixmap::fromImage(img);
}

// ============================================================
//  Daño / reset
// ============================================================
void Personaje::recibirDanio(int cantidad)
{
    vidas -= cantidad;
    if (vidas < 0) vidas = 0;
}

void Personaje::resetearPosicion(float rx, float ry)
{
    x = rx; y = ry;
    Vx = 0.f; Vy = 0.f;
    enSuelo         = false;
    saltosRestantes = 2;
    tiempoViento    = 0.f;
    estadoAnim      = EstadoAnim::IDLE;
    frameActual     = 0;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  Getters
// ============================================================
int   Personaje::getVidas()        const { return vidas;       }
float Personaje::getEnergia()      const { return energia;     }
bool  Personaje::isEnSuelo()       const { return enSuelo;     }
bool  Personaje::isBoostActivo()   const { return boostActivo; }
bool  Personaje::isDeslizando()    const { return deslizando;  }
float Personaje::getFactorSigilo() const { return factorSigilo;}
