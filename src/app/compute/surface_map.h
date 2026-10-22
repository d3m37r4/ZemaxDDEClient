#pragma once

#include <optional>
#include <vector>

#include "app/compute/sag_sampling.h"
#include "app/models/surface_data.h"
#include "app/models/types.h"

namespace app::compute {

    // --- Ограничения карты неравномерностей ---------------------------------
    // Пределы живут здесь, потому что их считают и UI, и сервис: значение
    // одно, правило одно. Каждый предел — следствие своего ограничения.
    //
    // Число точек в профиле берётся из app/compute/sag_sampling.h — это
    // ограничения инструмента Zemax, одинаковые для профиля и для карты.

    // Угловой шаг между секциями, градусы. Ниже 0.001° Zemax всё равно не
    // различит: double даёт ~1e-16 радиана, это ~1e-14 градуса.
    constexpr double MIN_ANGLE_STEP_DEG = 0.001;
    constexpr double MAX_ANGLE_STEP_DEG = 180.0;

    // Бюджет DDE-запросов на одну карту. Профиль стоит (sampling + 2) запросов:
    // два на данные поверхности и по одному на точку. Клиент держит один
    // запрос в полёте, поэтому время ≈ бюджет × RTT (десятки мс на GetSag).
    //
    // 16 млн запросов при 65 точках — это 238 805 секций (шаг 0.00075°), то есть
    // заявленные 0.001° (180 000 секций, ~12 млн запросов, ~190 МБ под профили)
    // достижимы. Меняем это число — меняем предел точности и время расчёта.
    //
    // Второй регулятор скорости — таймаут одного запроса (Preferences → DDE
    // Performance → GetSag Map): он задаёт, сколько ждать сервер до ответа.
    constexpr long long MAX_MAP_REQUESTS = 16'000'000;

    /// План расчёта карты: эффективные значения и оценка нагрузки.
    struct MapPlan {
        double    stepDeg = 0.0;           // эффективный шаг между секциями, градусы
        int       sampling = 0;            // эффективное число точек в профиле
        int       sections = 0;            // число секций (профилей)
        long long estimatedRequests = 0;   // оценка DDE-запросов
        /// true ⇒ эффективные значения отличаются от введённых: шаг или число
        /// точек пришлось поднять до пределов.
        bool      adjusted = false;
    };

    /// План карты по введённым значениям.
    ///
    /// Шаг и число точек ограничиваются пределами, а шаг дополнительно
    /// поднимается так, чтобы оценка запросов не превысила MAX_MAP_REQUESTS:
    /// 0.001° при 65 точках — это 180 000 секций и ~12 млн запросов.
    MapPlan planSurfaceMap(double stepDeg, int sampling);

    class SurfaceMap {
        public:
            std::optional<app::models::MaxPVResult> findMaxPVSection(
                const std::vector<app::models::SurfaceData>& profiles,
                const app::models::SurfaceData& nominalSurfaceData) const;
    };

}
