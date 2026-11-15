# Changelog

## Unreleased
- Custom scaling strategies: `appdimens_custom.h` (C99 single header, C++ compatible) ports `appdimens-dynamic` library-custom — shared JSON document, text formulas, code factors/formulas, rules, built-ins and presets; `adc_dp/adc_sp/adc_px`; `tests/test_custom.c` reproduces the 540-case cross-platform fixture.

## 3.2.0 — first release
- Portable C99 single-header core `appdimens.h` (float/double, freestanding no-libm, Q16.16 fixed point, C++ compatible).
- `AppDimensScope` GObject for GTK 3.22+ and GTK 4: window size, fractional scale, monitor DPI, text scaling, tiled windows; frame-clock driven updates with oscillation guard.
- Responsive GTK CSS templates (`16sdp` → px), Pango helpers, string expressions.
- Meson (+ GObject Introspection) and CMake builds, pkg-config, CI on Linux/macOS/Windows (core) and GTK 3/4 under ASan/UBSan.
