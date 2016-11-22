#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>

struct Project_Tester
{
	GtkWindow* window;
	GtkHeaderBar* header;
	GtkStack* content;
	GtkListBox* list;
	GtkWidget* back_button;
	GtkWidget* list_refresh_button;
	GtkWidget* project_refresh_button;
	GtkWidget* edit_button;
};

struct Popup
{
	GtkAppChooserDialog* dialog;
	GFile* file;
};

GtkWidget* create_project(const char* project)
{
	GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_wide_handle(GTK_PANED(paned), TRUE);
	GtkWidget* stack = gtk_stack_new();
	GtkWidget* sidebar = gtk_stack_sidebar_new();

	GString* string = g_string_new(project);
	g_string_append(string, "/test");
	if(g_file_test(string->str, G_FILE_TEST_IS_DIR))
	{
		GDir* dir = g_dir_open(string->str, 0, NULL);
		for(const char* name; (name = g_dir_read_name(dir));)
		{
			if(!g_ascii_strncasecmp(name, "test", 4))
			{
				GString* script = g_string_new("ruby ");
				g_string_append(script, string->str);
				g_string_append(script, "/");
				g_string_append(script, name);

				char* result;
				char* error;
				g_spawn_command_line_sync(
					script->str,
					&result,
					&error,
					NULL,
					NULL
				);
				if(!result)
				{
					return gtk_label_new(
						"Ruby not installed or working"
					);
				}

				GtkWidget* vte = vte_terminal_new();
				vte_terminal_set_rewrap_on_resize(
					VTE_TERMINAL(vte),
					TRUE
				);
				if(strlen(result))
				{
					for(int i = 0; i < strlen(result); i++)
					{
						if(result[i] == '\n')
						{
							vte_terminal_feed(
								VTE_TERMINAL(vte), 
								"\r\n",
								2
							);
						}
						else
							vte_terminal_feed(
								VTE_TERMINAL(vte), 
								result + i,
								1
							);
					}
				}
				else
				{
					for(int i = 0; i < strlen(error); i++)
					{
						if(error[i] == '\n')
						{
							vte_terminal_feed(
								VTE_TERMINAL(vte), 
								"\r\n",
								2
							);
						}
						else
							vte_terminal_feed(
								VTE_TERMINAL(vte), 
								error + i,
								1
							);
					}
				}

				GtkWidget* inner_box = gtk_box_new(
					GTK_ORIENTATION_HORIZONTAL, 0
				);
				gtk_box_pack_start(
					GTK_BOX(inner_box), 
					vte,
					TRUE,
					TRUE,
					0
				);

				gtk_stack_add_titled(
					GTK_STACK(stack), 
					inner_box, name, name
				);
				g_string_free(script, TRUE);
			}
		}
	}
	g_string_free(string, TRUE);

	gtk_paned_add1(GTK_PANED(paned), sidebar);
	gtk_paned_add2(GTK_PANED(paned), stack);
	gtk_stack_sidebar_set_stack(
		GTK_STACK_SIDEBAR(sidebar), 
		GTK_STACK(stack)
	);

	return paned;
}

void on_application(
	GtkAppChooserWidget* widget, 
	GAppInfo* info, 
	gpointer user_data
)
{
	struct Popup* popup = user_data;
	GList* list = NULL;
	list = g_list_append(list, popup->file);
	g_app_info_launch(info, list, NULL, NULL);
	gtk_window_close(GTK_WINDOW(popup->dialog));
}

void on_response(
	GtkDialog* dialog, 
	gint response_id, 
	gpointer user_data
)
{
	struct Popup* popup = user_data;
	GtkAppChooser* chooser = GTK_APP_CHOOSER(dialog);
	if(response_id == GTK_RESPONSE_OK)
	{
		on_application(NULL, gtk_app_chooser_get_app_info(chooser), popup);
	}
	else
	{
		gtk_window_close(GTK_WINDOW(popup->dialog));
	}
}

void on_edit(GtkButton* button, gpointer user_data)
{
	struct Project_Tester* program = user_data;
	GString* file_name = g_string_new(
		gtk_stack_get_visible_child_name(program->content)
	);
	if(!strcmp(file_name->str, "counter"))
	{
		g_string_append(file_name, "/lib/");
		g_string_append(file_name, "count");
	}
	else
	{
		g_string_append(file_name, "/lib/");
		g_string_append(
			file_name, 
			gtk_stack_get_visible_child_name(program->content)
		);
	}

	g_string_append(file_name, ".rb");
	GFile* file = g_file_new_for_path(file_name->str);

	GtkWidget* dialog = gtk_app_chooser_dialog_new(
		program->window, 
		GTK_DIALOG_USE_HEADER_BAR,
		file
	);
	struct Popup* popup = g_malloc(sizeof(struct Popup));
	popup->file = file;
	popup->dialog = GTK_APP_CHOOSER_DIALOG(dialog);

	g_signal_connect(
		dialog,
		"response",
		G_CALLBACK(on_response),
		popup
	);
	gtk_widget_show(dialog);
}

void on_list_click(
	GtkListBox* box, 
	GtkListBoxRow* row, 
	gpointer user_data
)
{
	struct Project_Tester* program = user_data;
	if(!gtk_stack_get_child_by_name(
			program->content,
			gtk_label_get_text(
				GTK_LABEL(gtk_bin_get_child(GTK_BIN(row)))
			)
		)
	)
	{
		GtkWidget* widget = create_project(
			gtk_label_get_text(
				GTK_LABEL(gtk_bin_get_child(GTK_BIN(row)))
			)
		);
		gtk_widget_show_all(widget);
		gtk_stack_add_named(
			program->content,
			widget,
			gtk_label_get_text(
				GTK_LABEL(gtk_bin_get_child(GTK_BIN(row)))
			)
		);
	}

	gtk_stack_set_visible_child_name(
		program->content,
		gtk_label_get_text(
			GTK_LABEL(gtk_bin_get_child(GTK_BIN(row)))
		)
	);
	
	gtk_widget_show_all(GTK_WIDGET(program->window));
	gtk_widget_hide(program->list_refresh_button);
}

void on_back(GtkButton* button, gpointer user_data)
{
	struct Project_Tester* program = user_data;
	gtk_stack_set_transition_type(
		program->content,
		GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT
	);
	gtk_stack_set_visible_child_name(program->content, "list");
	gtk_stack_set_transition_type(
		program->content,
		GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT
	);

	gtk_widget_show_all(GTK_WIDGET(program->window));
	gtk_widget_hide(program->project_refresh_button);
	gtk_widget_hide(program->back_button);
	gtk_widget_hide(program->edit_button);
}

void on_list_refresh(GtkButton* button, gpointer user_data)
{
	struct Project_Tester* program = user_data;
	GList* list_items = gtk_container_get_children(
		GTK_CONTAINER(program->list)
	);

	for(GList* l = list_items; l != NULL; l = l->next)
	{
		gtk_container_remove(GTK_CONTAINER(program->list), l->data);
	}
	
	GDir* dir = g_dir_open(".", 0, NULL);
	for(const char* name = NULL; (name = g_dir_read_name(dir));)
	{
		if(g_file_test(name, G_FILE_TEST_IS_DIR))
		{
			GtkWidget* label = gtk_label_new(name);
			gtk_container_add(GTK_CONTAINER(program->list), label);
		}
	}
	g_dir_close(dir);
	gtk_widget_show_all(GTK_WIDGET(program->list));
}
	

void on_project_refresh(GtkButton* button, gpointer user_data)
{
	struct Project_Tester* program = user_data;
	GtkStack* stack = GTK_STACK(
		gtk_paned_get_child2(
			GTK_PANED(
				gtk_stack_get_visible_child(
					program->content
				)
			)
		)
	);
	GtkWidget* box = gtk_stack_get_visible_child(stack);
	GList* box_content = gtk_container_get_children(GTK_CONTAINER(box));
	gtk_container_remove(
		GTK_CONTAINER(box), 
		GTK_WIDGET(box_content->data)
	);

	GString* string = g_string_new(
		gtk_stack_get_visible_child_name(program->content)
	);
	g_string_append(string, "/test");
	GString* script = g_string_new("ruby ");
	g_string_append(script, string->str);
	g_string_append(script, "/");
	g_string_append(
		script, 
		gtk_stack_get_visible_child_name(stack)
	);

	char* result;
	char* error;
	g_spawn_command_line_sync(
		script->str,
		&result,
		&error,
		NULL,
		NULL
	);

	g_string_free(string, TRUE);
	g_string_free(script, TRUE);

	GtkWidget* vte = vte_terminal_new();
	vte_terminal_set_rewrap_on_resize(
		VTE_TERMINAL(vte),
		TRUE
	);
	if(strlen(result))
	{
		for(int i = 0; i < strlen(result); i++)
		{
			if(result[i] == '\n')
			{
				vte_terminal_feed(
					VTE_TERMINAL(vte), 
					"\r\n",
					2
				);
			}
			else
				vte_terminal_feed(
					VTE_TERMINAL(vte), 
					result + i,
					1
				);
		}
	}
	else
	{
		for(int i = 0; i < strlen(error); i++)
		{
			if(error[i] == '\n')
			{
				vte_terminal_feed(
					VTE_TERMINAL(vte), 
					"\r\n",
					2
				);
			}
			else
				vte_terminal_feed(
					VTE_TERMINAL(vte), 
					error + i,
					1
				);
		}
	}

	gtk_box_pack_start(
		GTK_BOX(box), 
		vte,
		TRUE,
		TRUE,
		0
	);

	gtk_widget_show_all(GTK_WIDGET(box));
}

void activate(GtkApplication* app, gpointer user_data)
{
	struct Project_Tester* program = g_malloc(sizeof(struct Project_Tester));
	program->window = GTK_WINDOW(gtk_application_window_new(app));
	gtk_window_set_default_size(program->window, 850, 525);
	gtk_container_set_border_width(GTK_CONTAINER(program->window), 5);

	program->header = GTK_HEADER_BAR(gtk_header_bar_new());
	gtk_header_bar_set_title(program->header, "Project Tester");
	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_header_bar_set_has_subtitle(program->header, TRUE);
	gtk_header_bar_set_show_close_button(program->header, TRUE);
	gtk_window_set_titlebar(program->window, GTK_WIDGET(program->header));
	
	program->list_refresh_button = gtk_button_new_from_icon_name(
		"gtk-refresh",
		GTK_ICON_SIZE_MENU
	);
	g_signal_connect(
		program->list_refresh_button,
		"clicked",
		G_CALLBACK(on_list_refresh),
		program
	);
	gtk_header_bar_pack_end(
		program->header, 
		program->list_refresh_button
	);
	
	program->content = GTK_STACK(gtk_stack_new());
	gtk_stack_set_transition_duration(program->content, 500);
	gtk_stack_set_transition_type(
		program->content, 
		GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT
	);

	program->list = GTK_LIST_BOX(gtk_list_box_new());
	g_signal_connect(
		program->list,
		"row-activated",
		G_CALLBACK(on_list_click),
		program
	);

	GDir* dir = g_dir_open(".", 0, NULL);
	for(const char* name = NULL; (name = g_dir_read_name(dir));)
	{
		if(g_file_test(name, G_FILE_TEST_IS_DIR))
		{
			GString* string = g_string_new(name);
			g_string_append(string, "/test");
			if(g_file_test(string->str, G_FILE_TEST_IS_DIR))
			{
				GtkWidget* label = gtk_label_new(name);
				gtk_container_add(GTK_CONTAINER(program->list), label);
			}
			g_string_free(string, TRUE);
		}
	}
	g_dir_close(dir);

	GtkWidget* visible;
	if(g_list_length(gtk_container_get_children(GTK_CONTAINER(program->list))))
	{
		gtk_widget_show(GTK_WIDGET(program->list));
		gtk_stack_add_named(
			program->content, 
			GTK_WIDGET(program->list),
			"list"
		);
		visible = GTK_WIDGET(program->list);
	}
	else
	{
		visible = gtk_label_new("No projects were found");
		gtk_stack_add_named(
			program->content,
			visible,
			"list"
		);
	}
	gtk_stack_set_visible_child(program->content, visible);
	gtk_container_add(
		GTK_CONTAINER(program->window), 
		GTK_WIDGET(program->content)
	);
	gtk_widget_show_all(GTK_WIDGET(program->window));
	
	program->back_button = gtk_button_new_with_label("Back");
	/*program->back_button = gtk_button_new_from_icon_name(
		"gtk-go-back",
		GTK_ICON_SIZE_MENU
	);*/
	g_signal_connect(
		program->back_button,
		"clicked",
		G_CALLBACK(on_back),
		program
	);
	gtk_header_bar_pack_start(program->header, program->back_button);

	program->edit_button = gtk_button_new_with_label("Edit");
	g_signal_connect(
		program->edit_button,
		"clicked",
		G_CALLBACK(on_edit),
		program
	);
	gtk_header_bar_pack_end(
		program->header,
		program->edit_button
	);

	program->project_refresh_button = gtk_button_new_from_icon_name(
		"gtk-refresh",
		GTK_ICON_SIZE_MENU
	);
	g_signal_connect(
		program->project_refresh_button,
		"clicked",
		G_CALLBACK(on_project_refresh),
		program
	);
	gtk_header_bar_pack_end(
		program->header,
		program->project_refresh_button
	);
}

int main(int argc, char* argv[])
{
	GtkApplication* app = gtk_application_new(
		"com.github.itggot-erik-wallstrom.Project_Tester",
		G_APPLICATION_FLAGS_NONE
	);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}
