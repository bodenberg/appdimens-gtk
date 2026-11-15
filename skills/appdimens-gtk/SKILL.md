---
name: appdimens-gtk
description: Use for responsive sizing in GTK 3 / GTK 4 apps (C, PyGObject, GJS, Vala, gtk-rs, libadwaita) and for any C/C++ toolkit that can embed the AppDimens single-header core — whenever the user mentions appdimens-gtk, AppDimensScope, appdimens.h, sdp/ssp tokens in GTK CSS, HiDPI/fractional scaling or text-scaling issues in GTK.
---

# appdimens-gtk — workflow

**Library:** appdimens-gtk 3.2.0 · pkg-config `appdimens-gtk4` / `appdimens-gtk3` · core `appdimens/appdimens.h` (C99).

1. Create one scope per toplevel: `AppDimensScope *ad = appdimens_scope_new_for_widget(win);` keep a ref
   (`g_object_set_data_full(win, "appdimens", ad, g_object_unref)`).
2. Move literal sizes in CSS to tokens via `appdimens_scope_add_css(ad, "...16sdp...14ssp...")`. Prefer `asdp`/`assp`
   on desktop, `sdp`/`ssp` on phones (Phosh), `ftsdp` for canvases.
3. In C: `appdimens_scope_px(ad, "16sdp")` for int setters (margins, spacing, icon sizes);
   `ad_sdp(appdimens_scope_get_metrics(ad), 16)` in draw functions (re-read the pointer each frame).
4. Re-layout in the `changed` signal. Don't poll.
5. Text: `ssp` follows the desktop text-scaling factor; `sem` doesn't; Pango: `appdimens_scope_pango_size`.
6. Validate: at a 300 × 533 window every token returns its input; check a maximized 1080p and a 4K @ 150 % window.

Don'ts: don't multiply by the scale factor yourself (logical px already); don't use `Px` values in CSS (CSS px are logical);
don't call `ad_metrics_init` per frame.

## Custom strategies

Only when the project declares `customStrategies` (or the user asks for their own rule): one
`#define ADC_IMPLEMENTATION` + `#include "appdimens/appdimens_custom.h"`, `adc_load_json(text, err, n)` at start-up, then
`adc_dp(adc_get("brand"), appdimens_scope_get_metrics(scope), 16)` / `adc_sp(...)`. One-off math: `adc_formula("base * pow(sw / 360, 0.8)", err, n)`. Never invent
ids; not available in `AD_FIXED_POINT` builds. Reference: `docs/CUSTOM.md`.
