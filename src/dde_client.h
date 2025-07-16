#ifndef DDE_CLIENT_H
#define DDE_CLIENT_H

#include <windows.h>
#include <string>

namespace ZemaxDDE {
    void initiateDDE(HWND hwnd);
    double getSurfaceRadius(HWND hwnd, int surfaceNumber);
    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void terminateDDE();
}

#endif // DDE_CLIENT_H
