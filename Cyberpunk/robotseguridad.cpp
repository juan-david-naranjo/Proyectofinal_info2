#include "robotseguridad.h"
#include "gestorfisicas.h"
#include <cmath>
#include <algorithm>

RobotSeguridad::RobotSeguridad(float px, float py,
                               float rDeteccion,
                               float rDesenganche,
                               float vPatrulla,
                               float vPersecucion,
                               const std::vector<Punto2D> &wps)
    : Enemigo(px, py, vPatrulla, rDeteccion)
    , estado(EstadoAgente::PATRULLAJE)
    , radioDeteccion(rDeteccion)
    , radioDesenganche(rDesenganche)
    , velPatrulla(vPatrulla)
    , velPersecucion(vPersecucion)
    , waypoints(wps)
    , frameActual(0)
    , tiempoFrame(0.f)
    , duracionFramePatrullaje(0.1f)   // 10 fps en patrullaje
    , duracionFrameAlert(0.15f)       // 6-7 fps en alerta (más dramático)
{
    // El ítem gráfico lo asigna el nivel
    itemGrafico = nullptr;

    // Si no hay waypoints, el robot se queda quieto en su posición
    if (waypoints.empty())
        waypoints.push_back(Punto2D(px, py));
}

// ════════════════════════════════════════════════════════════════════════════
//  cargarSprites
//  Recibe la QPixmap ya cargada (misma hoja que el personaje u otra).
//  Elimina los fondos de cada grupo de animación de forma independiente.
// ════════════════════════════════════════════════════════════════════════════
void RobotSeguridad::cargarSprites(const QPixmap& sheet)
{
    if (sheet.isNull()) return;

    // ── Helper: elimina un color de fondo con tolerancia ─────────────────────
    auto quitarFondo = [](const QPixmap& src, QColor fondo, int tol) -> QPixmap
    {
        QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
        const int r = fondo.red(), g = fondo.green(), b = fondo.blue();

        for (int py = 0; py < img.height(); py++)
            for (int px = 0; px < img.width(); px++)
            {
                QColor p = img.pixelColor(px, py);
                if (std::abs(p.red()   - r) <= tol &&
                    std::abs(p.green() - g) <= tol &&
                    std::abs(p.blue()  - b) <= tol)
                    img.setPixelColor(px, py, Qt::transparent);
            }
        return QPixmap::fromImage(img);
    };

    // ════════════════════════════════════════════════════════════════════════
    //  ANIMACIÓN DE PATRULLAJE (estado normal, sin detectar al jugador)
    //
    //  8 frames · origen (723, 135) · 71 × 70 px · separación 11 px
    //  Fondo: #2f4b56
    // ════════════════════════════════════════════════════════════════════════
    framesPatrullaje.clear();
    {
        const int ox  = 723, oy = 135;
        const int fw  = 71,  fh = 70;
        const int sep = 11;
        const int num = 8;
        const QColor bg(0x2f, 0x4b, 0x56);

        for (int i = 0; i < num; i++)
        {
            int x = ox + i * (fw + sep);
            if (x + fw > sheet.width() || oy + fh > sheet.height())
            {
                QPixmap ph(fw, fh);
                ph.fill(Qt::transparent);
                framesPatrullaje.append(ph);
                continue;
            }
            QPixmap frame = sheet.copy(x, oy, fw, fh);
            framesPatrullaje.append(quitarFondo(frame, bg, 10));
        }
    }

    // ════════════════════════════════════════════════════════════════════════
    //  ANIMACIÓN DE ALERTA / PERSECUCIÓN (robot detecta al jugador)
    //
    //  3 frames en posiciones irregulares dentro de la hoja:
    //
    //    Frame 0 → (791, 295)  251 × 78  fondo #0e1528
    //    Frame 1 → (953, 392)   77 × 79  (mismo fondo que frame 0)
    //    Frame 2 → (1036, 328)  79 × 102 fondo #0f172a
    //
    //  Al tener tamaños distintos se escalan al tamaño del frame 0 para
    //  que la transición no cambie el tamaño visual del robot en pantalla.
    // ════════════════════════════════════════════════════════════════════════
    framesAlert.clear();
    {
        const QColor bg0(0x0e, 0x15, 0x28);
        const QColor bg2(0x0f, 0x17, 0x2a);

        // Tamaño de referencia: el del frame más pequeño (77 × 78 aprox.)
        // Lo normalizamos al tamaño de los frames de patrullaje (71 × 70).
        const int refW = 71, refH = 70;

        struct AlertFrame { int x, y, w, h; QColor bg; };
        static const AlertFrame aFrames[] = {
                                             { 791, 295, 251, 78,  bg0 },
                                             { 953, 392,  77, 79,  bg0 },
                                             {1036, 328,  79, 102, bg2 },
                                             };

        for (const auto& af : aFrames)
        {
            QPixmap raw;
            if (af.x + af.w <= sheet.width() && af.y + af.h <= sheet.height())
                raw = sheet.copy(af.x, af.y, af.w, af.h);
            else
            {
                raw = QPixmap(af.w, af.h);
                raw.fill(Qt::transparent);
            }

            QPixmap limpio = quitarFondo(raw, af.bg, 10);

            // Escalar al tamaño de referencia para animación uniforme
            framesAlert.append(
                limpio.scaled(refW, refH, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
        }
    }

    // ── Aplicar primer frame y pivote de rotación en el centro ───────────────
    if (itemGrafico && !framesPatrullaje.isEmpty())
    {
        const QPixmap& f0 = framesPatrullaje.at(0);
        itemGrafico->setPixmap(f0);
        itemGrafico->setTransformOriginPoint(f0.width() / 2.0, f0.height() / 2.0);
    }
}









// ── PERCIBIR ─────────────────────────────────────────────────────────────────
// Calcula la distancia al jugador y actualiza la posición conocida.
void RobotSeguridad::percibir(float jx, float jy)
{
    jugadorPosX  = jx;
    jugadorPosY  = jy;
    float dx     = jx - x;
    float dy     = jy - y;
    distanciaJugador = std::sqrt(dx*dx + dy*dy);
}

float RobotSeguridad::calcularDistancia() const
{
    return distanciaJugador;
}

// ── RAZONAR ──────────────────────────────────────────────────────────────────
// Evalúa el estado actual y decide la transición.
void RobotSeguridad::razonar()
{
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
        // Transición a PERSECUCION si el jugador entra en el radio
        if (GestorFisicas::colisionCirculo(x, y,
                                           jugadorPosX, jugadorPosY,
                                           radioDeteccion))
        {
            estado = EstadoAgente::PERSECUCION;
            tiempoPersecucion = 0.f;
            // Guardar posición en historial (aprendizaje)
            actualizarWaypoints();
        }
        break;

    case EstadoAgente::PERSECUCION:
        // Solo puede volver a patrullaje si pasó el tiempo mínimo Y
        // el jugador salió del radio de desenganche
        if (tiempoPersecucion >= DURACION_MIN_PERSECUCION &&
            !GestorFisicas::colisionCirculo(x, y,
                                            jugadorPosX, jugadorPosY,
                                            radioDesenganche))
        {
            estado = EstadoAgente::PATRULLAJE;
            // El próximo waypoint será la última posición guardada
        }
        break;
    }
}

// ── ACTUAR ───────────────────────────────────────────────────────────────────
// Ejecuta el movimiento según el estado.
void RobotSeguridad::actuar(float dt)
{
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
    {
        // Seguir waypoints en orden circular (usando Punto2D, sin Qt)
        Punto2D objetivo = waypoints[indiceWaypoint];
        float dx = objetivo.x - x;
        float dy = objetivo.y - y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 8.f)
        {
            // Llegó al waypoint → avanzar al siguiente
            indiceWaypoint = (indiceWaypoint + 1) % static_cast<int>(waypoints.size());
        }
        else
        {
            moverHacia(objetivo.x, objetivo.y, dt);
        }
        break;
    }

    case EstadoAgente::PERSECUCION:
    {
        tiempoPersecucion += dt;
        // Guardar la última posición vista (aprendizaje continuo)
        if (historial.empty() ||
            std::abs(jugadorPosX - historial.back().x) > 20.f ||
            std::abs(jugadorPosY - historial.back().y) > 20.f)
        {
            historial.push_back(Punto2D(jugadorPosX, jugadorPosY));
        }
        // Perseguir con velocidad aumentada
        moverHacia(jugadorPosX, jugadorPosY, dt);
        break;
    }
    }

    // Sincronizar sprite
    if (itemGrafico)
        itemGrafico->setPos(x, y);
}

// ── TICK (método completo para el nivel) ─────────────────────────────────────
void RobotSeguridad::tick(float jx, float jy, float dt)
{
    percibir(jx, jy);
    razonar();
    actuar(dt);
}

// ── APRENDIZAJE: actualizar waypoints ────────────────────────────────────────
// Añade la última posición vista del jugador al patrón de vigilancia.
void RobotSeguridad::actualizarWaypoints()
{
    Punto2D nuevoPunto(jugadorPosX, jugadorPosY);

    // Evitar duplicados muy cercanos (usando Punto2D, sin Qt)
    for (const Punto2D &wp : waypoints)
    {
        float dx = wp.x - nuevoPunto.x;
        float dy = wp.y - nuevoPunto.y;
        if (std::sqrt(dx*dx + dy*dy) < 40.f) return;
    }

    // Insertar después del waypoint actual para que lo visite pronto
    auto it = waypoints.begin() + ((indiceWaypoint + 1) % waypoints.size());
    waypoints.insert(it, nuevoPunto);
}

// ── MOVIMIENTO CON INERCIA ───────────────────────────────────────────────────
// Mueve el robot hacia (tx, ty) usando MRUA en ambos ejes.
void RobotSeguridad::moverHacia(float tx, float ty, float dt)
{
    float velActual = (estado == EstadoAgente::PERSECUCION)
                      ? velPersecucion
                      : velPatrulla;

    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist < 1.f) { Vx = 0; Vy = 0; return; }

    // Dirección normalizada
    float nx = dx / dist;
    float ny = dy / dist;

    float targetVx = nx * velActual;
    float targetVy = ny * velActual;

    // Aplicar inercia (MRUA)
    GestorFisicas::aplicarInercia(Vx, targetVx, dt);
    GestorFisicas::aplicarInercia(Vy, targetVy, dt);

    x += Vx * dt;
    y += Vy * dt;
}
