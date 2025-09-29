#pragma once
#include "globals.hpp"
#include "memory_reader/properties.hpp"
#include <memory>
#include <string>
#include <vector>

namespace MemoryAnalysis {

    class RegionSnapshot : public std::vector<char> {
      public:
        RegionSnapshot(long snapshotTime) : m_snapshottedTime(snapshotTime) {};
        const long  m_snapshottedTime;
        std::string toStr();
    };
    class SnapshotList : public std::vector<RegionSnapshot> {};

    class BasicMemoryRegion {
      protected:
        SnapshotList                  m_snaps_l;
        const MemoryRegionProperties* m_properties_p;

      public:
        BasicMemoryRegion() {};
        BasicMemoryRegion(const MemoryRegionProperties* properties);

        e_ErrorTypes           snapshot(const pid_t& pid);

        MemoryRegionProperties getProperties();

        RegionSnapshot&        getLastSnapshot() {
            return this->m_snaps_l.back();
        }

        void clearSnapshots() {
            this->m_snaps_l.clear();
        }

        // Look for differences and changes within the last 2 snapshots.
        // Searches for an entire alignment * 8 bits of which there is *a* change.
        // Input is number of bytes to be aligned by.
        // Output is the alignments which have changes, as an offset from starting
        // address.
        std::vector<uintptr_t> getChanged(uintptr_t alignment);

        // Look for differences and changes within the last 2 snapshots.
        // Searches for an entire alignment * 8 bits of which there is *no* change.
        // Input is number of bytes to be aligned by.
        // Output is the alignments which have changes, as an offset from starting
        // address.
        std::vector<uintptr_t> getUnchanged(uintptr_t alignment);
    };

    class MemorySubRegion;

    class MemoryRegion : public BasicMemoryRegion {
      protected:
        std::vector<std::shared_ptr<MemorySubRegion>> subRegions_l;

      public:
        std::weak_ptr<MemorySubRegion> generateSubRegion(uintptr_t offset, size_t size);
        MemoryRegion(const MemoryRegionProperties* properties);
    };

    // Only sub regions should be polling, as they will be the ones watching small
    // regions of memory.
    class MemorySubRegion : public BasicMemoryRegion {

      protected:
        // We do not own the parent as the child, just take a view.
        const MemoryRegion* m_parentRegion_p;

      public:
        MemorySubRegion(const MemoryRegionProperties* properties, MemoryRegion* parentRegion, uintptr_t offset, size_t size);
    };
} // namespace MemoryAnalysis
