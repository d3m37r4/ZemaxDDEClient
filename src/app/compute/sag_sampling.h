#pragma once

namespace app::compute {

    // Пределы числа точек в профиле поверхности — ограничения инструмента
    // Zemax «Surface Sag Cross Section»: меньше 33 точек сервер не принимает,
    // больше 16385 — предел самого инструмента. Действуют и для профиля
    // (Surface Profile Inspector), и для секций карты неравномерностей.
    //
    // Сколько запросов это стоит, решает бюджет MAX_MAP_REQUESTS: один профиль
    // стоит (sampling + 2) запросов, секций — 180° / шаг.
    constexpr int MIN_SAG_SAMPLING = 33;
    constexpr int MAX_SAG_SAMPLING = 16385;

}
