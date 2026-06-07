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
    , duracionFramePatrullaje(0.1f)
    , duracionFrameAlert(0.15f)
    , indiceWaypoint(0)          // ← crítico: índice al primer waypoint
    , tiempoPersecucion(0.f)
    , distanciaJugador(0.f)
    , posXAnterior(px)           // posición anterior = posición inicial
    , posYAnterior(py)
    , tiempoStuck(0.f)
    , tieneDesvio(false)         // ← crítico: sin desvío al arrancar
    , puntoDesvio(px, py)
    , ladoDesvio(1.f)            // ← crítico: debe ser ±1, nunca 0

{
    // El ítem gráfico lo asigna el nivel
    itemGrafico = nullptr;

    // Si no hay waypoints, el robot se queda quieto en su posición
    if (waypoints.empty())
        waypoints.push_back(Punto2D(px, py));
}

//-------------- Sobrecargas-----------------
RobotSeguridad::RobotSeguridad(const RobotSeguridad& otro)
    : Enemigo(otro)
    , estado(otro.estado)
    , radioDeteccion(otro.radioDeteccion)
    , radioDesenganche(otro.radioDesenganche)
    , velPatrulla(otro.velPatrulla)
    , velPersecucion(otro.velPersecucion)
    , waypoints(otro.waypoints)          // copia profunda del vector
    , historial(otro.historial)          // copia profunda del historial
    , indiceWaypoint(otro.indiceWaypoint)
    , tiempoPersecucion(otro.tiempoPersecucion)
    , distanciaJugador(otro.distanciaJugador)
    , frameActual(otro.frameActual)
    , tiempoFrame(otro.tiempoFrame)
    , duracionFramePatrullaje(otro.duracionFramePatrullaje)
    , duracionFrameAlert(otro.duracionFrameAlert)
    , framesPatrullaje(otro.framesPatrullaje)   // QPixmap comparte datos (implícitamente)
    , framesAlert(otro.framesAlert)
    , posXAnterior(otro.posXAnterior)
    , posYAnterior(otro.posYAnterior)
    , tiempoStuck(otro.tiempoStuck)
    , tieneDesvio(otro.tieneDesvio)
    , puntoDesvio(otro.puntoDesvio)
    , ladoDesvio(otro.ladoDesvio)
    , velObjetivo(otro.velObjetivo)
    , paredesCache(otro.paredesCache)
// itemGrafico = nullptr por herencia de EntidadJuego(otro)
{}

bool RobotSeguridad::operator==(const RobotSeguridad& otro) const
{
    // Iguales si están en la misma posición y en el mismo estado de IA
    return x == otro.x && y == otro.y && estado == otro.estado;
}


//  Operador de asignación — Regla de los Tres
//  RobotSeguridad no posee heap propio más allá de lo que hereda
//  (itemGrafico en EntidadJuego). Los vectores de QPixmap usan
//  copy-on-write de Qt. Se copian todos los campos de estado de IA.

RobotSeguridad& RobotSeguridad::operator=(const RobotSeguridad& otro)
{
    if (this == &otro) return *this;
    Enemigo::operator=(otro);                   // base: posición, velocidad, radio, jugadorPos
    estado                  = otro.estado;
    radioDeteccion          = otro.radioDeteccion;
    radioDesenganche        = otro.radioDesenganche;
    velPatrulla             = otro.velPatrulla;
    velPersecucion          = otro.velPersecucion;
    waypoints               = otro.waypoints;
    historial               = otro.historial;
    indiceWaypoint          = otro.indiceWaypoint;
    tiempoPersecucion       = otro.tiempoPersecucion;
    distanciaJugador        = otro.distanciaJugador;
    frameActual             = otro.frameActual;
    tiempoFrame             = otro.tiempoFrame;
    duracionFramePatrullaje = otro.duracionFramePatrullaje;
    duracionFrameAlert      = otro.duracionFrameAlert;
    framesPatrullaje        = otro.framesPatrullaje;  // QPixmap copy-on-write
    framesAlert             = otro.framesAlert;
    posXAnterior            = otro.posXAnterior;
    posYAnterior            = otro.posYAnterior;
    tiempoStuck             = otro.tiempoStuck;
    tieneDesvio             = otro.tieneDesvio;
    puntoDesvio             = otro.puntoDesvio;
    ladoDesvio              = otro.ladoDesvio;
    velObjetivo             = otro.velObjetivo;
    paredesCache            = otro.paredesCache;
    capturado               = otro.capturado;
    // itemGrafico: no se copia, pertenece a la escena Qt
    return *this;
}


//  cargarSprites
//  Recibe la QPixmap ya cargada (misma hoja que el personaje u otra).
//  Elimina los fondos de cada grupo de animación de forma independiente.

void RobotSeguridad::cargarSprites(const QPixmap& sheet)
{
    if (sheet.isNull()) return;

    // Helper: elimina un color de fondo con tolerancia
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

    //Aplicar primer frame y pivote de rotación en el centro
    if (itemGrafico && !framesPatrullaje.empty())
    {
        const QPixmap& f0 = framesPatrullaje.at(0);
        itemGrafico->setPixmap(f0);
        itemGrafico->setTransformOriginPoint(f0.width() / 2.0, f0.height() / 2.0);
    }
}

void RobotSeguridad::moverHacia(float tx, float ty, float dt)
{
    // ── PROTECCIÓN: Si el objetivo es un fantasma de la memoria, abortamos
    if (!std::isfinite(tx) || !std::isfinite(ty)) {
        Vx = 0.f; Vy = 0.f;
        return;
    }
    float velActual = (estado == EstadoAgente::PERSECUCION)
                          ? velPersecucion
                          : velPatrulla;

    float dx = tx - x;
    float dy = ty - y;
    float dist = std::sqrt(dx*dx + dy*dy);
    // ── PROTECCIÓN: Validar que la distancia sea real y suficiente
    if (!std::isfinite(dist) || dist < 1.f) {
        Vx = 0.f; Vy = 0.f;
        return;
    }

    if (dist < 1.f) { Vx = 0; Vy = 0; return; }

    // Dirección normalizada
    float nx = dx / dist;
    float ny = dy / dist;

    float targetVx = nx * velActual;
    float targetVy = ny * velActual;
    //qDebug() << "mostrando robot antes de aplicarInercia en moverhacia: " << x << " y: " << y;
    // Aplicar inercia (MRUA)
    GestorFisicas::aplicarInercia(Vx, targetVx, dt);
    GestorFisicas::aplicarInercia(Vy, targetVy, dt);

    x += Vx * dt;
    y += Vy * dt;
    //qDebug() << "mostrando robot despues de aplicarInercia en moverhacia: " << x << " y: " << y;
}




// PERCIBIR
// Calcula la distancia al jugador y actualiza la posición conocida.
void RobotSeguridad::percibir(float jx, float jy)
{
    jugadorPosX  = jx;
    jugadorPosY  = jy;
    float dx     = jx - x;
    float dy     = jy - y;
    distanciaJugador = std::sqrt(dx*dx + dy*dy);
    // ← NUEVO: señal de captura (misma geometría que antes en verificarDeteccion)
    if (GestorFisicas::colisionCirculo(x + 10.f, y + 10.f, jx, jy, RADIO_CAPTURA))
        capturado = true;
}

float RobotSeguridad::calcularDistancia() const
{
    return distanciaJugador;
}




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
            frameActual       = 0;
            tiempoFrame       = 0.f;
        }
        break;
    }

}

//  ACTUAR
// Ejecuta el movimiento según el estado.
// void RobotSeguridad::actuar(float dt)
void RobotSeguridad::actuar(float dt){
    switch (estado)
    {
    case EstadoAgente::PATRULLAJE:
    {
        // Sin cambios — waypoints conocidos no necesitan evasión proactiva
        Punto2D objetivo = waypoints[indiceWaypoint];
        float dx = objetivo.x - x;
        float dy = objetivo.y - y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist < 8.f)
            indiceWaypoint = (indiceWaypoint + 1) % static_cast<int>(waypoints.size());
        else
            moverHaciaConEvacion(objetivo.x, objetivo.y, dt);
        break;
    }
    case EstadoAgente::PERSECUCION:
    {
        tiempoPersecucion += dt;

        if (historial.empty() ||
            std::abs(jugadorPosX - historial.back().x) > 20.f ||
            std::abs(jugadorPosY - historial.back().y) > 20.f){
            // historial.push_back(wp);
            historial.push_back(Punto2D(jugadorPosX, jugadorPosY));

        }
        // Solo cambia la llamada final:
        if (tieneDesvio)
            moverHaciaConEvacion(puntoDesvio.x, puntoDesvio.y, dt);
        else
            moverHaciaConEvacion(jugadorPosX, jugadorPosY, dt);
        break;
    }
    }


    // ── ANIMACIÓN
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
        // qDebug()<<"robot N: ";
        // qDebug()<<"Coordenadas P(x,y) ("<<itemGrafico->x()<<","<<itemGrafico->y()<<")";
        itemGrafico->setPos(x, y);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  posicionLibre — comprueba si el robot cabría en (px, py) sin solapar paredes
// ════════════════════════════════════════════════════════════════════════════
// posicionLibre usa paredesCache como miembro
bool RobotSeguridad::posicionLibre(float px, float py, float tam) const
{
    for (const Hitbox& hb : paredesCache)
    {
        if (px < hb.x + hb.w && px + tam > hb.x &&
            py < hb.y + hb.h && py + tam > hb.y)
            return false;
    }
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  moverHaciaConEvacion — exactamente la lógica que describiste:
//
//  1. ¿Puedo ir directo (diagonal)?  → ir directo
//  2. ¿Puedo mover en X solamente?   → deslizar por X
//  3. ¿Puedo mover en Y solamente?   → deslizar por Y
//  4. ¿Ninguno? → probar perpendicular izquierda y derecha
//  5. ¿Nada? → frenar (stuck detector tomará el relevo)
// ════════════════════════════════════════════════════════════════════════════
// moverHaciaConEvacion igual que antes pero sin el parámetro paredes
// void RobotSeguridad::moverHaciaConEvacion(float tx, float ty, float dt)
// {
//     // ── PROTECCIÓN: Si el objetivo es un fantasma de la memoria, abortamos
//     if (!std::isfinite(tx) || !std::isfinite(ty)) {
//         Vx = 0.f; Vy = 0.f;
//         return;
//     }
//     float velActual = (estado == EstadoAgente::PERSECUCION)
//     ? velPersecucion : velPatrulla;
//     float dx = tx - x, dy = ty - y;
//     float dist = std::sqrt(dx*dx + dy*dy);
//     //if (dist < 1.f) { Vx = 0.f; Vy = 0.f; return; }
//     // ── PROTECCIÓN: Validar que la distancia sea real y suficiente
//     if (!std::isfinite(dist) || dist < 1.f) {
//         Vx = 0.f; Vy = 0.f;
//         return;
//     }

//     float nx = dx / dist, ny = dy / dist;
//     const float TAM = 32.f, SONDA = TAM * 1.5f;

//     bool diagLibre = posicionLibre(x + nx*SONDA, y + ny*SONDA, TAM);
//     bool xLibre    = posicionLibre(x + nx*SONDA, y,            TAM);
//     bool yLibre    = posicionLibre(x,            y + ny*SONDA, TAM);

//     float targetVx, targetVy;

//     if (diagLibre)
//     { targetVx = nx * velActual; targetVy = ny * velActual; }
//     else if (xLibre && !yLibre)
//     { targetVx = nx * velActual; targetVy = 0.f; }
//     else if (yLibre && !xLibre)
//     { targetVx = 0.f;            targetVy = ny * velActual; }
//     else if (xLibre && yLibre)
//     {
//         if (std::abs(dx) >= std::abs(dy))
//         { targetVx = nx * velActual; targetVy = 0.f; }
//         else
//         { targetVx = 0.f;            targetVy = ny * velActual; }
//     }
//     else
//     {
//         float p1x = -ny, p1y =  nx;
//         float p2x =  ny, p2y = -nx;
//         if (posicionLibre(x + p1x*SONDA, y + p1y*SONDA, TAM))
//         { targetVx = p1x * velActual; targetVy = p1y * velActual; }
//         else if (posicionLibre(x + p2x*SONDA, y + p2y*SONDA, TAM))
//         { targetVx = p2x * velActual; targetVy = p2y * velActual; }
//         else
//         { Vx = 0.f; Vy = 0.f; return; }
//     }


//     //qDebug() << "mostrando robot antes de aplicarInercia en moverhaciaE: " << x << " y: " << y;
//     GestorFisicas::aplicarInercia(Vx, targetVx, dt);
//     GestorFisicas::aplicarInercia(Vy, targetVy, dt);
//     x += Vx * dt;
//     y += Vy * dt;
//     //qDebug() << "mostrando robot despues de aplicarInercia MoverhaciaE: " << x << " y: " << y;
// }
void RobotSeguridad::moverHaciaConEvacion(float tx, float ty, float dt)
{
    if (!std::isfinite(tx) || !std::isfinite(ty)) {
        Vx = 0.f; Vy = 0.f;
        return;
    }

    float velActual = (estado == EstadoAgente::PERSECUCION)
                          ? velPersecucion : velPatrulla;

    float dx = tx - x, dy = ty - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (!std::isfinite(dist) || dist < 1.f) {
        Vx = 0.f; Vy = 0.f;
        return;
    }

    float nx = dx / dist, ny = dy / dist;
    const float TAM = 32.f;

    //FIX 1: Sonda adaptativa
    // Si el robot está más cerca que TAM*1.5 del objetivo, la sonda se acorta
    // al propio destino — evita detectar paredes que están DESPUÉS del waypoint.
    const float SONDA = std::min(TAM * 1.5f, dist);

    bool diagLibre = posicionLibre(x + nx*SONDA, y + ny*SONDA, TAM);
    bool xLibre    = posicionLibre(x + nx*SONDA, y,            TAM);
    bool yLibre    = posicionLibre(x,            y + ny*SONDA, TAM);

    float targetVx, targetVy;

    if (diagLibre)
    {
        targetVx = nx * velActual;
        targetVy = ny * velActual;
    }
    // Guard de componente cero
    // Deslizar en X solo tiene sentido si hay componente horizontal real (nx ≠ 0).
    // Si nx ≈ 0 (movimiento puramente vertical), targetVx = 0 → robot parado.
    // En ese caso caer directo a las perpendiculares.
    else if (xLibre && !yLibre && std::abs(nx) > 0.05f)
    {
        targetVx = nx * velActual;
        targetVy = 0.f;
    }
    else if (yLibre && !xLibre && std::abs(ny) > 0.05f)
    {
        targetVx = 0.f;
        targetVy = ny * velActual;
    }
    else if (xLibre && yLibre)
    {
        // Ambos ejes libres: preferir el de mayor delta,
        // pero solo si el componente correspondiente es significativo
        if (std::abs(dx) >= std::abs(dy) && std::abs(nx) > 0.05f)
        { targetVx = nx * velActual; targetVy = 0.f; }
        else if (std::abs(ny) > 0.05f)
        { targetVx = 0.f; targetVy = ny * velActual; }
        else
        { targetVx = nx * velActual; targetVy = ny * velActual; }
    }
    else
    {
        // Camino directo bloqueado Y componente demasiado pequeño:
        // probar perpendicular izquierda y derecha
        float p1x = -ny, p1y =  nx;
        float p2x =  ny, p2y = -nx;
        if (posicionLibre(x + p1x*SONDA, y + p1y*SONDA, TAM))
        { targetVx = p1x * velActual; targetVy = p1y * velActual; }
        else if (posicionLibre(x + p2x*SONDA, y + p2y*SONDA, TAM))
        { targetVx = p2x * velActual; targetVy = p2y * velActual; }
        else
        { Vx = 0.f; Vy = 0.f; return; }
    }

    GestorFisicas::aplicarInercia(Vx, targetVx, dt);
    GestorFisicas::aplicarInercia(Vy, targetVy, dt);
    x += Vx * dt;
    y += Vy * dt;
}

// TICK (método completo para el nivel)
// void RobotSeguridad::tick(float jx, float jy, float dt)
// {
//     percibir(jx, jy);
//     razonar();
//     actuar(dt);
// }

// void RobotSeguridad::tick(float jx, float jy, float dt, bool jugadorOculto)
// {
//     percibir(jx, jy);
//     razonar(jugadorOculto);
//     actuar(dt);
// }

void RobotSeguridad::tick(float jx, float jy, float dt,
                          bool jugadorOculto,
                          const std::vector<Hitbox>& paredes)
{
    paredesCache = paredes;   // ← disponible para actuar() sin cambiar su firma
    percibir(jx, jy);
    razonar(jugadorOculto);
    actuar(dt);               // ← firma original intacta, override válido
}

// APRENDIZAJE: actualizar waypoints
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




