#include <gtk/gtk.h>

#define UI_FILE "design.glade"

GtkBuilder* builder;
GtkWindow* topWindow;

GtkListStore* liststore;

int main(int argc, char* argv[]) {
    GError* err = NULL;

    // GTK+ initialization
    gtk_init(&argc, &argv);

    builder = gtk_builder_new();

    // * UI file should be placed to the same directory as the application
    if (!gtk_builder_add_from_file(builder, UI_FILE, &err)) {
        g_warning("%s\n", err->message);
        g_free(err);
        return 1;
    }

    topWindow = GTK_WINDOW(gtk_builder_get_object(builder, "topWindow"));

    liststore = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));

    g_signal_connect(G_OBJECT(topWindow), "destroy", gtk_main_quit, NULL);

    // freeing up memory
    g_object_unref(G_OBJECT(builder));

    GtkTreeIter iter;
    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter, 0, "загружен", 1, "активен", 2, "запущен", 3, "avachi-daemon.service", -1);

    gtk_widget_show(GTK_WIDGET(topWindow));

    gtk_main();

    return 0;
}