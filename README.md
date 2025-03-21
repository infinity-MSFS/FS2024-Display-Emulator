# FS2024 Display Emulator

**Linux prototype for Microsoft Flight Simulator 2024 (FS2024) WASM gauge development**

Windows support maybe if I feel like it

A GUI for testing FS2024 aircraft instrumentation without:

- Constant MSFS reloads

- Full WASM compilation cycles

- Simulator dependencies (runtime dependencies)

___

Uses shared library compilation (*.so/*.dll) in a spec that allows for the same source to be used for both testing
and WASM deployment

## Features

- ✅ Linux-native gauge testing
- ✅ Direct OpenGL rendering
- ✅ Shared library hot-reloading
- ✅ Same codebase for both emulation and WASM deployment
- ✅ ImGUI-based control panel
- 🔄 Windows support (in progress)

## Why This Approach?

- WASM gauges are good, very good. But they are slow to iterate on due to the need for constant simulator reloads and
  WASM compilation cycles.

- This approach allows for native debuggers (GDB/LLDB) to be used directly on gauge code, enabling faster iteration and
  debugging.

- Better resource monitoring and profiling capabilities.

## How to Use

### WASM Project Setup

1. In your existing WASM project, add the compatibility header `include/Emulator.h` to your gauge source files. This
   header
   redefines some rendering functions to use the emulator's OpenGL context instead of the simulator's.

2. define `EMULATOR` in your gauge source files to enable emulator-specific code.
3. compile your gauge as a shared library (`.so` on Linux, `.dll` on Windows) instead of a WASM module. (no changes to
   your existing build system should be needed)
4. Create a `<GAUGE_NAME>.json` next to your shared library, this serves as a replacement for a `panel.cfg` in the sim,
   the schema
   is as follows:

```json
{
  "gauge": {
    "size": {
      "width": 800,
      "height": 600
    },
    "string_params": ""
  }
}
```

### Emulator Setup

1. Clone this repo
2. Build the emulator using `cmake` (see below)
3. Run the emulator and load your gauge using the control panel

## Things to Note

- MSFS does not provide a cross-platform shared library for its SDK functions (its built into the sim), so all bindings
  must be implemented
  manually in the emulator. As a result, only common rendering and simvars are supported. Networking and events are
  planned for future updates but are not currently available.