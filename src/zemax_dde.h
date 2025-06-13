#ifndef ZEMAX_DDE_H
#define ZEMAX_DDE_H

#ifdef __cplusplus
extern "C" {
#endif

// Глобальные переменные
extern int numsurfs; // Число поверхностей
extern int unitflag; // Единицы измерения

// Структура для хранения данных о системе
typedef struct {
    int numsurfs;
    int units;
    char lensname[100];
    char filename[100];
    int error;
    int vignc;
    double chief_x, chief_y, chief_z; // Координаты главного луча
} SystemData;

// Функции для доступа к данным
SystemData* GetSystemData();
void ResetSystemData();

// Функции для работы с DDE
const char* send_zemax_request(const char* item);
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int initialize_dde(HWND hwnd);
DWORD WINAPI DDEMessageThread(LPVOID lpParam);
int get_system_data();
void close_dde(HWND hwnd);
void PostRequestMessage(char* szItem, HWND hwndServer, HWND hwnd);
void WaitForData(HWND hwnd);
char* GetString(char* szBuffer, int n, char* szSubString);

// Функция для добавления сообщений в лог (реализуется в main.cpp)
void AddDebugLog(const char* message);

#ifdef __cplusplus
}
#endif

#endif
