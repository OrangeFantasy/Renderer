#pragma once

enum class EGpuVendorId
{
    Unknown = -1,
    NotQueried = 0,

    Amd = 0x1002,
    ImgTec = 0x1010,
    Nvidia = 0x10DE,
    Arm = 0x13B5,
    Qualcomm = 0x5143,
    Intel = 0x8086,
};
