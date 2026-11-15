/* GTK 3 / GTK 4 integration test (run under a display: xvfb-run, Wayland or broadway). */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "appdimens/appdimens-gtk.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)
static int changed = 0;
static void on_changed(AppDimensScope *s, gpointer u) { (void)u; changed++; if (g_getenv("AD_TRACE")) { const ad_metrics *m = appdimens_scope_get_metrics(s); printf("  change %d: %gx%g d=%g fs=%g mw=%d\n", changed, (double)m->in.width, (double)m->in.height, (double)m->in.density, (double)m->in.font_scale, m->in.multi_window); } }

static void pump(int ms) {
    gint64 end = g_get_monotonic_time() + ms * 1000;
    while (g_get_monotonic_time() < end) { g_main_context_iteration(NULL, FALSE); g_usleep(2000); }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    /* 1. pure expansion (no display needed) */
    ad_metrics m; ad_metrics_make(&m, 360, 800, 2, 1);
    char *css = appdimens_expand_css(&m, ".a{padding:16sdp 8hdpLw;color:#1d4ed8;margin:-10sdp;width:50spaceW;border:1px solid;font:14ssp;x:2em;h1{} .b16sdp{}}");
    CHECK(strcmp(css, ".a{padding:19.20px 21.33px;color:#1d4ed8;margin:-12.00px;width:180.00px;border:1px solid;font:16.80px;x:2em;h1{} .b16sdp{}}") == 0, "css=%s", css);
    g_free(css);

#if GTK_MAJOR_VERSION >= 4
    if (!gtk_init_check()) { printf("SKIP: no display\n"); return 77; }
    GtkWidget *win = gtk_window_new();
    gtk_window_set_default_size(GTK_WINDOW(win), 600, 1066);
#else
    if (!gtk_init_check(NULL, NULL)) { printf("SKIP: no display\n"); return 77; }
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 600, 1066);
#endif
    AppDimensScope *ad = appdimens_scope_new_for_widget(win);
    g_signal_connect(ad, "changed", G_CALLBACK(on_changed), NULL);
    appdimens_scope_set_font_scale_override(ad, 1.0);
    guint css_id = appdimens_scope_add_css(ad, "window { padding: 16sdp; }");
#if GTK_MAJOR_VERSION >= 4
    gtk_window_present(GTK_WINDOW(win));
#else
    gtk_widget_show_all(win);
#endif
    pump(600);
    const ad_metrics *mm = appdimens_scope_get_metrics(ad);
    printf("GTK %d: window %gx%g density %g dpi %g fs %g, changes=%d\n", GTK_MAJOR_VERSION,
           (double)mm->in.width, (double)mm->in.height, (double)mm->in.density, (double)mm->in.dpi, (double)mm->in.font_scale, changed);
    CHECK(mm->in.width > 100 && mm->in.height > 100, "window size not tracked");
    CHECK(changed >= 1, "no changed signal");
    CHECK(changed <= 6, "feedback loop: %d changes while idle", changed);
    double expect = 16.0 * fmin(mm->in.width, mm->in.height) / 300.0;
    CHECK(fabs(appdimens_scope_dimen(ad, "16sdp", 0) - expect) < 1e-3, "sdp");
    CHECK(appdimens_scope_px(ad, "16sdp") == (int)lround(expect), "px");
    CHECK(fabs(appdimens_scope_resolve(ad, "asdp", 10) - 10 * mm->table[AD_AUTO][0]) < 1e-4, "resolve");
    CHECK(appdimens_scope_dimen(ad, "bogus", -1) == -1, "fallback");

    int before = changed;
#if GTK_MAJOR_VERSION >= 4
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 500);
#else
    gtk_window_resize(GTK_WINDOW(win), 900, 500);
#endif
    pump(800);
    mm = appdimens_scope_get_metrics(ad);
    printf("after resize: %gx%g, changes=%d\n", (double)mm->in.width, (double)mm->in.height, changed);
    CHECK(changed > before, "resize not observed");
    CHECK(mm->is_landscape, "should be landscape");

    /* manual input + ui mode */
    appdimens_scope_attach(ad, NULL);
    ad_input in; ad_input_default(&in); in.width = 1920; in.height = 1080; in.density = 1.5;
    appdimens_scope_set_input(ad, &in);
    CHECK(fabs(appdimens_scope_dimen(ad, "10sdp", 0) - 36.0) < 1e-3, "manual sdp");
    CHECK(appdimens_scope_pango_size(ad, "ssp", 10) == (int)lround(36.0 * PANGO_SCALE), "pango");
    appdimens_scope_remove_css(ad, css_id);
#if GTK_MAJOR_VERSION >= 4
    gtk_window_destroy(GTK_WINDOW(win));
#else
    gtk_widget_destroy(win);
#endif
    pump(50);
    g_object_unref(ad);
    printf("%s\n", fails ? "FAILED" : "OK");
    return fails != 0;
}
