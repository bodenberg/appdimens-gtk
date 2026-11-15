/*
 * AppDimens for GTK 3 and GTK 4 — responsive sizes that follow the window, the monitor
 * scale (integer and fractional), the monitor DPI and the desktop text-scaling factor.
 *
 *   AppDimensScope *ad = appdimens_scope_new_for_widget(GTK_WIDGET(window));
 *   appdimens_scope_add_css(ad, ".card { padding: 16sdp; font-size: 14ssp; }");
 *   int pad = appdimens_scope_px(ad, "16asdp");                // logical px, rounded
 *   double v = ad_sdp(appdimens_scope_get_metrics(ad), 16);     // core hot path
 *
 * AppDimensScope is a GObject (GObject Introspection friendly: PyGObject, GJS, Vala,
 * gtk-rs). Version 3.2.0 · Apache-2.0.
 */
#ifndef APPDIMENS_GTK_H
#define APPDIMENS_GTK_H

#include <gtk/gtk.h>
#include "appdimens.h"

G_BEGIN_DECLS

#define APPDIMENS_TYPE_SCOPE (appdimens_scope_get_type())
G_DECLARE_FINAL_TYPE(AppDimensScope, appdimens_scope, APPDIMENS, SCOPE, GObject)

/** New scope at the reference snapshot (300 × 533, density 1). */
AppDimensScope *appdimens_scope_new(void);

/** New scope that tracks `widget` (usually the toplevel window). */
AppDimensScope *appdimens_scope_new_for_widget(GtkWidget *widget);

/**
 * Tracks `widget`: its allocated size, scale factor (fractional on GTK ≥ 4.12), the
 * monitor DPI, the GtkSettings text scale (gtk-xft-dpi) and the tiled/snapped state
 * (reported as multi-window, so `i` tokens keep their base value). Updates happen on the
 * frame clock's after-paint signal — no polling, no timers. Pass NULL to detach.
 */
void appdimens_scope_attach(AppDimensScope *self, GtkWidget *widget);

/** The widget being tracked, or NULL. */
GtkWidget *appdimens_scope_get_widget(AppDimensScope *self);

/** Publishes a snapshot manually (tests, offscreen rendering, custom toolkits). */
void appdimens_scope_set_input(AppDimensScope *self, const ad_input *input);

/** Current immutable snapshot (valid until the next "changed" emission). */
const ad_metrics *appdimens_scope_get_metrics(AppDimensScope *self);

/** Re-reads the tracked widget now. Returns TRUE when the snapshot changed. */
gboolean appdimens_scope_refresh(AppDimensScope *self);

/**
 * Resolves "<number><token>" ("16sdp", "14assp", "8hdpLw", "50spaceW", "5mm") in logical
 * px. Returns `fallback` when the expression is invalid.
 */
double appdimens_scope_dimen(AppDimensScope *self, const char *expr, double fallback);

/** Same as appdimens_scope_dimen() rounded to an int (for gtk_widget_set_margin_*, …). */
int appdimens_scope_px(AppDimensScope *self, const char *expr);

/** Resolves `value` through `token` ("sdp", "asspa", …). Invalid token → value. */
double appdimens_scope_resolve(AppDimensScope *self, const char *token, double value);

/** Pango size (value in logical px × PANGO_SCALE) for a text token, e.g. ("ssp", 14). */
int appdimens_scope_pango_size(AppDimensScope *self, const char *token, double value);

/**
 * Adds a CSS template whose AppDimens tokens are expanded to px ("padding: 16sdp" →
 * "padding: 19.20px") and re-expanded on every change. Installed for the widget's display
 * (GTK 4) / screen (GTK 3) at GTK_STYLE_PROVIDER_PRIORITY_APPLICATION. Returns an id for
 * appdimens_scope_remove_css().
 */
guint appdimens_scope_add_css(AppDimensScope *self, const char *css_template);
void appdimens_scope_remove_css(AppDimensScope *self, guint id);

/** Override the font scale source (default: GtkSettings gtk-xft-dpi / 96). ≤ 0 = auto. */
void appdimens_scope_set_font_scale_override(AppDimensScope *self, double font_scale);

/** Override the UI mode (default AD_UI_DESK on desktops). */
void appdimens_scope_set_ui_mode(AppDimensScope *self, ad_ui_mode mode);

/**
 * Expands AppDimens tokens inside any text (CSS, markup, config). Plain CSS units (px, em,
 * %, pt…) and hex colours are left untouched. Caller frees the result with g_free().
 */
char *appdimens_expand_css(const ad_metrics *metrics, const char *css_template);

G_END_DECLS

#endif /* APPDIMENS_GTK_H */
