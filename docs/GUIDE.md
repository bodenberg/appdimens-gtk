# AppDimens GTK — guide

## 1. Pick what to track
Attach the scope to the **toplevel window** (`GtkWindow`/`AdwApplicationWindow`): sizes then follow the window, like
Android's `screenWidthDp`. Attach to an inner widget (e.g. a `GtkPaned` side) to scale a region independently.

## 2. Where values come from

| Input | GTK 4 | GTK 3 |
|-------|-------|-------|
| width/height | `gtk_widget_get_width/height` | `gtk_widget_get_allocated_*` |
| density | `gdk_surface_get_scale` (4.12+, fractional) or `gtk_widget_get_scale_factor` | `gtk_widget_get_scale_factor` |
| dpi | monitor geometry × scale ÷ `width_mm` | same |
| font scale | `GtkSettings:gtk-xft-dpi / 1024 / 96` (GNOME "Large text", KDE font DPI) | same |
| multi-window | `GDK_TOPLEVEL_STATE_TILED` (not maximized/fullscreen) | `GDK_WINDOW_STATE_TILED` |

Override with `appdimens_scope_set_font_scale_override()` and `appdimens_scope_set_ui_mode()` (e.g. `AD_UI_TELEVISION`
for a 10-foot UI, `AD_UI_APPLIANCE` for kiosks).

## 3. CSS templates
`appdimens_scope_add_css()` expands only AppDimens tokens; `px`, `em`, `%`, `pt`, hex colours and selectors are kept.
Two decimals are emitted (`19.20px`), which GTK renders sub-pixel exactly.

## 4. Choosing strategies on desktop
- `sdp`/`ssp` for mobile-first layouts (Phosh, Plasma Mobile).
- **`asdp`/`assp`** for desktop apps: linear up to 480 dp, then logarithmic — a maximized 4K window doesn't get giant buttons.
- `ftsdp` for canvases/games that must never crop, `flsdp` to always cover.
- `sem` for text inside fixed-size badges (ignores text scaling).

## 5. Stability
Updates happen once per painted frame and only when an input changed. If a CSS rule changes the tracked widget's own
size by a few px (A → B → A), the scope keeps B instead of oscillating (`AD_PINGPONG_PX`, 4 px).
