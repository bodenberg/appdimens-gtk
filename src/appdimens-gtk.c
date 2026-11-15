/* AppDimens for GTK 3 / GTK 4 — implementation. Apache-2.0. */
#include "appdimens/appdimens-gtk.h"

#include <math.h>
#include <string.h>

#define AD_GTK4 (GTK_MAJOR_VERSION >= 4)

typedef struct {
    guint id;
    char *template_css;
    GtkCssProvider *provider;
} AdCss;

struct _AppDimensScope {
    GObject parent_instance;
    GtkWidget *widget;           /* weak */
    GdkFrameClock *clock;        /* strong while connected */
    gulong clock_handler;
    gulong realize_handler, unrealize_handler, scale_handler;
    GtkSettings *settings;       /* weak-ish, owned by display */
    gulong xft_handler;
    ad_metrics metrics[2];       /* double buffer: readers keep a stable pointer for one frame */
    guint front;
    GPtrArray *css;              /* AdCss* */
    guint next_css_id;
    double font_scale_override;
    ad_ui_mode ui_mode;
    ad_real prev_w, prev_h;      /* size published before the current one (oscillation guard) */
};

/* Sizes within this many logical px of the current one that flip back to the previous
 * value are treated as layout feedback (e.g. CSS padding changing the measured size) and
 * ignored. A genuinely new size always publishes. */
#define AD_PINGPONG_PX 4

enum { SIGNAL_CHANGED, N_SIGNALS };
static guint signals[N_SIGNALS];

#if GLIB_CHECK_VERSION(2, 70, 0)
G_DEFINE_FINAL_TYPE(AppDimensScope, appdimens_scope, G_TYPE_OBJECT)
#else
G_DEFINE_TYPE(AppDimensScope, appdimens_scope, G_TYPE_OBJECT)
#endif

static void ad_css_free(gpointer p) {
    AdCss *c = p;
    if (c->provider) {
#if AD_GTK4
        GdkDisplay *d = gdk_display_get_default();
        if (d) gtk_style_context_remove_provider_for_display(d, GTK_STYLE_PROVIDER(c->provider));
#else
        GdkScreen *s = gdk_screen_get_default();
        if (s) gtk_style_context_remove_provider_for_screen(s, GTK_STYLE_PROVIDER(c->provider));
#endif
        g_object_unref(c->provider);
    }
    g_free(c->template_css);
    g_free(c);
}

/* ── CSS expansion ─────────────────────────────────────────────────────────────── */
static gboolean is_ident(char c) {
    return g_ascii_isalnum(c) || c == '_' || c == '-';
}

char *appdimens_expand_css(const ad_metrics *m, const char *src) {
    g_return_val_if_fail(m != NULL && src != NULL, NULL);
    GString *out = g_string_sized_new(strlen(src) + 64);
    const size_t n = strlen(src);
    size_t i = 0;
    gboolean in_hex = FALSE;
    while (i < n) {
        const char c = src[i];
        if (c == '#') { in_hex = TRUE; g_string_append_c(out, c); i++; continue; }
        if (in_hex && !is_ident(c)) in_hex = FALSE;
        const gboolean starts_number = (g_ascii_isdigit(c) || (c == '.' && i + 1 < n && g_ascii_isdigit(src[i + 1]))) &&
                                       (i == 0 || (!is_ident(src[i - 1]) || src[i - 1] == '-'));
        if (!in_hex && starts_number) {
            /* number span */
            size_t j = i;
            while (j < n && (g_ascii_isdigit(src[j]) || src[j] == '.')) j++;
            size_t k = j;
            while (k < n && g_ascii_isalpha(src[k])) k++;
            if (k > j && (k >= n || !is_ident(src[k]))) {
                ad_token tok;
                const char *t = src + j;
                const size_t tl = k - j;
                const gboolean space = (tl == 6 && (!strncmp(t, "spaceW", 6) || !strncmp(t, "spaceH", 6))) ||
                                       (tl == 7 && !strncmp(t, "spaceSw", 7));
                if (space || ad_parse_token_n(t, tl, &tok)) {
                    ad_real v = 0;
                    size_t used = 0;
                    if (ad_dimen_n(m, src + i, k - i, &v, &used) && used == k - i) {
                        char buf[G_ASCII_DTOSTR_BUF_SIZE];
                        g_ascii_formatd(buf, sizeof buf, "%.2f", (double)v);
                        g_string_append(out, buf);
                        g_string_append(out, "px");
                        i = k;
                        continue;
                    }
                }
            }
            g_string_append_len(out, src + i, (gssize)(k - i));
            i = k;
            continue;
        }
        g_string_append_c(out, c);
        i++;
    }
    return g_string_free(out, FALSE);
}

static void ad_css_load(AdCss *c, const ad_metrics *m) {
    char *css = appdimens_expand_css(m, c->template_css);
#if AD_GTK4
#if GTK_CHECK_VERSION(4, 12, 0)
    gtk_css_provider_load_from_string(c->provider, css);
#else
    gtk_css_provider_load_from_data(c->provider, css, -1);
#endif
#else
    gtk_css_provider_load_from_data(c->provider, css, -1, NULL);
#endif
    g_free(css);
}

/* ── Snapshot publication ──────────────────────────────────────────────────────── */
static gboolean publish(AppDimensScope *self, const ad_input *in) {
    const guint back = self->front ^ 1u;
    ad_metrics_init(&self->metrics[back], in);
    if (ad_metrics_same(&self->metrics[back], &self->metrics[self->front])) return FALSE;
    self->metrics[back].generation = self->metrics[self->front].generation + 1;
    self->front = back;
    for (guint i = 0; i < self->css->len; i++) ad_css_load(g_ptr_array_index(self->css, i), &self->metrics[self->front]);
    g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
    return TRUE;
}

static double read_font_scale(AppDimensScope *self) {
    if (self->font_scale_override > 0) return self->font_scale_override;
    GtkSettings *s = self->widget ? gtk_widget_get_settings(self->widget) : gtk_settings_get_default();
    if (!s) return 1.0;
    int xft = 0;
    g_object_get(s, "gtk-xft-dpi", &xft, NULL);
    return xft > 0 ? (xft / 1024.0) / 96.0 : 1.0;
}

static void read_widget(AppDimensScope *self, ad_input *in) {
    ad_input_default(in);
    in->ui_mode = self->ui_mode;
    in->font_scale = (ad_real)read_font_scale(self);
    GtkWidget *w = self->widget;
    if (!w) return;
    double scale = gtk_widget_get_scale_factor(w);
#if AD_GTK4
    in->width = (ad_real)gtk_widget_get_width(w);
    in->height = (ad_real)gtk_widget_get_height(w);
    GtkNative *native = gtk_widget_get_native(w);
    GdkSurface *surface = native ? gtk_native_get_surface(native) : NULL;
#if GTK_CHECK_VERSION(4, 12, 0)
    if (surface) scale = gdk_surface_get_scale(surface);
#endif
    if (surface && GDK_IS_TOPLEVEL(surface)) {
        GdkToplevelState st = gdk_toplevel_get_state(GDK_TOPLEVEL(surface));
        in->multi_window = (st & GDK_TOPLEVEL_STATE_TILED) && !(st & (GDK_TOPLEVEL_STATE_MAXIMIZED | GDK_TOPLEVEL_STATE_FULLSCREEN));
    }
    GdkMonitor *mon = surface ? gdk_display_get_monitor_at_surface(gtk_widget_get_display(w), surface) : NULL;
#else
    in->width = (ad_real)gtk_widget_get_allocated_width(w);
    in->height = (ad_real)gtk_widget_get_allocated_height(w);
    GdkWindow *win = gtk_widget_get_window(w);
    if (win) {
        GdkWindowState st = gdk_window_get_state(gdk_window_get_toplevel(win));
        in->multi_window = (st & GDK_WINDOW_STATE_TILED) && !(st & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN));
    }
    GdkMonitor *mon = win ? gdk_display_get_monitor_at_window(gtk_widget_get_display(w), win) : NULL;
#endif
    in->density = (ad_real)(scale > 0 ? scale : 1.0);
    if (mon) {
        GdkRectangle g;
        gdk_monitor_get_geometry(mon, &g);
        const int mm = gdk_monitor_get_width_mm(mon);
        if (mm > 0 && g.width > 0) in->dpi = (ad_real)(g.width * scale / (mm / 25.4));
    }
    /* Before the first allocation GTK reports 0/1 — keep the reference snapshot. */
    if (in->width <= 1 || in->height <= 1) {
        in->width = 0;
        in->height = 0;
    }
}

gboolean appdimens_scope_refresh(AppDimensScope *self) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self), FALSE);
    ad_input in;
    read_widget(self, &in);
    if (in.width <= 0) return FALSE;
    const ad_input *cur = &self->metrics[self->front].in;
    const gboolean only_size = in.density == cur->density && in.font_scale == cur->font_scale &&
                               in.dpi == cur->dpi && in.multi_window == cur->multi_window;
    if (only_size && in.width == self->prev_w && in.height == self->prev_h &&
        fabs((double)(in.width - cur->width)) <= AD_PINGPONG_PX && fabs((double)(in.height - cur->height)) <= AD_PINGPONG_PX)
        return FALSE; /* A → B → A feedback: keep B */
    const ad_real ow = cur->width, oh = cur->height;
    if (!publish(self, &in)) return FALSE;
    self->prev_w = ow;
    self->prev_h = oh;
    return TRUE;
}

static void on_after_paint(GdkFrameClock *clock, gpointer user) {
    (void)clock;
    appdimens_scope_refresh(APPDIMENS_SCOPE(user));
}

static void disconnect_clock(AppDimensScope *self) {
    if (self->clock) {
        g_signal_handler_disconnect(self->clock, self->clock_handler);
        g_clear_object(&self->clock);
        self->clock_handler = 0;
    }
}

static void on_realize(GtkWidget *w, gpointer user) {
    AppDimensScope *self = APPDIMENS_SCOPE(user);
    disconnect_clock(self);
    GdkFrameClock *clock = gtk_widget_get_frame_clock(w);
    if (clock) {
        self->clock = g_object_ref(clock);
        self->clock_handler = g_signal_connect(clock, "after-paint", G_CALLBACK(on_after_paint), self);
    }
    appdimens_scope_refresh(self);
}

static void on_unrealize(GtkWidget *w, gpointer user) {
    (void)w;
    disconnect_clock(APPDIMENS_SCOPE(user));
}

static void on_notify_refresh(GObject *o, GParamSpec *p, gpointer user) {
    (void)o; (void)p;
    appdimens_scope_refresh(APPDIMENS_SCOPE(user));
}

static void on_widget_gone(gpointer data, GObject *where) {
    (void)where;
    AppDimensScope *self = data;
    self->widget = NULL;
    self->realize_handler = self->unrealize_handler = self->scale_handler = 0;
    disconnect_clock(self);
}

static void detach(AppDimensScope *self) {
    disconnect_clock(self);
    if (self->settings && self->xft_handler) g_signal_handler_disconnect(self->settings, self->xft_handler);
    self->xft_handler = 0;
    self->settings = NULL;
    if (self->widget) {
        g_signal_handler_disconnect(self->widget, self->realize_handler);
        g_signal_handler_disconnect(self->widget, self->unrealize_handler);
        g_signal_handler_disconnect(self->widget, self->scale_handler);
        g_object_weak_unref(G_OBJECT(self->widget), on_widget_gone, self);
        self->widget = NULL;
    }
}

void appdimens_scope_attach(AppDimensScope *self, GtkWidget *widget) {
    g_return_if_fail(APPDIMENS_IS_SCOPE(self));
    g_return_if_fail(widget == NULL || GTK_IS_WIDGET(widget));
    if (self->widget == widget) return;
    detach(self);
    if (!widget) return;
    self->widget = widget;
    g_object_weak_ref(G_OBJECT(widget), on_widget_gone, self);
    self->realize_handler = g_signal_connect(widget, "realize", G_CALLBACK(on_realize), self);
    self->unrealize_handler = g_signal_connect(widget, "unrealize", G_CALLBACK(on_unrealize), self);
    self->scale_handler = g_signal_connect(widget, "notify::scale-factor", G_CALLBACK(on_notify_refresh), self);
    self->settings = gtk_widget_get_settings(widget);
    if (self->settings)
        self->xft_handler = g_signal_connect(self->settings, "notify::gtk-xft-dpi", G_CALLBACK(on_notify_refresh), self);
    if (gtk_widget_get_realized(widget)) on_realize(widget, self);
}

GtkWidget *appdimens_scope_get_widget(AppDimensScope *self) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self), NULL);
    return self->widget;
}

void appdimens_scope_set_input(AppDimensScope *self, const ad_input *input) {
    g_return_if_fail(APPDIMENS_IS_SCOPE(self) && input != NULL);
    publish(self, input);
}

const ad_metrics *appdimens_scope_get_metrics(AppDimensScope *self) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self), NULL);
    return &self->metrics[self->front];
}

double appdimens_scope_dimen(AppDimensScope *self, const char *expr, double fallback) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self) && expr != NULL, fallback);
    ad_real v;
    return ad_dimen(&self->metrics[self->front], expr, &v) ? (double)v : fallback;
}

int appdimens_scope_px(AppDimensScope *self, const char *expr) {
    return (int)lround(appdimens_scope_dimen(self, expr, 0.0));
}

double appdimens_scope_resolve(AppDimensScope *self, const char *token, double value) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self) && token != NULL, value);
    ad_token t;
    if (!ad_parse_token(token, &t)) return value;
    return (double)ad_token_resolve(&self->metrics[self->front], &t, (ad_real)value);
}

int appdimens_scope_pango_size(AppDimensScope *self, const char *token, double value) {
    return (int)lround(appdimens_scope_resolve(self, token, value) * PANGO_SCALE);
}

guint appdimens_scope_add_css(AppDimensScope *self, const char *css_template) {
    g_return_val_if_fail(APPDIMENS_IS_SCOPE(self) && css_template != NULL, 0);
    AdCss *c = g_new0(AdCss, 1);
    c->id = ++self->next_css_id;
    c->template_css = g_strdup(css_template);
    c->provider = gtk_css_provider_new();
    ad_css_load(c, &self->metrics[self->front]);
#if AD_GTK4
    GdkDisplay *d = self->widget ? gtk_widget_get_display(self->widget) : gdk_display_get_default();
    if (d) gtk_style_context_add_provider_for_display(d, GTK_STYLE_PROVIDER(c->provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#else
    GdkScreen *s = self->widget ? gtk_widget_get_screen(self->widget) : gdk_screen_get_default();
    if (s) gtk_style_context_add_provider_for_screen(s, GTK_STYLE_PROVIDER(c->provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif
    g_ptr_array_add(self->css, c);
    return c->id;
}

void appdimens_scope_remove_css(AppDimensScope *self, guint id) {
    g_return_if_fail(APPDIMENS_IS_SCOPE(self));
    for (guint i = 0; i < self->css->len; i++) {
        AdCss *c = g_ptr_array_index(self->css, i);
        if (c->id == id) { g_ptr_array_remove_index(self->css, i); return; }
    }
}

void appdimens_scope_set_font_scale_override(AppDimensScope *self, double font_scale) {
    g_return_if_fail(APPDIMENS_IS_SCOPE(self));
    self->font_scale_override = font_scale;
    if (self->widget) appdimens_scope_refresh(self);
    else {
        ad_input in = self->metrics[self->front].in;
        in.font_scale = (ad_real)(font_scale > 0 ? font_scale : 1.0);
        publish(self, &in);
    }
}

void appdimens_scope_set_ui_mode(AppDimensScope *self, ad_ui_mode mode) {
    g_return_if_fail(APPDIMENS_IS_SCOPE(self));
    self->ui_mode = mode;
    ad_input in = self->metrics[self->front].in;
    in.ui_mode = mode;
    publish(self, &in);
}

/* ── GObject boilerplate ───────────────────────────────────────────────────────── */
static void appdimens_scope_dispose(GObject *o) {
    AppDimensScope *self = APPDIMENS_SCOPE(o);
    detach(self);
    if (self->css) g_ptr_array_set_size(self->css, 0);
    G_OBJECT_CLASS(appdimens_scope_parent_class)->dispose(o);
}

static void appdimens_scope_finalize(GObject *o) {
    AppDimensScope *self = APPDIMENS_SCOPE(o);
    g_ptr_array_unref(self->css);
    G_OBJECT_CLASS(appdimens_scope_parent_class)->finalize(o);
}

static void appdimens_scope_class_init(AppDimensScopeClass *klass) {
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->dispose = appdimens_scope_dispose;
    oc->finalize = appdimens_scope_finalize;
    /**
     * AppDimensScope::changed:
     * Emitted after a new snapshot is published (resize, scale, DPI, text scale, tiling).
     */
    signals[SIGNAL_CHANGED] = g_signal_new("changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL, G_TYPE_NONE, 0);
}

static void appdimens_scope_init(AppDimensScope *self) {
    ad_input in;
    ad_input_default(&in);
    self->ui_mode = AD_UI_DESK;
    in.ui_mode = self->ui_mode;
    ad_metrics_init(&self->metrics[0], &in);
    self->metrics[1] = self->metrics[0];
    self->front = 0;
    self->css = g_ptr_array_new_with_free_func(ad_css_free);
}

AppDimensScope *appdimens_scope_new(void) {
    return g_object_new(APPDIMENS_TYPE_SCOPE, NULL);
}

AppDimensScope *appdimens_scope_new_for_widget(GtkWidget *widget) {
    AppDimensScope *s = appdimens_scope_new();
    appdimens_scope_attach(s, widget);
    return s;
}
