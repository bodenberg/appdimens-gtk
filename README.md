<div align="center">

# AppDimens for GTK

**Write `16sdp` once. It looks right on a 13" laptop, a 4K monitor at 150 %, an ultrawide, a Steam Deck or a Librem phone.**

[![Release](https://img.shields.io/badge/release-3.2.0-1d4ed8.svg)](CHANGELOG.md)
[![GTK](https://img.shields.io/badge/GTK-3.22%2B%20%7C%204.x-15803d.svg)](#install)
[![C](https://img.shields.io/badge/core-C99%20single%20header-f59e0b.svg)](include/appdimens/appdimens.h)
[![License](https://img.shields.io/badge/license-Apache%202.0-15803d.svg)](LICENSE)

Part of the [AppDimens family](https://github.com/bodenberg/appdimens) — same tokens and math as Android, iOS, Flutter, React Native, Web, .NET, Qt, LVGL, Roku, Python and Games.

🇧🇷 [Resumo em português](#-resumo-em-português)

</div>

## What you get

- **`appdimens.h`** — portable C99 single-header core (also valid C++). 12 strategies, every family token, `"16sdp"` string
  parser, physical units, resize search. No allocation, no globals; one load + one multiply per call.
- **`AppDimensScope`** (GObject) for **GTK 3.22+ and GTK 4** that follows the window size, the scale factor (fractional on
  GTK ≥ 4.12), the monitor DPI, the GNOME/KDE **text scaling factor** (`gtk-xft-dpi`) and tiled/snapped windows (treated as
  multi-window so `i` tokens keep their base value). Updates ride the frame clock (`after-paint`) — no timers, no polling,
  with an oscillation guard against CSS ↔ layout feedback.
- **Responsive CSS**: write `padding: 16sdp; font-size: 14ssp` in GTK CSS; the scope expands it to px and reloads it on every change.
- **GObject Introspection** (`AppDimens-3.gir`): use it from Python (PyGObject), JavaScript (GJS), Vala and Rust (gtk-rs).

## Install

```bash
meson setup build -Dgtk=4          # or -Dgtk=3
ninja -C build && xvfb-run -a meson test -C build
sudo ninja -C build install        # pkg-config: appdimens-gtk4 / appdimens-gtk3
```

CMake: `cmake -B build -DAPPDIMENS_GTK_VERSION=4 && cmake --build build` → targets `AppDimens::gtk`, `AppDimens::core`.
Core only (any toolkit, any OS): copy `include/appdimens/appdimens.h`.

## First lines of code

```c
#include <appdimens/appdimens-gtk.h>

AppDimensScope *ad = appdimens_scope_new_for_widget(window);
appdimens_scope_add_css(ad,
    ".card  { padding: 16sdp; border-radius: 12sdp; }"
    ".title { font-size: 20assp; }"
    "button { min-height: 44asdp; }");

gtk_widget_set_margin_start(box, appdimens_scope_px(ad, "16sdp"));
g_signal_connect(ad, "changed", G_CALLBACK(relayout), NULL);

const ad_metrics *m = appdimens_scope_get_metrics(ad);   /* hot path from the core: */
double pad = ad_sdp(m, 16), title = ad_assp(m, 20), icon = ad_ftsdp_px(m, 24);
```

Python (PyGObject):

```python
gi.require_version("AppDimens", "3")
from gi.repository import AppDimens
ad = AppDimens.Scope.new_for_widget(window)
ad.add_css(".card { padding: 16sdp; }")
label.set_margin_top(ad.px("8sdp"))
```

## Tokens

`<prefix><s|h|w><dp|sp|em>[Px][a|i|ia][Ph|Lh|Pw|Lw][a|i|ia][Px]` — e.g. `sdp`, `ssp`, `asdp` (auto, recommended for
desktop), `hdpLw`, `ftsdp` (fit), `logssp`, `sdpPxia`. C shortcuts: `ad_<prefix><axis><kind>[_a|_i|_ia][_px]`
(`ad_sdp`, `ad_assp_a`, `ad_ftwdp_px`, `ad_hem_ia`); inverters through `ad_dp(m, AD_SCALED, AD_SLOT(AD_VAR_S_PH, AD_FLAG_A), v)`.

| GTK unit | Meaning |
|----------|---------|
| logical px (GTK CSS `px`) | AppDimens `dp` — `sdp` etc. return this |
| device px | `…_px` / `Px` tokens (× scale factor, fractional on GTK ≥ 4.12) |
| Pango units | `appdimens_scope_pango_size(ad, "ssp", 14)` |

## Custom strategies

Your own scaling rule, shared with every AppDimens platform (same JSON document and text formulas),
through the C99 single header `appdimens_custom.h`:

```c
#define ADC_IMPLEMENTATION
#include "appdimens/appdimens_custom.h"
adc_load_json(json_text, err, sizeof err);                       /* ids + "default" */
float pad = adc_dp(adc_get("brand"), appdimens_scope_get_metrics(scope), 16);
adc_strategy *hud = adc_formula("base * pow(sw / 360, 0.8)", err, sizeof err);
```

Reproduces the cross-platform 540-case fixture. Details: [docs/CUSTOM.md](docs/CUSTOM.md).

## Precision, stability, performance

- **Parity**: `tests/test_core.c` checks 10 704 values (12 strategies × 44 slots × 20 screens + tokens + parser) against
  vectors generated from the reference implementation, in **float, double, freestanding no-libm and Q16.16 fixed point**, as C99 and C++.
- **Clean** under `-Wall -Wextra -pedantic -Werror`, `clang -Weverything`, ASan + UBSan; GTK test leak-free (system
  fontconfig excluded).
- **Speed**: building a snapshot (528 factors) ≈ 1–3 µs; a token lookup is one multiply (~1 ns).

Docs: [GTK guide](docs/GUIDE.md) · [Skill](skills/appdimens-gtk/SKILL.md) · [LLMS.txt](LLMS.txt) · [Theory (hub)](https://github.com/bodenberg/appdimens/blob/main/DOCS/THEORY.md)

<!-- appdimens-family:begin (maintained in bodenberg/appdimens tools/family_block.py) -->
## 🌍 AppDimens family

Same token names and the same math on every platform — [hub](https://github.com/bodenberg/appdimens) · [AI agent skills](https://github.com/bodenberg/appdimens-skills) · [custom strategies](https://github.com/bodenberg/appdimens/blob/main/DOCS/CUSTOM.md) (one JSON for every platform).

| Platform | Library | Scaled `16` |
|---|---|---|
| Android — Jetpack Compose, Views, Java | [appdimens-dynamic](https://github.com/bodenberg/appdimens-dynamic) | `16.sdp` · `16.sdp(context)` |
| Android — XML resources | [appdimens-sdps](https://github.com/bodenberg/appdimens-sdps) | `@dimen/_16sdp` |
| Android — XML text sizes | [appdimens-ssps](https://github.com/bodenberg/appdimens-ssps) | `@dimen/_16ssp` |
| Kotlin / Compose Multiplatform | [appdimens-kmp](https://github.com/bodenberg/appdimens-kmp) | `16.sdp` |
| iOS · macOS — SwiftUI, UIKit, AppKit, Metal | [appdimens-ios](https://github.com/bodenberg/appdimens-ios) | `16.sdp` |
| Flutter | [appdimens-flutter](https://github.com/bodenberg/appdimens-flutter) | `16.sdp` |
| React Native / Expo | [appdimens-react-native](https://github.com/bodenberg/appdimens-react-native) | `sdp(16)` |
| Web — HTML/CSS, React, Vue, Svelte, Angular | [appdimens-web](https://github.com/bodenberg/appdimens-web) | `sdp(16)` · `16sdp` |
| .NET — MAUI, WPF, WinForms, WinUI, Android | [appdimens-net-binding](https://github.com/bodenberg/appdimens-net-binding) | `16.Sdp()` · `{sdp:16}` |
| Qt 5/6 — QML, Widgets | [appdimens-qt](https://github.com/bodenberg/appdimens-qt) | `AppDimens.sdp(16)` · `16_sdp` |
| GTK 3/4 and portable C99 | **appdimens-gtk** (this repository) | `16sdp` (CSS) · `ad_sdp(&m, 16)` |
| LVGL 8/9 — MCUs, embedded Linux | [appdimens-lvgl](https://github.com/bodenberg/appdimens-lvgl) | `lv_ad_sdp(16)` |
| Roku — BrightScript / SceneGraph | [appdimens-roku](https://github.com/bodenberg/appdimens-roku) | `sdp(16)` · `ad.sdp(16)` |
| Python — Tkinter, Qt, Kivy, pygame, Blender | [appdimens-py](https://github.com/bodenberg/appdimens-py) | `ad.sdp(16)` |
| Games — Unity, Unreal, Godot, C/C++, .NET, Android | [appdimens-games](https://github.com/bodenberg/appdimens-games) | `64f.ASdp()` · `AppDimens.asdp(64)` · `appdimens::asdp(64)` |

<!-- appdimens-family:end -->

## 🇧🇷 Resumo em português

**AppDimens para GTK** (GTK 3.22+ e GTK 4): escreva `padding: 16sdp; font-size: 14ssp` no CSS do GTK ou use
`appdimens_scope_px(ad, "16sdp")` no C. O `AppDimensScope` acompanha o tamanho da janela, a escala (inclusive fracionária),
o DPI do monitor e a escala de texto do GNOME/KDE, e atualiza tudo sozinho. O núcleo `appdimens.h` (C99, header único)
funciona em qualquer toolkit. Também disponível em Python/GJS/Vala/Rust via GObject Introspection.

## Author

**Jean Bodenberg** — [@bodenberg](https://github.com/bodenberg) · Apache 2.0
