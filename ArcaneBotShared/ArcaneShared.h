#pragma once

#include <cstdint>


class RuneClient {
private:
    const int32_t ClientID;
public:
    RuneClient() = delete;
    ~RuneClient() = delete;
};


// Define structures shared between user and kernel space
#pragma pack(push, 1)  // Ensure no padding
typedef struct _ArcaneClickData {
    int32_t x;
    int32_t y;
} ArcaneClickData, *PArcaneClickData;

typedef struct _ArcaneKeypressData {
    uint8_t keys[16];  // Fixed size array for keys
    uint32_t count;     // Number of valid keys in the array
} ArcaneKeypressData, *PArcanekeypressData;
#pragma pack(pop)
