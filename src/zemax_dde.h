#ifndef ZEMAX_DDE_H
#define ZEMAX_DDE_H

#ifdef __cplusplus
extern "C" {
#endif

const char* send_zemax_request(const char* item);
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int initialize_dde(HWND hwnd);
DWORD WINAPI DDEMessageThread(LPVOID lpParam);

#ifdef __cplusplus
}
#endif

#endif