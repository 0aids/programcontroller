#pragma once
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

// A bit map of properties
namespace MemoryAnalysis {

    enum e_Permissions {
        READ    = 1 << 0,
        WRITE   = 1 << 1,
        EXECUTE = 1 << 2,
        PRIVATE = 1 << 3,
    };
    typedef unsigned char PermissionsMask;

    std::string           maskToString(PermissionsMask perms);

    PermissionsMask       charsToMask(char perms[4]);

    struct MemoryRegionProperties {
        const std::string     m_name;
        const uintptr_t       m_start;
        const size_t          m_size;
        const PermissionsMask m_perms;

        MemoryRegionProperties(std::string name, uintptr_t start, size_t size, PermissionsMask perms);

        std::string       toStr();
        const std::string toConstStr() const;
    };
    using RegionPropertiesList = std::vector<MemoryRegionProperties>;
} // namespace MemoryAnalysis
