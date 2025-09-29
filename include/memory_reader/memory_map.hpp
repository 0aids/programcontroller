#pragma once
#include "memory_reader/globals.hpp"
#include "memory_reader/properties.hpp"
#include <string>
#include <unordered_map>

namespace MemoryAnalysis {

    class MemoryMap {
      private:
        const pid_t                             m_pid;
        const std::string                       m_mapLocation;
        RegionPropertiesList                    m_regionProperties_l;
        std::unordered_map<std::string, size_t> m_nameToIndex_l;

      public:
        // Supply a pid and it will generate the mapLocation
        MemoryMap(pid_t pid);

        // In the case you want to read from a custom map
        MemoryMap(const std::string& mapLocation);

        e_ErrorTypes                  readMaps();

        RegionPropertiesList          getPropertiesList();

        MemoryRegionProperties&       operator[](const size_t& index);

        MemoryRegionProperties&       operator[](const std::string& name);

        const MemoryRegionProperties& operator[](const size_t& index) const;

        const MemoryRegionProperties& operator[](const std::string& name) const;
    };
} // namespace MemoryAnalysis
