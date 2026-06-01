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





   //  if (n1_framesIdle.empty())
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


     if (framesIdle.empty())
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


void Personaje::activarDeslizNivel2()
{
    float velTotal = std::sqrt(Vx*Vx + Vy*Vy);
    bool  estaCorreindo = velTotal > velMax * 0.3f;

    if (estaCorreindo && !deslizando)
    {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;

        // Impulso en la dirección actual de movimiento
        float nx = Vx / velTotal;
        float ny = Vy / velTotal;
        Vx = nx * velMax * 1.5f;
        Vy = ny * velMax * 1.5f;
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
    // switch (estadoAnim)
    // {
    // case EstadoAnim::IDLE:         frames = &n1_framesIdle;        break;
    // case EstadoAnim::CORRIENDO:    frames = &n1_framesCorriendo;   break;
    // case EstadoAnim::SALTANDO:     frames = &n1_framesSaltando;    break;
    // case EstadoAnim::DOBLE_SALTO:  frames = &n1_framesDobleSalto;  break;
    // case EstadoAnim::VIENTO_CAIDA: frames = &n1_framesVientoCalda; break;
    // case EstadoAnim::COLISION:     frames = &n1_framesColision;    break;
    // default:                       frames = &n1_framesIdle;        break;
    // }

    bool loop = (estadoAnim != EstadoAnim::DOBLE_SALTO); // doble salto no looea
    //if (frames) tickAnimacion(dt, *frames, loop);

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
        //qDebug("deslizando");
        GestorFisicas::aplicarInercia(Vx, 0.f, dt);
        GestorFisicas::aplicarInercia(Vy, 0.f, dt);
        tiempoDesliz -= dt;
        if (tiempoDesliz <= 0.f) { deslizando = false; tiempoDesliz = 0.f; }
    }
    else
    {
        //qDebug("no deslizando");
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
        //qDebug("boost activado");
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }


    // Animación nivel 2

    EstadoAnim nuevoEstado;

    if (deslizando)
        nuevoEstado = EstadoAnim::DESLIZANDO;
    else if (velTotal > 10.f)
    {
        if (std::abs(Vy) > std::abs(Vx))
        {
            // Movimiento principalmente vertical
            if (Vy < 0.f)
                nuevoEstado = EstadoAnim::CORRIENDO_ARRIBA;
            else
                nuevoEstado = EstadoAnim::CORRIENDO_ABAJO;
        }
        else
            nuevoEstado = EstadoAnim::CORRIENDO;  // horizontal dominante
    }
    else
        nuevoEstado = EstadoAnim::IDLE;

    if (nuevoEstado != estadoAnim)   // solo si realmente cambia
    {
        frameActual = 0;
        tiempoFrame = 0.f;
    }
    estadoAnim = nuevoEstado;

    std::vector<QPixmap>* frames = nullptr;
    float multAnim = 1.0f;

    switch (estadoAnim)
    {
    case EstadoAnim::IDLE:
        frames = &framesIdle;
        break;
    case EstadoAnim::CORRIENDO:
        frames = &framesCorriendo;
        break;
    case EstadoAnim::CORRIENDO_ARRIBA:
        frames   = &framesUprun;
        multAnim = boostActivo ? 0.4f : 0.7f;   // boost → aún más rápido
        break;
    case EstadoAnim::CORRIENDO_ABAJO:
        frames   = &framesIdle;
        multAnim = boostActivo ? 0.4f : 0.7f;
        break;
    case EstadoAnim::DESLIZANDO:
        frames = &framesDeslizando;
        break;
    default:
        frames = &framesIdle;
        break;
    }


    // Calcular multiplicador: si va principalmente hacia arriba/abajo → más lento
    // Así los mismos sprites de correr se ven diferente en movimiento vertical

    if (velTotal > 10.f && std::abs(Vy) > std::abs(Vx))
        multAnim = 1.8f;   // ~45% más lento — ajusta este valor a tu gusto




    if (frames) tickAnimacion(dt, *frames, true, multAnim);

    if (itemGrafico) itemGrafico->setPos(x, y);


}













// ============================================================
//  tickAnimacion  — avanza frame y aplica al item gráfico
// ============================================================
// void Personaje::tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop)
// {
//     if (frames.empty() || !itemGrafico) return;

//     tiempoFrame += dt;
//     if (tiempoFrame >= duracionFrame)
//     {
//         tiempoFrame = 0.f;
//         if (loop)
//             frameActual = (frameActual + 1) % frames.size();
//         else
//             frameActual = std::min(frameActual + 1, (int)frames.size() - 1);
//     }

//     QPixmap frame = frames[frameActual];

//     if (!miraDerecha)
//         frame = frame.transformed(QTransform().scale(-1, 1));

//     itemGrafico->setPixmap(frame);
// }


void Personaje::tickAnimacion(float dt, std::vector<QPixmap>& frames,
                              bool loop, float multVelocidad)
{

    if (frames.empty() || !itemGrafico) return;

    tiempoFrame += dt;

    // multVelocidad > 1 → cada frame dura más → animación más lenta
    float duracionEfectiva = duracionFrame * multVelocidad;

    if (tiempoFrame >= duracionEfectiva)
    {
        tiempoFrame = 0.f;
        if (loop)
            frameActual = (frameActual + 1) % frames.size();
        else
            frameActual = std::min(frameActual + 1, (int)frames.size() - 1);
    }

    QPixmap frame = frames[frameActual];

    // Flip horizontal (izquierda/derecha)
    if (!miraDerecha)
        frame = frame.transformed(QTransform().scale(-1, 1));

    // ── Tinte visual de boost ─────────────────────────────────────────────
    // Pinta un overlay cian semitransparente encima del sprite.
    // Solo afecta los píxeles no transparentes (CompositionMode_SourceAtop).
    // Cambia QColor(0, 200, 255, 90) por el color que prefieras.
    if (boostActivo)
    {
        //qdebug("Intentando boost");
        QPixmap tinted(frame.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, frame);                          // sprite original
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.fillRect(frame.rect(), QColor(0, 200, 255, 60)); // tinte cian
        p.end();
        frame = tinted;
        //qdebug("boost hecho");
    }

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
    if (sheet.isNull()) {
        qDebug() << "ERROR: No se cargo sprites nivel 1 kael.png";
        return;
    }
    qDebug() << "Spritesheet N1:" << sheet.width() << "x" << sheet.height();

    const int TW = static_cast<int>(ANCHO);  // 70
    const int TH = static_cast<int>(ALTO);   // 70

    // Recorta un frame del sheet, elimina fondo negro y escala a TW x TH
    auto extraer = [&](int x1, int y1, int w, int h) -> QPixmap
    {
        if (x1<0||y1<0||x1+w>sheet.width()||y1+h>sheet.height()) {
            QPixmap ph(TW,TH); ph.fill(Qt::transparent); return ph;
        }
        QImage img = sheet.copy(x1,y1,w,h).toImage()
                         .convertToFormat(QImage::Format_ARGB32);
        for (int py=0; py<img.height(); ++py)
            for (int px=0; px<img.width(); ++px) {
                QColor c = img.pixelColor(px,py);
                if (c.red()<15 && c.green()<15 && c.blue()<15)
                    img.setPixelColor(px,py,Qt::transparent);
            }
        return QPixmap::fromImage(img)
            .scaled(TW,TH,Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
    };

    // ── FILA 1 (y=44 h=65): SPRINT ───────────────────────────
    // 8 grupos detectados:
    // Grupo 1: x=17  w=36  → IDLE (estático)
    // Grupos 2-8: x=86,148,206,269,346,402,490 → frames corriendo
    n1_framesIdle.clear();
    n1_framesIdle.append(extraer(17, 44, 36, 65));

    n1_framesCorriendo.clear();
    n1_framesCorriendo.append(extraer( 86, 44,  42, 65));
    n1_framesCorriendo.append(extraer(148, 44,  44, 65));
    n1_framesCorriendo.append(extraer(206, 44,  54, 65));
    n1_framesCorriendo.append(extraer(269, 44,  61, 65));
    n1_framesCorriendo.append(extraer(346, 44,  55, 65));
    n1_framesCorriendo.append(extraer(402, 44,  64, 65));
    n1_framesCorriendo.append(extraer(490, 44,  51, 65));

    // ── FILA 2 (y=110 h=98): SALTO ───────────────────────────
    // 9 grupos detectados:
    // Grupo 1: x=18   w=38  → preparación
    // Grupo 2: x=70   w=173 → arco (pegados, dividir en 3x57)
    // Grupo 3: x=251  w=40  → apex
    // Grupo 4: x=322  w=33  → caída f1
    // Grupo 5: x=371  w=34  → caída f2
    // Grupo 6: x=418  w=38  → caída f3
    // Grupo 7: x=462  w=89  → doble salto flash (dividir en 2x44)
    // Grupo 8: x=552  w=43  → post doble salto
    // Grupo 9: x=605  w=39  → aterrizaje
    // n1_framesSaltando.clear();
    // n1_framesSaltando.append(extraer( 18, 110, 38, 98));  // prep
    // n1_framesSaltando.append(extraer( 70, 110, 57, 98));  // arco f1
    // n1_framesSaltando.append(extraer(127, 110, 58, 98));  // arco f2
    // n1_framesSaltando.append(extraer(185, 110, 58, 98));  // arco f3
    // n1_framesSaltando.append(extraer(251, 110, 40, 98));  // apex
    // n1_framesSaltando.append(extraer(322, 110, 33, 98));  // caída f1
    // n1_framesSaltando.append(extraer(371, 110, 34, 98));  // caída f2
    // n1_framesSaltando.append(extraer(418, 110, 38, 98));  // caída f3
    // n1_framesSaltando.append(extraer(462, 110, 44, 98));  // doble f1
    // n1_framesSaltando.append(extraer(506, 110, 45, 98));  // doble f2
    // n1_framesSaltando.append(extraer(552, 110, 43, 98));  // post
    // n1_framesSaltando.append(extraer(605, 110, 39, 98));  // aterrizaje

    // ── FILA 3 (y=214 h=66): VIENTO — OMITIDA ────────────────
    //n1_framesVientoCalda.clear();  // vacío hasta implementar ventilador

    // ── FILA 4 (y=294 h=59): CAÍDA FINAL ─────────────────────
    // Solo los 3 primeros grupos son del personaje.
    // Grupos 4-7 son obstáculos/UI del nivel — ignorar.
    // Grupo 1: x=18  w=60
    // Grupo 2: x=96  w=60
    // Grupo 3: x=180 w=67
    // n1_framesCaidaFinal.clear();
    // n1_framesCaidaFinal.append(extraer( 18, 294, 60, 59));
    // n1_framesCaidaFinal.append(extraer( 96, 294, 60, 59));
    // n1_framesCaidaFinal.append(extraer(180, 294, 67, 59));

    // ── Estado inicial ─────────────────────────────────────────
    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.09f;
    miraDerecha   = true;

    // qDebug() << "N1 OK:"
    //          << "Idle:"       << n1_framesIdle.size()
    //          << "Correr:"     << n1_framesCorriendo.size()
    //          << "Salto:"      << n1_framesSaltando.size()
    //          << "CaidaFinal:" << n1_framesCaidaFinal.size();
}

// ============================================================
//  cargarSpritesNivel2  (igual que antes)
// ============================================================
void Personaje::cargarSpritesNivel2()
{
    QPixmap sheet(":/Kael_nivel2/Sprites/Nivel2/Sprites_kael_movimientos.png");
    if (sheet.isNull())
    {
        qDebug() << "ERROR: No se pudo cargar la hoja de sprites.";
        return;
    }

    // ── Lambda genérico: acepta cualquier tamaño de frame y separación ───────
    auto recortar = [&](std::vector<QPixmap>& destino,
                        int ox, int oy,
                        int numFrames,
                        int fw, int fh,        // ← ancho y alto del frame
                        int separacion = 10)   // ← separación entre frames (default 10px)
    {
        destino.clear();
        for (int i = 0; i < numFrames; i++)
        {
            int xFinal = ox + i * (fw + separacion);

            QPixmap frame;
            if (xFinal + fw <= sheet.width() && oy + fh <= sheet.height())
            {
                frame = sheet.copy(xFinal, oy, fw, fh);
            }
            else
            {
                qDebug() << "WARN: frame" << i << "fuera de la imagen";
                frame = QPixmap(fw, fh);
                frame.fill(Qt::transparent);
                destino.push_back(frame);
                continue;
            }

            frame = eliminarFondo(frame, QColor(0x31, 0x4d, 0x58), 8);
            frame = eliminarFondo(frame, QColor(0x0e, 0x15, 0x27), 8);
            destino.push_back(frame);
        }
    };

    //            destino           ox   oy   frames  fw   fh   sep
    recortar(framesIdle,            42, 140,    4,    93, 109,  10);
    recortar(framesCorriendo,       45, 338,    8,    66,  86,  15);
    recortar(framesDeslizando,      41, 516,    4,    89, 97,  10);
    recortar(framesUprun,      725, 314,    9,    60, 105,  12);             //la espalda del personaje
    recortar(framesDownrun,         320, 331,    2,    93, 109,  10);      //el frente del personaje, otro datasheet





    // ── Estado inicial ────────────────────────────────────────────────────────
    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.1f;
    miraDerecha   = true;

    if (itemGrafico && !framesIdle.empty())
    {
        itemGrafico->setPixmap(framesIdle.at(0));
        // Pivote en el centro del primer frame cargado
        itemGrafico->setTransformOriginPoint(
            framesIdle.at(0).width()  / 2.0,
            framesIdle.at(0).height() / 2.0
            );
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

    std::fill(std::begin(keys), std::end(keys), false);
    deslizando   = false;
    boostActivo  = false;
    tiempoDesliz = 0.f;
    tiempoBoost  = 0.f;
    if (itemGrafico) itemGrafico->setPos(x, y);
}


void Personaje::setHitboxOffset(float offsetX,float offsetY, float anchoEfectivo ,float altoEfectivo) {
    hitboxOffsetY  = offsetY;
    hitboxOffsetX  = offsetX;
    hitboxAltoReal = altoEfectivo;
    hitboxAnchoReal = anchoEfectivo;
    ANCHO  = anchoEfectivo;
    ALTO  = altoEfectivo;
    // x = offsetX;
    // y = offsetY;


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
