#include <gtk/gtk.h>
#include <stdbool.h>

#define UI_FILE "design.glade"

GtkListStore* liststore;
GtkTreeModelFilter* filter;

GtkEntry* filterEntry;

static gboolean filter_func(GtkTreeModel* model, GtkTreeIter* iter, gpointer data) {
    const gchar* text = gtk_entry_get_text(filterEntry);
    gchar* value;

    gtk_tree_model_get(model, iter, 3, &value, -1); // Get value from the fourth column

    gboolean match = g_strrstr(value, text) != NULL;

    g_free(value);

    return match;
}

void on_filter_entry_changed(GtkEntry* entry, gpointer user_data) {
    gtk_tree_model_filter_refilter(filter);
}

int main(int argc, char* argv[]) {
    GError* err = NULL;

    // GTK+ initialization
    gtk_init(&argc, &argv);

    GtkBuilder* builder = gtk_builder_new();

    // * UI file should be placed to the same directory as the application
    if (!gtk_builder_add_from_file(builder, UI_FILE, &err)) {
        g_warning("%s\n", err->message);
        g_free(err);
        return 1;
    }

    GtkWindow* topWindow = GTK_WINDOW(gtk_builder_get_object(builder, "topWindow"));
    liststore = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));
    filterEntry = GTK_ENTRY(gtk_builder_get_object(builder, "filterEntry"));
    GtkTreeView* treeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tree_view"));

    // GtkTreeIter iter;
    // gtk_list_store_append(liststore, &iter);
    // gtk_list_store_set(liststore, &iter, 0, "загружен", 1, "активен", 2, "запущен", 3, "avachi-daemon.service", -1);

    GtkTreeIter iter0;
    gtk_list_store_append(liststore, &iter0);
    gtk_list_store_set(liststore, &iter0, 0, "00", 1, "01", 2, "02", 3, "03", -1);

    GtkTreeIter iter1;
    gtk_list_store_append(liststore, &iter1);
    gtk_list_store_set(liststore, &iter1, 0, "10", 1, "11", 2, "12", 3, "13", -1);

    GtkTreeIter iter2;
    gtk_list_store_append(liststore, &iter2);
    gtk_list_store_set(liststore, &iter2, 0, "20", 1, "21", 2, "22", 3, "23", -1);

    filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(liststore), NULL));
    gtk_tree_model_filter_set_visible_func(filter, filter_func, (gpointer) "", NULL);

    gtk_tree_view_set_model(treeView, GTK_TREE_MODEL(filter));

    g_signal_connect(G_OBJECT(topWindow), "destroy", gtk_main_quit, NULL);
    g_signal_connect(filterEntry, "changed", G_CALLBACK(on_filter_entry_changed), NULL);

    gtk_widget_show_all(GTK_WIDGET(topWindow));
    gtk_main();

    return 0;
}