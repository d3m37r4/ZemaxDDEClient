#ifndef DDE_CLIENT_H
#define DDE_CLIENT_H

#include <windows.h>
#include "logger/logger.h"

namespace ZemaxDDE {
    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void initiateDDE(HWND hwnd);
    void terminateDDE();
    extern Logger logger;
    double getSurfaceRadius(HWND hwnd, int surfaceNumber);
}

#endif // DDE_CLIENT_H
