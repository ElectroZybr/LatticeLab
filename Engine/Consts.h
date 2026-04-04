#pragma once

#include <numbers>

namespace Consts {
    inline constexpr float Epsilon = 1e-6f;
}

namespace Units {
    // время
    inline constexpr double kTimeUnitToFs = 10.1805; // константа для перевода Dt -> Fs (1 Dt = 10.1805 fs)
    inline constexpr double kTimeUnitToNs = 10.1805e-6; // константа для перевода Dt -> Ns (1 Dt = 0.0000101805 ns)

    // единицы измерения
    inline constexpr double AngstromToNm = 0.1; // константа для перевода ангстрем в нанометры
    inline constexpr double NmToAngstrom = 10.0; // константа для перевода нанометров в ангстремы

    // энергия
    inline constexpr double kEnergyUnitToEv = 1.0; // константа для перевода единиц энергии в электрон-вольты
    inline constexpr double kEvToJ = 1.602176634e-19; // константа для перевода электрон-вольт в джоули
    inline constexpr double kEvToPJ = kEvToJ * 10e12; // константа для перевода электрон-вольт в пико джоули
}
