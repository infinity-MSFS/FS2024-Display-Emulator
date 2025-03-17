# FS2024 Display Gauge Emulator

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