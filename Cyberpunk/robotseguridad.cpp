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
        const int ox  = 1, oy = 48;
        const int fw  = 29,  fh = 43;
        const int sep = 1;
        const int num = 5;
        const QColor bg(255, 0, 255);

        for (int i = 0; i < num; i++)
        {
            int x = ox + i * (fw + sep);
            if (x + fw > sheet.width() || oy + fh > sheet.height())
            {
                QPixmap ph(fw, fh);
                ph.fill(Qt::transparent);
                framesPatrullaje.push_back(ph);
                continue;
            }
            QPixmap frame = sheet.copy(x, oy, fw, fh);
            framesPatrullaje.push_back(quitarFondo(frame, bg, 0));
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
        const QColor bg(255, 0, 255);
        // const QColor bg2(0x0f, 0x17, 0x2a);

        const int ox  = 1, oy = 222;
        const int fw  = 80,  fh = 45;
        const int sep = 1;
        const int num = 8;


        for (int i = 0; i < num; i++)
        {
            int x = ox + i * (fw + sep);
            if (x + fw > sheet.width() || oy + fh > sheet.height())
            {
                QPixmap ph(fw, fh);
                ph.fill(Qt::transparent);
                framesAlert.push_back(ph);
                continue;
            }
            QPixmap frame = sheet.copy(x, oy, fw, fh);
            framesAlert.push_back(quitarFondo(frame, bg, 0));
        }

    }

    // ── Aplicar primer frame y pivote de rotación en el centro ───────────────
    if (itemGrafico && !framesPatrullaje.empty())
    {
        const QPixmap& f0 = framesPatrullaje.at(0);
        itemGrafico->setPixmap(f0);
        itemGrafico->setTransformOriginPoint(f0.width() / 2.0, f0.height() / 2.0);
    }
}

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

// void RobotSeguridad::moverHacia(float tx, float ty, float dt)
// {
//     float velActual = (estado == EstadoAgente::PERSECUCION) ? velPersecucion : velPatrulla;

//     float dx = tx - x;
//     float dy = ty - y;
//     float dist = std::sqrt(dx*dx + dy*dy);

//     if (dist < 2.f) { Vx = 0; Vy = 0; return; }

//     // 1. Intentar trayectoria directa (Diagonal/Recta original)
//     float nx = dx / dist;
//     float ny = dy / dist;
//     float targetVx = nx * velActual;
//     float targetVy = ny * velActual;

//     // Simular dónde estaría el robot en el próximo frame si va directo
//     float siguienteX_directo = x + targetVx * dt;
//     float siguienteY_directo = y + targetVy * dt;

//     // ── EVALUACIÓN DE OBSTÁCULO ──
//     // Reemplaza 'GestorFisicas::colisionConMuro' por tu función real de colisión del nivel
//     bool caminoDirectoBloqueado = GestorFisicas::colisionConMuro(siguienteX_directo, siguienteY_directo);

//     if (caminoDirectoBloqueado)
//     {
//         // ¡Alerta! Hay una pared al frente. Aplicamos tu lógica: Descomponer en ejes.

//         // Prueba 1: Intentar moverse SOLO en Y (para buscar la altura del objetivo)
//         float siguienteY_solo = y + (ny > 0 ? velActual : -velActual) * dt;
//         bool ejeY_libre = !GestorFisicas::colisionConMuro(x, siguienteY_solo) && std::abs(dy) > 4.f;

//         // Prueba 2: Intentar moverse SOLO en X (moverse hacia el lado)
//         float siguienteX_solo = x + (nx > 0 ? velActual : -velActual) * dt;
//         bool ejeX_libre = !GestorFisicas::colisionConMuro(siguienteX_solo, y) && std::abs(dx) > 4.f;

//         if (ejeY_libre)
//         {
//             // Se alinea verticalmente primero (tu idea de "subir hasta la altura")
//             targetVx = 0.f;
//             targetVy = (dy > 0) ? velActual : -velActual;
//         }
//         else if (ejeX_libre)
//         {
//             // Si no puede en Y, intenta avanzar en X bordeando la pared
//             targetVx = (dx > 0) ? velActual : -velActual;
//             targetVy = 0.f;
//         }
//         else
//         {
//             // Si ambos ejes individuales están bloqueados por esquinas, retrocede un poco o frena
//             targetVx = -targetVx * 0.5f;
//             targetVy = -targetVy * 0.5f;
//         }
//     }

//     // 2. Aplicar las velocidades finales con la inercia que ya tenías
//     GestorFisicas::aplicarInercia(Vx, targetVx, dt);
//     GestorFisicas::aplicarInercia(Vy, targetVy, dt);

//     x += Vx * dt;
//     y += Vy * dt;
// }








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
// void RobotSeguridad::razonar()
// {
//     switch (estado)
//     {
//     case EstadoAgente::PATRULLAJE:
//         // Transición a PERSECUCION si el jugador entra en el radio
//         if (GestorFisicas::colisionCirculo(x, y,
//                                            jugadorPosX, jugadorPosY,
//                                            radioDeteccion))
//         {
//             estado = EstadoAgente::PERSECUCION;
//             tiempoPersecucion = 0.f;
//             // Guardar posición en historial (aprendizaje)
//             actualizarWaypoints();
//         }
//         break;

//     case EstadoAgente::PERSECUCION:
//         // Solo puede volver a patrullaje si pasó el tiempo mínimo Y
//         // el jugador salió del radio de desenganche
//         if (tiempoPersecucion >= DURACION_MIN_PERSECUCION &&
//             !GestorFisicas::colisionCirculo(x, y,
//                                             jugadorPosX, jugadorPosY,
//                                             radioDesenganche))
//         {
//             estado = EstadoAgente::PATRULLAJE;
//             // El próximo waypoint será la última posición guardada
//         }
//         break;
//     }
// }


void RobotSeguridad::razonar(bool jugadorOculto)
{
     //qDebug() << "razonar llamado | oculto:" << jugadorOculto << "| estado:" << (int)estado;
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
        // Si el jugador está oculto, el robot no puede detectarlo
        if (!jugadorOculto &&
            GestorFisicas::colisionCirculo(x, y,
                                           jugadorPosX, jugadorPosY,
                                           radioDeteccion))
        {
            estado = EstadoAgente::PERSECUCION;
            frameActual       = 0;    // ← animación de alerta desde frame 0
            tiempoFrame       = 0.f;
            tiempoPersecucion = 0.f;
            actualizarWaypoints();
        }
        break;

    case EstadoAgente::PERSECUCION:

        // Desenganche normal por distancia
        if (tiempoPersecucion >= DURACION_MIN_PERSECUCION &&
            !GestorFisicas::colisionCirculo(x, y,
                                            jugadorPosX, jugadorPosY,
                                            radioDesenganche))
        {
            estado = EstadoAgente::PATRULLAJE;
            frameActual       = 0;    // ← animación de alerta desde frame 0
            tiempoFrame       = 0.f;
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

        // Guardar historial de posición vista
        if (historial.empty() ||
            std::abs(jugadorPosX - historial.back().x) > 20.f ||
            std::abs(jugadorPosY - historial.back().y) > 20.f)
        {
            historial.push_back(Punto2D(jugadorPosX, jugadorPosY));
        }

        // ── Detección de atasco ───────────────────────────────
        float dxMov = x - posXAnterior;
        float dyMov = y - posYAnterior;
        float movimiento = std::sqrt(dxMov*dxMov + dyMov*dyMov);

        // Umbral normalizado a 60fps para que dt no afecte
        if (movimiento < 3.f * dt * 60.f)
            tiempoStuck += dt;
        else
            tiempoStuck = 0.f;

        posXAnterior = x;
        posYAnterior = y;

        // ── Si llegó al punto de desvío, cancelarlo ───────────
        if (tieneDesvio)
        {
            float dxD = puntoDesvio.x - x;
            float dyD = puntoDesvio.y - y;
            if (std::sqrt(dxD*dxD + dyD*dyD) < RADIO_LLEGADA_DESVIO)
                tieneDesvio = false;
        }

        // ── Generar desvío si está atascado ───────────────────
        if (tiempoStuck >= UMBRAL_STUCK)
        {
            float dxJ = jugadorPosX - x;
            float dyJ = jugadorPosY - y;
            float dist = std::sqrt(dxJ*dxJ + dyJ*dyJ);

            if (dist > 1.f)
            {
                // Dirección normalizada hacia el jugador
                float nx =  dxJ / dist;
                float ny =  dyJ / dist;

                // Perpendicular: rotar 90° según ladoDesvio
                float px = -ny * ladoDesvio;
                float py =  nx * ladoDesvio;

                puntoDesvio.x = x + px * DIST_DESVIO;
                puntoDesvio.y = y + py * DIST_DESVIO;
                tieneDesvio   = true;
                tiempoStuck   = 0.f;
                ladoDesvio   *= -1;  // próximo atasco gira al lado contrario
            }
        }

        // ── Moverse al desvío o directo al jugador ────────────
        if (tieneDesvio)
            moverHacia(puntoDesvio.x, puntoDesvio.y, dt);
        else
            moverHacia(jugadorPosX, jugadorPosY, dt);

        break;
    }
    // case EstadoAgente::PERSECUCION:
    // {
    //     tiempoPersecucion += dt;
    //     // Guardar la última posición vista (aprendizaje continuo)
    //     if (historial.empty() ||
    //         std::abs(jugadorPosX - historial.back().x) > 20.f ||
    //         std::abs(jugadorPosY - historial.back().y) > 20.f)
    //     {
    //         historial.push_back(Punto2D(jugadorPosX, jugadorPosY));
    //     }
    //     // Perseguir con velocidad aumentada
    //     moverHacia(jugadorPosX, jugadorPosY, dt);
    //     break;
    // }
    }




    // ── ANIMACIÓN + ORIENTACIÓN + POSICIÓN ────────────────────────────────
    if (itemGrafico)
    {
        // ── Seleccionar vector de frames según estado ─────────────────────
        const std::vector<QPixmap>* frames = nullptr;
        float duracion = duracionFramePatrullaje;

        if (estado == EstadoAgente::PERSECUCION && !framesAlert.empty())
        {
            frames   = &framesAlert;
            duracion = duracionFrameAlert;
            //qDebug("aqui no esta tirando error 361");

        }
        else if (!framesPatrullaje.empty())
        {
            frames   = &framesPatrullaje;
            duracion = duracionFramePatrullaje;
            //qDebug("aqui no esta tirando error 368");
        }

        // ── Avanzar frame ─────────────────────────────────────────────────
        if (frames && !frames->empty())
        {
            tiempoFrame += dt;
            if (tiempoFrame >= duracion)
            {
                tiempoFrame  = 0.f;
                frameActual  = (frameActual + 1) % static_cast<int>(frames->size());
            }
            //qDebug("aqui no esta tirando error 380");
            itemGrafico->setPixmap(frames->at(frameActual));
        }
        //qDebug("aqui esta tirando error 383");
        itemGrafico->setPos(x, y);
    }
}

// ── TICK (método completo para el nivel) ─────────────────────────────────────
// void RobotSeguridad::tick(float jx, float jy, float dt)
// {
//     percibir(jx, jy);
//     razonar();
//     actuar(dt);
// }

void RobotSeguridad::tick(float jx, float jy, float dt, bool jugadorOculto)
{
    percibir(jx, jy);
    razonar(jugadorOculto);
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

