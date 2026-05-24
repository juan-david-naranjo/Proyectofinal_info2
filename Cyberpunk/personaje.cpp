#include "personaje.h"
#include "gestorfisicas.h"
#include <algorithm>
#include <cmath>

// ============================================================
//  Constructor
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

void Personaje::activarBoost()  { boostActivo=true; tiempoBoost=DURACION_BOOST; }
void Personaje::activarDesliz() { if(enSuelo&&!deslizando){deslizando=true;tiempoDesliz=DURACION_DESLIZ_MAX;} }
void Personaje::actualizar(float dt) { actualizarNivel2(dt); }

// ============================================================
//  actualizarNivel1
// ============================================================
void Personaje::actualizarNivel1(float dt, float tiempoTotal)
{
    // ── Caída final: bloquear física, solo animar ─────────────
    if (enCaidaFinal)
    {
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

    // ── Viento ────────────────────────────────────────────────
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
    if (!enSuelo && estadoAnim == EstadoAnim::SALTANDO && Vy < 0.f)
        nuevo = EstadoAnim::SALTANDO;
    else if (!enSuelo)
        nuevo = EstadoAnim::SALTANDO;   // en el aire = frame salto
    else if (std::abs(Vx) > 10.f)
        nuevo = EstadoAnim::CORRIENDO;
    else
        nuevo = EstadoAnim::IDLE;

    if (nuevo != estadoAnim) { estadoAnim=nuevo; frameActual=0; tiempoFrame=0.f; }

    QVector<QPixmap>* frames = nullptr;
    bool loop = true;
    switch (estadoAnim) {
    case EstadoAnim::IDLE:      frames = &n1_framesIdle;      break;
    case EstadoAnim::CORRIENDO: frames = &n1_framesCorriendo; break;
    case EstadoAnim::SALTANDO:  frames = &n1_framesSaltando;  break;
    default:                    frames = &n1_framesIdle;      break;
    }
    if (frames) tickAnimacion(dt, *frames, loop);
    if (itemGrafico) itemGrafico->setPos(x, y);
}

// ============================================================
//  actualizarNivel2
// ============================================================
void Personaje::actualizarNivel2(float dt)
{
    float ve = boostActivo ? velMax*MULTIPLICADOR_BOOST : velMax;
    if (deslizando) {
        GestorFisicas::aplicarInercia(Vx,0.f,dt);
        GestorFisicas::aplicarInercia(Vy,0.f,dt);
        tiempoDesliz-=dt; if(tiempoDesliz<=0.f){deslizando=false;tiempoDesliz=0.f;}
    } else {
        float vox=0.f,voy=0.f;
        if(keys[0]){vox=-ve;miraDerecha=false;}
        if(keys[1]){vox= ve;miraDerecha=true; }
        if(keys[2]) voy=-ve;
        if(keys[3]) voy= ve;
        GestorFisicas::aplicarInercia(Vx,vox,dt);
        GestorFisicas::aplicarInercia(Vy,voy,dt);
    }
    x+=Vx*dt; y+=Vy*dt;
    float vt=std::sqrt(Vx*Vx+Vy*Vy);
    factorSigilo=(vt<velMax*0.4f)?0.5f:1.f;
    if(boostActivo){tiempoBoost-=dt;if(tiempoBoost<=0.f){boostActivo=false;tiempoBoost=0.f;}}

    EstadoAnim nuevo;
    if(boostActivo)                                  nuevo=EstadoAnim::BOOST;
    else if(deslizando)                              nuevo=EstadoAnim::DESLIZANDO;
    else if(std::abs(Vx)>10.f||std::abs(Vy)>10.f)  nuevo=EstadoAnim::CORRIENDO;
    else                                             nuevo=EstadoAnim::IDLE;
    if(nuevo!=estadoAnim){estadoAnim=nuevo;frameActual=0;tiempoFrame=0.f;}

    QVector<QPixmap>* frames=nullptr;
    switch(estadoAnim){
    case EstadoAnim::IDLE:       frames=&framesIdle;      break;
    case EstadoAnim::CORRIENDO:  frames=&framesCorriendo; break;
    case EstadoAnim::DESLIZANDO: frames=&framesDeslizando;break;
    case EstadoAnim::BOOST:      frames=&framesBoost;     break;
    default:                     frames=&framesIdle;      break;
    }
    if(frames) tickAnimacion(dt,*frames,true);
    if(itemGrafico) itemGrafico->setPos(x,y);
}

// ============================================================
//  tickAnimacion
// ============================================================
void Personaje::tickAnimacion(float dt, QVector<QPixmap>& frames, bool loop)
{
    if (frames.isEmpty() || !itemGrafico) return;
    tiempoFrame += dt;
    if (tiempoFrame >= duracionFrame) {
        tiempoFrame = 0.f;
        frameActual = loop
                          ? (frameActual+1) % frames.size()
                          : std::min(frameActual+1, (int)frames.size()-1);
    }
    QPixmap frame = frames[frameActual];
    if (!miraDerecha)
        frame = frame.transformed(QTransform().scale(-1,1));
    itemGrafico->setPixmap(frame);
}

// ============================================================
//  cargarSpritesNivel1
//
//  Spritesheet: 669 x 373 px — fondo negro
//
//  FILA 1 (y=44-109, h=65) — SPRINT/IDLE: 8 grupos
//    Grupo 1:  x=17  w=36  → IDLE (personaje estático)
//    Grupo 2:  x=86  w=42  → CORRER frame 1
//    Grupo 3:  x=148 w=44  → CORRER frame 2
//    Grupos 4-8: no usados (boost, omitidos)
//
//  FILA 2 (y=110-208, h=98) — SALTO: 9 grupos
//    Grupos 1-9: frames del salto completo (impulso → arco → caída)
//    Grupo 2 (x=70-243) contiene múltiples sub-frames pegados
//    Se dividen en 5 frames uniformes para el arco de salto
//
//  FILA 3 (y=214-280) — VIENTO: OMITIDA
//
//  FILA 4 (y=294-353, h=59) — CAÍDA FINAL (antes de respawn):
//    Solo se activa si el personaje cae ≥4 plataformas hacia abajo
//    Grupos 1-3: x=18-247 (3 frames, excluye obstáculos/UI)
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
    n1_framesSaltando.clear();
    n1_framesSaltando.append(extraer( 18, 110, 38, 98));  // prep
    n1_framesSaltando.append(extraer( 70, 110, 57, 98));  // arco f1
    n1_framesSaltando.append(extraer(127, 110, 58, 98));  // arco f2
    n1_framesSaltando.append(extraer(185, 110, 58, 98));  // arco f3
    n1_framesSaltando.append(extraer(251, 110, 40, 98));  // apex
    n1_framesSaltando.append(extraer(322, 110, 33, 98));  // caída f1
    n1_framesSaltando.append(extraer(371, 110, 34, 98));  // caída f2
    n1_framesSaltando.append(extraer(418, 110, 38, 98));  // caída f3
    n1_framesSaltando.append(extraer(462, 110, 44, 98));  // doble f1
    n1_framesSaltando.append(extraer(506, 110, 45, 98));  // doble f2
    n1_framesSaltando.append(extraer(552, 110, 43, 98));  // post
    n1_framesSaltando.append(extraer(605, 110, 39, 98));  // aterrizaje

    // ── FILA 3 (y=214 h=66): VIENTO — OMITIDA ────────────────
    n1_framesVientoCalda.clear();  // vacío hasta implementar ventilador

    // ── FILA 4 (y=294 h=59): CAÍDA FINAL ─────────────────────
    // Solo los 3 primeros grupos son del personaje.
    // Grupos 4-7 son obstáculos/UI del nivel — ignorar.
    // Grupo 1: x=18  w=60
    // Grupo 2: x=96  w=60
    // Grupo 3: x=180 w=67
    n1_framesCaidaFinal.clear();
    n1_framesCaidaFinal.append(extraer( 18, 294, 60, 59));
    n1_framesCaidaFinal.append(extraer( 96, 294, 60, 59));
    n1_framesCaidaFinal.append(extraer(180, 294, 67, 59));

    // ── Estado inicial ─────────────────────────────────────────
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
// ============================================================
void Personaje::cargarSpritesNivel2()
{
    QPixmap sheet(":/Kael_nivel2/Sprites/Nivel2/sprites nivel 2 kael.png");
    if (sheet.isNull()) { qDebug()<<"ERROR: sprites nivel 2"; return; }
    const int FW=70, FH=70;
    QColor bg(0x31,0x4d,0x58);
    auto recortar=[&](QVector<QPixmap>&dest,int ox,int oy,int n){
        dest.clear();
        for(int i=0;i<n;++i){
            int sx=ox+i*(FW+8);
            if(sx+FW<=sheet.width()&&oy+FH<=sheet.height())
                dest.append(eliminarFondo(sheet.copy(sx,oy,FW,FH),bg,5));
            else{QPixmap p(FW,FH);p.fill(Qt::transparent);dest.append(p);}
        }
    };
    recortar(framesIdle,       40, 136, 5);
    recortar(framesCorriendo,  40, 136, 5);
    recortar(framesDeslizando, 53, 335, 5);
    recortar(framesBoost,      450,331, 2);
    estadoAnim=EstadoAnim::IDLE; frameActual=0;
    tiempoFrame=0.f; duracionFrame=0.1f; miraDerecha=true;
}

// ============================================================
//  eliminarFondo (nivel 2)
// ============================================================
QPixmap Personaje::eliminarFondo(const QPixmap& src, QColor cf, int tol)
{
    QImage img=src.toImage().convertToFormat(QImage::Format_ARGB32);
    for(int py=0;py<img.height();++py)
        for(int px=0;px<img.width();++px){
            QColor p=img.pixelColor(px,py);
            if(std::abs(p.red()-cf.red())<=tol&&
                std::abs(p.green()-cf.green())<=tol&&
                std::abs(p.blue()-cf.blue())<=tol)
                img.setPixelColor(px,py,Qt::transparent);
        }
    return QPixmap::fromImage(img);
}

// ============================================================
//  Caída final
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
//  Colisión / reset
// ============================================================
void Personaje::aterrizarEnSuelo(float)
{
    Vy=0.f; enSuelo=true; saltosRestantes=2; tiempoViento=0.f;
    // Registrar la Y más alta cuando toca suelo
    yMasAlta = y;
    if(itemGrafico) itemGrafico->setPos(x,y);
}
void Personaje::despegarSuelo() { enSuelo=false; }

void Personaje::recibirDanio(int n) { vidas-=n; if(vidas<0)vidas=0; }

void Personaje::resetearPosicion(float rx, float ry)
{
    x=rx; y=ry; Vx=0.f; Vy=0.f;
    enSuelo=false; saltosRestantes=2;
    tiempoViento=0.f; enCaidaFinal=false;
    yMasAlta=ry; plataformasCalda=0;
    estadoAnim=EstadoAnim::IDLE; frameActual=0;
    if(itemGrafico) itemGrafico->setPos(x,y);
}

int   Personaje::getVidas()        const { return vidas;        }
float Personaje::getEnergia()      const { return energia;      }
bool  Personaje::isEnSuelo()       const { return enSuelo;      }
bool  Personaje::isBoostActivo()   const { return boostActivo;  }
bool  Personaje::isDeslizando()    const { return deslizando;   }
float Personaje::getFactorSigilo() const { return factorSigilo; }
float Personaje::getYMasAlta()     const { return yMasAlta;     }
