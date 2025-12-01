#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#define LPSTR char*
#define UINT64 uint64_t
#define DWORD uint32_t
#define WORD uint16_t
#define BYTE uint8_t
#define __T(s) s
#endif

#include <core/Backend.h>
#include <driver/CanDriver.h>

class PeakCanInterface;
class GenericCanSetupPage;

class PeakCanDriver : public CanDriver
{
public:
    PeakCanDriver(Backend &backend);

    virtual QString getName();
    virtual bool update();

private:
    PeakCanInterface *createOrUpdateInterface(uint32_t hnd);
    GenericCanSetupPage *setupPage;

};