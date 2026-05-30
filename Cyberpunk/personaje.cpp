#include "personaje.h"
#include "gestorfisicas.h"
#include <algorithm>
#include <cmath>

// ============================================================
//  Constructor base
// ============================================================
Personaje::Personaje() : EntidadJuego(0, 0)
{
    vidas            = 3;
    energia          = 100.f;
    velMax           = 200.f;
    enSuelo          = false;
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
    enCaidaFinal     = false;
    plataformasCalda = 0;
    yMasAlta         = 0.f;
}

// ============================================================
//  Constructor con posición — carga sprites nivel 1 por defecto
// ============================================================
Personaje::Personaje(float X, float Y) : Personaje()
{
    x = X; y = Y;
    yMasAlta = Y;
    itemGrafico = new QGraphicsPixmapItem();

    cargarSpritesNivel1();

    QPixmap inicial = n1_framesIdle.isEmpty()
        ? []{ QPixmap p(70,70); p.fill(Qt::red); return p; }()
        : n1_framesIdle[0];
    itemGrafico->setPixmap(inicial);
    itemGrafico->setPos(x, y);
}

Personaje::~Personaje() { delete Sprite; }

// ============================================================
//  Entrada
// ============================================================
void Personaje::keyPressed (int key) { if(key>=0&&key<4) keys[key]=true;  }
void Personaje::keyReleased(int key) { if(key>=0&&key<4) keys[key]=false; }

// ============================================================
//  Saltar
// ============================================================
void Personaje::saltar()
{
    if (saltosRestantes <= 0) return;
    Vy = -fuerzaSalto;
    enSuelo = false;
    saltosRestantes--;
    estadoAnim  = EstadoAnim::SALTANDO;
    frameActual = 0;
    tiempoFrame = 0.f;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  Boost / Deslizamiento
// ============================================================
void Personaje::activarBoost()
{
    boostActivo = true;
    tiempoBoost = DURACION_BOOST;
}

void Personaje::activarDesliz()
{
    // Nivel 1: solo si está en suelo
    if (enSuelo && !deslizando) {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;
    }
}

void Personaje::activarDeslizNivel2()
{
    // Nivel 2: en movimiento, con impulso en la dirección actual
    float velTotal = std::sqrt(Vx*Vx + Vy*Vy);
    if (velTotal > velMax * 0.3f && !deslizando) {
        deslizando   = true;
        tiempoDesliz = DURACION_DESLIZ_MAX;
        float nx = Vx / velTotal;
        float ny = Vy / velTotal;
        Vx = nx * velMax * 1.5f;
        Vy = ny * velMax * 1.5f;
    }
}

void Personaje::actualizar(float dt) { actualizarNivel2(dt); }

// ============================================================
//  actualizarNivel1
// ============================================================
void Personaje::actualizarNivel1(float dt, float tiempoTotal)
{
    // ── Caída final: bloquear física, solo animar ─────────────
    if (enCaidaFinal) {
        tickAnimacion(dt, n1_framesCaidaFinal, false);
        if (itemGrafico) itemGrafico->setPos(x, y);
        return;
    }

    // ── Movimiento horizontal ─────────────────────────────────
    float velHoriz = 0.f;
    if (keys[0]) { velHoriz = -velMax; miraDerecha = false; }
    if (keys[1]) { velHoriz =  velMax; miraDerecha = true;  }

    if (enSuelo && std::abs(velHoriz) < 0.01f)
        GestorFisicas::aplicarFriccion(Vx, dt);
    else
        Vx = velHoriz;

    // ── Viento (solo en el aire) ──────────────────────────────
    if (!enSuelo) {
        GestorFisicas::aplicarViento(Vx, tiempoTotal, dt);
        tiempoViento += dt;
    } else {
        tiempoViento = 0.f;
    }

    // ── Gravedad + posición ───────────────────────────────────
    GestorFisicas::aplicarGravedad(Vy, y, dt);
    x += Vx * dt;

    // ── Registrar posición más alta alcanzada ─────────────────
    if (y < yMasAlta) yMasAlta = y;

    // ── Estado de animación ───────────────────────────────────
    EstadoAnim nuevo;
    if (!enSuelo)
        nuevo = EstadoAnim::SALTANDO;
    else if (std::abs(Vx) > 10.f)
        nuevo = EstadoAnim::CORRIENDO;
    else
        nuevo = EstadoAnim::IDLE;

    if (nuevo != estadoAnim) { estadoAnim=nuevo; frameActual=0; tiempoFrame=0.f; }

    QVector<QPixmap>* frames = nullptr;
    switch (estadoAnim) {
    case EstadoAnim::IDLE:      frames = &n1_framesIdle;      break;
    case EstadoAnim::CORRIENDO: frames = &n1_framesCorriendo; break;
    case EstadoAnim::SALTANDO:  frames = &n1_framesSaltando;  break;
    default:                    frames = &n1_framesIdle;      break;
    }
    if (frames) tickAnimacion(dt, *frames, true);
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  actualizarNivel2
// ============================================================
void Personaje::actualizarNivel2(float dt)
{
    float ve = boostActivo ? velMax * MULTIPLICADOR_BOOST : velMax;

    if (deslizando) {
        GestorFisicas::aplicarInercia(Vx, 0.f, dt);
        GestorFisicas::aplicarInercia(Vy, 0.f, dt);
        tiempoDesliz -= dt;
        if (tiempoDesliz <= 0.f) { deslizando = false; tiempoDesliz = 0.f; }
    } else {
        float vox=0.f, voy=0.f;
        if (keys[0]) { vox = -ve; miraDerecha = false; }
        if (keys[1]) { vox =  ve; miraDerecha = true;  }
        if (keys[2]) voy = -ve;
        if (keys[3]) voy =  ve;
        GestorFisicas::aplicarInercia(Vx, vox, dt);
        GestorFisicas::aplicarInercia(Vy, voy, dt);
    }

    x += Vx * dt;
    y += Vy * dt;

    float vt = std::sqrt(Vx*Vx + Vy*Vy);
    factorSigilo = (vt < velMax * 0.4f) ? 0.5f : 1.f;

    if (boostActivo) {
        tiempoBoost -= dt;
        if (tiempoBoost <= 0.f) { boostActivo = false; tiempoBoost = 0.f; }
    }

    // ── Estado de animación nivel 2 ───────────────────────────
    EstadoAnim nuevo;
    if (deslizando)
        nuevo = EstadoAnim::DESLIZANDO;
    else if (vt > 10.f) {
        if (std::abs(Vy) > std::abs(Vx)) {
            nuevo = (Vy < 0.f) ? EstadoAnim::CORRIENDO_ARRIBA
                                : EstadoAnim::CORRIENDO_ABAJO;
        } else {
            nuevo = EstadoAnim::CORRIENDO;
        }
    } else {
        nuevo = EstadoAnim::IDLE;
    }

    if (nuevo != estadoAnim) { frameActual=0; tiempoFrame=0.f; }
    estadoAnim = nuevo;

    std::vector<QPixmap>* frames = nullptr;
    float multAnim = 1.0f;

    switch (estadoAnim) {
    case EstadoAnim::IDLE:
        frames = &framesIdle;
        break;
    case EstadoAnim::CORRIENDO:
        frames = &framesCorriendo;
        break;
    case EstadoAnim::CORRIENDO_ARRIBA:
        frames   = &framesUprun;
        multAnim = boostActivo ? 0.4f : 0.7f;
        break;
    case EstadoAnim::CORRIENDO_ABAJO:
        frames   = &framesDownrun;
        multAnim = boostActivo ? 0.4f : 0.7f;
        break;
    case EstadoAnim::DESLIZANDO:
        frames = &framesDeslizando;
        break;
    default:
        frames = &framesIdle;
        break;
    }

    // Movimiento vertical dominante → animación más lenta
    if (vt > 10.f && std::abs(Vy) > std::abs(Vx))
        multAnim = 1.8f;

    if (frames) tickAnimacion(dt, *frames, true, multAnim);
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  tickAnimacion — Nivel 1 (QVector<QPixmap>)
// ============================================================
void Personaje::tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop)
{
    if (frames.isEmpty() || !itemGrafico) return;
    tiempoFrame += dt;
    if (tiempoFrame >= duracionFrame) {
        tiempoFrame = 0.f;
        frameActual = loop
            ? (frameActual + 1) % frames.size()
            : std::min(frameActual + 1, (int)frames.size() - 1);
    }
    QPixmap frame = frames[frameActual];
    if (!miraDerecha)
        frame = frame.transformed(QTransform().scale(-1, 1));
    itemGrafico->setPixmap(frame);
}

// ============================================================
//  tickAnimacion — Nivel 2 (std::vector<QPixmap> + multVelocidad)
//  multVelocidad > 1 → cada frame dura más → animación más lenta
//  Aplica tinte cian si el boost está activo.
// ============================================================
void Personaje::tickAnimacion(float dt, std::vector<QPixmap>& frames,
                              bool loop, float multVelocidad)
{
    if (frames.empty() || !itemGrafico) return;

    tiempoFrame += dt;
    float duracionEfectiva = duracionFrame * multVelocidad;

    if (tiempoFrame >= duracionEfectiva) {
        tiempoFrame = 0.f;
        frameActual = loop
            ? (frameActual + 1) % (int)frames.size()
            : std::min(frameActual + 1, (int)frames.size() - 1);
    }

    QPixmap frame = frames[frameActual];

    if (!miraDerecha)
        frame = frame.transformed(QTransform().scale(-1, 1));

    // Tinte cian semitransparente cuando el boost está activo
    if (boostActivo) {
        QPixmap tinted(frame.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, frame);
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.fillRect(frame.rect(), QColor(0, 200, 255, 60));
        p.end();
        frame = tinted;
    }

    itemGrafico->setPixmap(frame);
}

// ============================================================
//  cargarSpritesNivel1
//  Spritesheet: 669x373 px — fondo negro (0,0,0)
//
//  Fila 1 (y=44 h=65):  IDLE (1f) + CORRER (7f)
//  Fila 2 (y=110 h=98): SALTO completo (12f)
//  Fila 3 (y=214 h=66): VIENTO — OMITIDA
//  Fila 4 (y=294 h=59): CAÍDA FINAL (3f)
// ============================================================
void Personaje::cargarSpritesNivel1()
{
    QPixmap sheet(":/Kael_nivel1/Sprites/Nivel1/sprites nivel 1 kael.png");
    if (sheet.isNull()) {
        qDebug() << "ERROR: No se cargo sprites nivel 1 kael.png";
        return;
    }
    qDebug() << "Spritesheet N1:" << sheet.width() << "x" << sheet.height();

    const int TW = static_cast<int>(ANCHO);
    const int TH = static_cast<int>(ALTO);

    auto extraer = [&](int x1, int y1, int w, int h) -> QPixmap {
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
            .scaled(TW,TH,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    };

    // IDLE
    n1_framesIdle.clear();
    n1_framesIdle.append(extraer(17, 44, 36, 65));

    // CORRER (7 frames)
    n1_framesCorriendo.clear();
    n1_framesCorriendo.append(extraer( 86, 44, 42, 65));
    n1_framesCorriendo.append(extraer(148, 44, 44, 65));
    n1_framesCorriendo.append(extraer(206, 44, 54, 65));
    n1_framesCorriendo.append(extraer(269, 44, 61, 65));
    n1_framesCorriendo.append(extraer(346, 44, 55, 65));
    n1_framesCorriendo.append(extraer(402, 44, 64, 65));
    n1_framesCorriendo.append(extraer(490, 44, 51, 65));

    // SALTO (12 frames)
    n1_framesSaltando.clear();
    n1_framesSaltando.append(extraer( 18, 110, 38, 98));
    n1_framesSaltando.append(extraer( 70, 110, 57, 98));
    n1_framesSaltando.append(extraer(127, 110, 58, 98));
    n1_framesSaltando.append(extraer(185, 110, 58, 98));
    n1_framesSaltando.append(extraer(251, 110, 40, 98));
    n1_framesSaltando.append(extraer(322, 110, 33, 98));
    n1_framesSaltando.append(extraer(371, 110, 34, 98));
    n1_framesSaltando.append(extraer(418, 110, 38, 98));
    n1_framesSaltando.append(extraer(462, 110, 44, 98));
    n1_framesSaltando.append(extraer(506, 110, 45, 98));
    n1_framesSaltando.append(extraer(552, 110, 43, 98));
    n1_framesSaltando.append(extraer(605, 110, 39, 98));

    // VIENTO — OMITIDA
    n1_framesVientoCalda.clear();

    // CAÍDA FINAL (3 frames del personaje, ignorar obstáculos/UI)
    n1_framesCaidaFinal.clear();
    n1_framesCaidaFinal.append(extraer( 18, 294, 60, 59));
    n1_framesCaidaFinal.append(extraer( 96, 294, 60, 59));
    n1_framesCaidaFinal.append(extraer(180, 294, 67, 59));

    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.09f;
    miraDerecha   = true;

    qDebug() << "N1 OK:"
             << "Idle:"       << n1_framesIdle.size()
             << "Correr:"     << n1_framesCorriendo.size()
             << "Salto:"      << n1_framesSaltando.size()
             << "CaidaFinal:" << n1_framesCaidaFinal.size();
}

// ============================================================
//  cargarSpritesNivel2
//  Spritesheet: Sprites_kael_movimientos.png (nivel 2 real)
//  Lambda con fw/fh/separacion explícitos (versión REMOTE)
// ============================================================
void Personaje::cargarSpritesNivel2()
{
    QPixmap sheet(":/Kael_nivel2/Sprites/Nivel2/Sprites_kael_movimientos.png");
    if (sheet.isNull()) {
        qDebug() << "ERROR: No se pudo cargar la hoja de sprites nivel 2.";
        return;
    }

    auto recortar = [&](std::vector<QPixmap>& destino,
                        int ox, int oy, int numFrames,
                        int fw, int fh, int separacion = 10)
    {
        destino.clear();
        for (int i = 0; i < numFrames; ++i) {
            int xFinal = ox + i * (fw + separacion);
            QPixmap frame;
            if (xFinal + fw <= sheet.width() && oy + fh <= sheet.height()) {
                frame = sheet.copy(xFinal, oy, fw, fh);
            } else {
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

    //            destino          ox   oy  frames  fw   fh   sep
    recortar(framesIdle,           42, 140,    4,   93, 109,  10);
    recortar(framesCorriendo,      45, 338,    8,   66,  86,  15);
    recortar(framesDeslizando,     41, 516,    4,   89,  97,  10);
    recortar(framesUprun,         725, 314,    9,   60, 105,  12);
    recortar(framesDownrun,       320, 331,    2,   93, 109,  10);

    estadoAnim    = EstadoAnim::IDLE;
    frameActual   = 0;
    tiempoFrame   = 0.f;
    duracionFrame = 0.1f;
    miraDerecha   = true;

    if (itemGrafico && !framesIdle.empty()) {
        itemGrafico->setPixmap(framesIdle.at(0));
        itemGrafico->setTransformOriginPoint(
            framesIdle.at(0).width()  / 2.0,
            framesIdle.at(0).height() / 2.0);
    }
}

// ============================================================
//  eliminarFondo
// ============================================================
QPixmap Personaje::eliminarFondo(const QPixmap& src, QColor cf, int tol)
{
    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int py=0; py<img.height(); ++py)
        for (int px=0; px<img.width(); ++px) {
            QColor p = img.pixelColor(px,py);
            if (std::abs(p.red()  -cf.red())  <=tol &&
                std::abs(p.green()-cf.green())<=tol &&
                std::abs(p.blue() -cf.blue()) <=tol)
                img.setPixelColor(px,py,Qt::transparent);
        }
    return QPixmap::fromImage(img);
}

// ============================================================
//  Caída final (nivel 1)
// ============================================================
void Personaje::activarCaidaFinal()
{
    if (enCaidaFinal) return;
    enCaidaFinal = true;
    estadoAnim   = EstadoAnim::CAIDA_FINAL;
    frameActual  = 0;
    tiempoFrame  = 0.f;
    Vx = 0.f; Vy = 0.f;
}

bool Personaje::caidaFinalTerminada() const
{
    if (!enCaidaFinal) return false;
    return frameActual >= (int)n1_framesCaidaFinal.size() - 1;
}

// ============================================================
//  Hitbox ajustable (nivel 2)
// ============================================================
void Personaje::setHitboxOffset(float offsetX, float offsetY,
                                float anchoEfectivo, float altoEfectivo)
{
    hitboxOffsetX   = offsetX;
    hitboxOffsetY   = offsetY;
    hitboxAnchoReal = anchoEfectivo;
    hitboxAltoReal  = altoEfectivo;
    ANCHO           = anchoEfectivo;
    ALTO            = altoEfectivo;
}

// ============================================================
//  Colisión / reset
// ============================================================
void Personaje::aterrizarEnSuelo(float)
{
    Vy = 0.f; enSuelo = true; saltosRestantes = 2; tiempoViento = 0.f;
    yMasAlta = y;
    if (itemGrafico) itemGrafico->setPos(x, y);
}

void Personaje::despegarSuelo() { enSuelo = false; }

void Personaje::recibirDanio(int n) { vidas -= n; if (vidas<0) vidas=0; }

void Personaje::resetearPosicion(float rx, float ry)
{
    x=rx; y=ry; Vx=0.f; Vy=0.f;
    enSuelo=false; saltosRestantes=2;
    tiempoViento=0.f; enCaidaFinal=false;
    yMasAlta=ry; plataformasCalda=0;
    estadoAnim=EstadoAnim::IDLE; frameActual=0;
    if (itemGrafico) itemGrafico->setPos(x,y);
}

// ── Getters ───────────────────────────────────────────────────
int   Personaje::getVidas()        const { return vidas;        }
float Personaje::getEnergia()      const { return energia;      }
bool  Personaje::isEnSuelo()       const { return enSuelo;      }
bool  Personaje::isBoostActivo()   const { return boostActivo;  }
bool  Personaje::isDeslizando()    const { return deslizando;   }
float Personaje::getFactorSigilo() const { return factorSigilo; }
float Personaje::getYMasAlta()     const { return yMasAlta;     }
