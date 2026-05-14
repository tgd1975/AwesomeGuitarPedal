#pragma once
#include <cstdint>
#include <cstring>

/**
 * @brief Name → pin lookup built from the hardware config's pinNames map.
 *
 * EPIC-029 / TASK-380. The hardware config stores pinNames as
 * pin → name (one role per physical pin). Profile actions look up the
 * other direction — given a role name, what physical pin? Rather than
 * walk the on-disk map per button press, the firmware builds this
 * inverse table at config-load and reuses it.
 *
 * Storage is a fixed-capacity flat array of (name, pin) pairs. Capacity
 * picked for the v1 standard set in pin-names.schema.json (17 names)
 * plus headroom: 32 entries is comfortably more than any realistic
 * hardware config maps. Names are copied (not borrowed) so the
 * underlying JSON document can be freed after load.
 *
 * Names are case-sensitive (lowercase snake_case, by convention from
 * pin-names.schema.json).
 */
class PinNameTable
{
public:
    static constexpr uint8_t kMaxEntries = 32;
    static constexpr uint8_t kMaxNameLen = 32;
    static constexpr uint8_t kUnresolved = 0xFF;

    PinNameTable() = default;

    void clear() { count_ = 0; }

    /// Insert a (name, pin) mapping. Returns false if the table is
    /// full, the name is too long, or `name` is null/empty. Earlier
    /// entries with the same pin (or the same name) are NOT removed —
    /// duplicates are tolerated, and lookup returns the first match
    /// (insertion-order, lowest-numbered pin first if the caller
    /// inserts pins in ascending order). The TASK-378 decision was to
    /// treat duplicates as a warning, not an error.
    bool insert(const char* name, uint8_t pin)
    {
        if (count_ >= kMaxEntries || name == nullptr || name[0] == '\0')
        {
            return false;
        }
        size_t len = std::strlen(name);
        if (len >= kMaxNameLen)
        {
            return false;
        }
        std::memcpy(entries_[count_].name, name, len);
        entries_[count_].name[len] = '\0';
        entries_[count_].pin = pin;
        ++count_;
        return true;
    }

    /// Look up *name* and return the mapped pin, or kUnresolved when
    /// the name is not in the table. *name* must be NUL-terminated.
    uint8_t lookup(const char* name) const
    {
        if (name == nullptr)
        {
            return kUnresolved;
        }
        for (uint8_t i = 0; i < count_; ++i)
        {
            if (std::strncmp(entries_[i].name, name, kMaxNameLen) == 0)
            {
                return entries_[i].pin;
            }
        }
        return kUnresolved;
    }

    uint8_t size() const { return count_; }

private:
    struct Entry
    {
        char name[kMaxNameLen];
        uint8_t pin;
    };
    Entry entries_[kMaxEntries]{};
    uint8_t count_ = 0;
};

/**
 * @brief Global pin-name lookup table.
 *
 * Populated by loadHardwareConfigFromJson() at boot. Cleared and
 * rebuilt on every config reload. Consumed by ConfigLoader's
 * Pin*Action parser to resolve named pin references in profiles.
 *
 * Non-const by design: clear()/insert() mutate the shared instance at
 * each boot from a single owner (pedal_config.cpp's load path). The
 * existing globals in this codebase (hardwareConfig, g_logger, etc.)
 * follow the same pattern.
 */
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern PinNameTable g_pinNameTable;
