/* AppDimens GTK demo — resize the window, change the scale or the text-scaling factor. */
#include "appdimens/appdimens-gtk.h"

static const char *CSS =
    ".card { padding: 16sdp; border-radius: 12sdp; background: #1d4ed8; color: white; }"
    ".title { font-size: 20assp; font-weight: bold; }"
    ".body { font-size: 14ssp; margin-top: 8sdp; }"
    "button { min-height: 44asdp; padding: 0 16sdp; font-size: 14ssp; }";

static void on_changed(AppDimensScope *ad, gpointer label) {
    const ad_metrics *m = appdimens_scope_get_metrics(ad);
    char *t = g_strdup_printf("%.0f × %.0f · scale %.2f · 16sdp = %.1f px · 16asdp = %.1f px",
                              (double)m->in.width, (double)m->in.height, (double)m->in.density,
                              (double)ad_sdp(m, 16), (double)ad_asdp(m, 16));
    gtk_label_set_text(GTK_LABEL(label), t);
    g_free(t);
}

#if GTK_MAJOR_VERSION >= 4
static void activate(GtkApplication *app, gpointer u) {
    (void)u;
    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, 700);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(box, "card");
    GtkWidget *title = gtk_label_new("AppDimens for GTK");
    gtk_widget_add_css_class(title, "title");
    GtkWidget *body = gtk_label_new("");
    gtk_widget_add_css_class(body, "body");
    gtk_label_set_wrap(GTK_LABEL(body), TRUE);
    gtk_box_append(GTK_BOX(box), title);
    gtk_box_append(GTK_BOX(box), body);
    gtk_box_append(GTK_BOX(box), gtk_button_new_with_label("Continue"));
    gtk_window_set_child(GTK_WINDOW(win), box);
    AppDimensScope *ad = appdimens_scope_new_for_widget(win);
    appdimens_scope_add_css(ad, CSS);
    g_signal_connect(ad, "changed", G_CALLBACK(on_changed), body);
    g_object_set_data_full(G_OBJECT(win), "appdimens", ad, g_object_unref);
    gtk_window_present(GTK_WINDOW(win));
}
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("io.github.bodenberg.AppDimensDemo", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int r = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return r;
}
#else
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, 700);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "card");
    GtkWidget *title = gtk_label_new("AppDimens for GTK");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "title");
    GtkWidget *body = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(body), "body");
    gtk_label_set_line_wrap(GTK_LABEL(body), TRUE);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), body, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_button_new_with_label("Continue"), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(win), box);
    AppDimensScope *ad = appdimens_scope_new_for_widget(win);
    appdimens_scope_add_css(ad, CSS);
    g_signal_connect(ad, "changed", G_CALLBACK(on_changed), body);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(win);
    gtk_main();
    g_object_unref(ad);
    return 0;
}
#endif
