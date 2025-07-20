#ifndef DDE_CLIENT_H
#define DDE_CLIENT_H

#include <windows.h>
#include <string>
#include <vector>

namespace ZemaxDDE {
    extern std::vector<std::string>* debug_log; // Глобальный указатель на debug_log

    void setDebugLog(std::vector<std::string>& log); // Метод для установки debug_log
    void initiateDDE(HWND hwnd);
    double getSurfaceRadius(HWND hwnd, int surfaceNumber);
    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void terminateDDE();
    void AddDebugLog(const char* message); // Объявление функции
}

#endif // DDE_CLIENT_H

