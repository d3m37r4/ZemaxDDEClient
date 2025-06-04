#ifndef ZEMAX_DDE_H
#define ZEMAX_DDE_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

std::string send_z_surface(const char* surface, const char* sampling, const char* path);

#ifdef __cplusplus
}
#endif

#endif
