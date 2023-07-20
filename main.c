#include <glib.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <stdbool.h>
#include <stdio.h>

#define UI_FILE "design.glade"
#define MAX_LINE_LENGTH 256

GtkListStore* liststore;
GtkTreeModelFilter* filter;

GtkEntry* filterEntry;

GtkTreeView* treeView;

GPtrArray* services;

GtkLabel* unit_label;
GtkLabel* unit_description;

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

struct Service {
    char* unit;
    char* load;
    /*
        loaded: The service has been successfully loaded.
        not-found: The service unit file was not found.
        bad-setting: The service unit file has a bad configuration setting.
        error: An error occurred while loading the service.
    */
    char* active;
    /*
        active: The service is currently running.
        inactive: The service is not running.
        activating: The service is in the process of being activated.
        deactivating: The service is in the process of being deactivated.
        failed: The service failed to start or encountered an error.
    */
    char* sub;
    /*
        running: The service is running normally.
        exited: The service has exited.
        waiting: The service is waiting for a specific condition to be met.
        start-pre: The service is in the pre-start state.
        start: The service is in the start state.
        start-post: The service is in the post-start state.
        stop-pre: The service is in the pre-stop state.
        stop: The service is in the stop state.
        stop-post: The service is in the post-stop state.
    */
    char* description;
};

/**
 * Executes a system command and stores the command output in a dynamically allocated memory.
 *
 * Written with phind (https://www.phind.com/agent?cache=clk6itjk4005jl708blm5tvza)
 *
 * @param command The command to execute.
 * @param output A pointer to a character pointer where the command output will be stored.
 *               The memory for the output will be dynamically allocated and should be freed by the caller.
 */
void executeCommand(const char* command, char** output) {
    FILE* fp;
    char buffer[1024];
    size_t outputSize = 0;
    size_t bufferSize = sizeof(buffer);

    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to execute command\n");
        return;
    }

    *output = NULL;

    while (fgets(buffer, bufferSize, fp) != NULL) {
        size_t lineSize = strlen(buffer);

        char* newOutput = realloc(*output, outputSize + lineSize + 1);
        if (newOutput == NULL) {
            fprintf(stderr, "Failed to allocate memory\n");
            free(*output);
            *output = NULL;
            break;
        }

        *output = newOutput;
        memcpy(*output + outputSize, buffer, lineSize);
        outputSize += lineSize;
    }

    if (*output != NULL) {
        (*output)[outputSize] = '\0';
    }

    pclose(fp);
}

char* parse_str(json_t* object, const char* key) {
    if (!json_is_object(object)) {
        fprintf(stderr, "error: object is not a json object\n");
        // json_decref(object);
        return NULL;
    }

    json_t* value = json_object_get(object, key);
    if (!json_is_string(value)) {
        fprintf(stderr, "error: %s is not a string\n", key);
        // json_decref(object);
        return NULL;
    }

    return json_string_value(value);
}

GPtrArray* get_services() {
    GPtrArray* array = g_ptr_array_new();

    char* output = NULL;
    executeCommand("systemctl --output=json list-units --type service --all", &output);

    json_t* root;
    json_error_t error;
    root = json_loads(output, 0, &error);

    free(output);

    if (!root) {
        fprintf(stderr, "error on line %d: %s\n", error.line, error.text);
        return array;
    }

    if (!json_is_array(root)) {
        fprintf(stderr, "error: root must be an array\n");
        json_decref(root);
        return array;
    }

    for (int i = 0; i < json_array_size(root); i++) {
        json_t* data = json_array_get(root, i);
        if (!json_is_object(data)) {
            fprintf(stderr, "error: data must be an object\n");
            json_decref(root);
            return array;
        }

        struct Service* service = malloc(sizeof(struct Service));
        service->unit = parse_str(data, "unit");
        service->load = parse_str(data, "load");
        service->active = parse_str(data, "active");
        service->sub = parse_str(data, "sub");
        service->description = parse_str(data, "description");

        g_ptr_array_add(array, service);
    }

    return array;
}

void on_row_activated(GtkTreeView* self, GtkTreePath* path, GtkTreeViewColumn* column, gpointer user_data) {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(treeView);
    GtkTreeModel* model = gtk_tree_view_get_model(treeView);

    GList* rows = gtk_tree_selection_get_selected_rows(selection, NULL);
    if (rows == NULL) {
        g_list_free(rows);
        return;
    }

    GtkTreePath* selectedPath = (GtkTreePath*)rows->data;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, selectedPath)) {
        g_list_free(rows);
        return;
    }

    gchar* load;
    gchar* active;
    gchar* sub;
    gchar* unit;

    gtk_tree_model_get(model, &iter, 0, &load, 1, &active, 2, &sub, 3, &unit, -1);

    gtk_label_set_text(unit_label, unit);

    int service_index = 0;
    struct Service* service;
    do {
        service = services->pdata[service_index++];
    } while (strcmp(service->unit, unit) != 0 && service_index < services->len);

    gtk_label_set_text(unit_description, service->description);

    g_free(load);
    g_free(active);
    g_free(sub);
    g_free(unit);

    g_list_free(rows);
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
    treeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tree_view"));

    unit_label = GTK_LABEL(gtk_builder_get_object(builder, "unit_label"));
    unit_description = GTK_LABEL(gtk_builder_get_object(builder, "unit_description"));

    services = get_services();

    for (int i = 0; i < services->len; i++) {
        struct Service* s = services->pdata[i];

        GtkTreeIter iter;
        gtk_list_store_append(liststore, &iter);
        gtk_list_store_set(liststore, &iter, 0, s->load, 1, s->active, 2, s->sub, 3, s->unit, -1);
    }

    filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(liststore), NULL));
    gtk_tree_model_filter_set_visible_func(filter, filter_func, (gpointer) "", NULL);

    gtk_tree_view_set_model(treeView, GTK_TREE_MODEL(filter));

    g_signal_connect(G_OBJECT(topWindow), "destroy", gtk_main_quit, NULL);
    g_signal_connect(filterEntry, "changed", G_CALLBACK(on_filter_entry_changed), NULL);
    g_signal_connect(treeView, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(treeView, "cursor-changed", G_CALLBACK(on_row_activated), NULL);

    gtk_widget_show_all(GTK_WIDGET(topWindow));
    gtk_main();

    return 0;
}