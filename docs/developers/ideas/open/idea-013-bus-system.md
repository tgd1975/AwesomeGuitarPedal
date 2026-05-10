---
id: IDEA-013
title: Bus System
category: hardware
description: Daisy-chain bus for connecting multiple pedals instead of direct controller wiring
---

> **See also:** [idea-013-bus-discussion.md](idea-013-bus-discussion.md) — full concept extract from the design discussion (electrical topology, protocol, node schematic, connector layout)

## Details

Currently, the pedals are connected directly to the controller. This does not scale well for more pedals. The plan is to create a daisy-chain bus for easy extensibility.

### Proposed Daisy-Chain Bus

- **ATtiny-based nodes**: each pedal gets an ATtiny MCU, up to 16 pedals per bus
- **Hardware Abstraction Layer**: unified `PedalInterface` covering both direct GPIO and bus-connected pedals
- **Backward compatible**: existing GPIO deployments need no changes; bus support is additive

### Directory layout (proposed)

```
src/hardware/
├── pedal_interface.h          # Abstract base class
├── gpio_pedal/                # Existing direct GPIO implementation
└── daisy_chain/               # New bus implementation
    ├── daisy_chain.cpp/h
    ├── bus_protocol.h
    └── attiny_node/           # ATtiny node firmware
```

### Configuration

**Build-time selection:**

```ini
[env:direct_gpio]
build_flags = -D PEDAL_CONNECTION=GPIO

[env:daisy_chain]
build_flags = -D PEDAL_CONNECTION=DAISY_CHAIN
```

**Runtime detection:**

```cpp
PedalConnectionType detectAvailableConnection() {
    #ifdef HAS_DAISY_CHAIN_SUPPORT
    if (testDaisyChainBus()) {
        return PedalConnectionType::DAISY_CHAIN;
    }
    #endif
    if (testDirectGPIO()) {
        return PedalConnectionType::GPIO;
    }
    return PedalConnectionType::NONE;
}
```

## MCU Capability Matrix

| MCU Variant   | Direct GPIO | Daisy-Chain Bus | Notes                       |
|---------------|-------------|-----------------|------------------------------|
| Current MCU 1 | Yes         | No              | Needs hardware modification  |
| Current MCU 2 | Yes         | No              | Needs hardware modification  |
| Future MCU A  | Yes         | Yes             | Native support               |
| Future MCU B  | No          | Yes             | Bus-only variant             |

## Implementation Phases

- **Phase 1** (2–4 weeks): Abstract pedal interface, GPIO refactor, bus protocol design, ATtiny prototype
- **Phase 2** (3–5 weeks): Bus driver, node firmware with addressing, discovery protocol, error handling
- **Phase 3** (2–3 weeks): Unified config system, capability detection, testing, documentation

## Error Handling

```cpp
enum class PedalError {
    NONE,
    GPIO_PIN_CONFIG_FAILED,
    DAISY_CHAIN_NO_RESPONSE,
    DAISY_CHAIN_ADDRESS_CONFLICT,
    COMMUNICATION_TIMEOUT,
    INVALID_PEDAL_ID
};
```

## Risks

- Bus protocol complexity — mitigated by thorough design review
- Timing issues — addressed through extensive testing
- Scope creep — managed through phased approach
- Power management — careful electrical design required
