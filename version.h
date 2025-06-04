#pragma once

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0
#define APP_VERSION_PRERELEASE "a"

// Автоматический билд-номер на основе даты/времени
#define APP_BUILD_NUMBER ((__DATE__[7] - '0') * 10000000 + \
                         (__DATE__[8] - '0') * 1000000 + \
                         (__DATE__[9] - '0') * 100000 + \
                         (__DATE__[0] == 'J' ? (__DATE__[1] == 'a' ? 1 : __DATE__[2] == 'n' ? 6 : 7) : \
                          __DATE__[0] == 'F' ? 2 : \
                          __DATE__[0] == 'M' ? (__DATE__[2] == 'r' ? 3 : 5) : \
                          __DATE__[0] == 'A' ? (__DATE__[1] == 'p' ? 4 : 8) : \
                          __DATE__[0] == 'S' ? 9 : \
                          __DATE__[0] == 'O' ? 10 : \
                          __DATE__[0] == 'N' ? 11 : 12) * 10000 + \
                         (__TIME__[0] - '0') * 1000 + \
                         (__TIME__[1] - '0') * 100 + \
                         (__TIME__[3] - '0') * 10 + \
                         (__TIME__[4] - '0'))
