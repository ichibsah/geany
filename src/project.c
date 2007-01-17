/*
 *      project.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007 Enrico Tröger <enrico.troeger@uvena.de>
 *      Copyright 2007 Nick Treleaven <nick.treleaven@btinternet.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 */

#include "geany.h"

#include <string.h>

#include "project.h"
#include "dialogs.h"
#include "support.h"
#include "utils.h"
#include "ui_utils.h"
#ifdef G_OS_WIN32
# include "win32.h"
#endif


// simple struct to keep references to the elements of the properties dialog
typedef struct
{
	GtkWidget *dialog;
	GtkWidget *name;
	GtkWidget *description;
	GtkWidget *file_name;
	GtkWidget *base_path;
	GtkWidget *patterns;
} PropertyDialogElements;



static void on_properties_dialog_response(GtkDialog *dialog, gint response,
										  PropertyDialogElements *e);
static void on_file_open_button_clicked(GtkButton *button, GtkWidget *entry);
static void on_folder_open_button_clicked(GtkButton *button, GtkWidget *entry);
static gboolean close_open_project();
static void on_name_entry_changed(GtkEditable *editable, PropertyDialogElements *e);


void project_new()
{
	if (! close_open_project()) return;

	project_properties();
}


void project_open()
{
	if (! close_open_project()) return;


}


void project_close()
{
	/// TODO should we handle open files in any way here?

	g_return_if_fail(app->project != NULL);

	g_free(app->project->name);
	g_free(app->project->description);
	g_free(app->project->file_name);
	g_free(app->project->base_path);
	g_free(app->project->executable);

	g_free(app->project);
	app->project = NULL;
}


void project_properties()
{
	gchar *ok_button;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *image;
	GtkWidget *button;
	GtkWidget *bbox;
	GtkWidget *label;
	GtkWidget *swin;
	PropertyDialogElements *e = g_new(PropertyDialogElements, 1);

	if (app->project == NULL)
		ok_button = GTK_STOCK_NEW;
	else
		ok_button = GTK_STOCK_OK;

	e->dialog = gtk_dialog_new_with_buttons(_("Project properties"), GTK_WINDOW(app->window),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										 ok_button, GTK_RESPONSE_OK, NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(e->dialog));


	table = gtk_table_new(5, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	label = gtk_label_new(_("Name:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->name = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), e->name, 1, 2, 0, 1,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(_("Description:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->description = gtk_text_view_new();
	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), GTK_WIDGET(e->description));
	gtk_table_attach(GTK_TABLE(table), swin, 1, 2, 1, 2,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(_("File location:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->file_name = gtk_entry_new();
	button = gtk_button_new();
	g_signal_connect((gpointer) button, "clicked",
				G_CALLBACK(on_file_open_button_clicked), e->file_name);
	image = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	bbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start_defaults(GTK_BOX(bbox), e->file_name);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), bbox, 1, 2, 2, 3,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(_("Base path:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->base_path = gtk_entry_new();
	button = gtk_button_new();
	g_signal_connect((gpointer) button, "clicked",
				G_CALLBACK(on_folder_open_button_clicked), e->base_path);
	image = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	bbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start_defaults(GTK_BOX(bbox), e->base_path);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), bbox, 1, 2, 3, 4,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(_("File patterns:"));
	// <small>Separate multiple patterns by a new line</small>
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->patterns = gtk_text_view_new();
	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), GTK_WIDGET(e->patterns));
	gtk_table_attach(GTK_TABLE(table), swin, 1, 2, 4, 5,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);



	gtk_container_add(GTK_CONTAINER(vbox), table);

	// signals
	if (app->project == NULL)
	{	// this should only be done when we are about to create a new project
		g_signal_connect((gpointer) e->name, "changed", G_CALLBACK(on_name_entry_changed), e);
		// run the callback manually to initialise the base_path and file_name fields
		on_name_entry_changed(GTK_EDITABLE(e->name), e);
	}
	g_signal_connect((gpointer) e->dialog, "response",
				G_CALLBACK(on_properties_dialog_response), e);

	// if we have an already open project, fill the elements with the appropriate data
	if (app->project != NULL)
	{
		GeanyProject *p = app->project;

		gtk_entry_set_text(GTK_ENTRY(e->name), p->name);

		if (p->description != NULL)
		{	// set text
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(e->description));
			gtk_text_buffer_set_text(buffer, p->description, -1);
		}

		if (p->file_patterns != NULL)
		{	// set the file patterns
			gint i;
			gint len = g_strv_length(p->file_patterns);
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(e->patterns));
			GString *str = g_string_sized_new(len * 4);

			for (i = 0; i < len; i++)
			{
				if (p->file_patterns[i] != NULL)
				{
					g_string_append(str, p->file_patterns[i]);
					g_string_append_c(str, '\n');
				}
			}
			gtk_text_buffer_set_text(buffer, str->str, -1);
			g_string_free(str, FALSE); // can this leak?
		}

		gtk_entry_set_text(GTK_ENTRY(e->file_name), p->file_name);
		gtk_entry_set_text(GTK_ENTRY(e->base_path), p->base_path);
	}

	gtk_widget_show_all(e->dialog);
}


/* checks whether there is an already open project and asks the user if he wants to close it or
 * abort the current action. Returns FALSE when the current action(the caller) should be cancelled
 * and TRUE if we can go ahead */
static gboolean close_open_project()
{
	if (app->project != NULL)
	{
		gchar *msg =
			_("There is already an open project \"%s\". Do you want to close it before proceed?");

		if (dialogs_show_question(msg, app->project->name))
		{
			project_close();
			return TRUE;
		}
		else
			return FALSE;
	}
	else
		return TRUE;
}


#define SHOW_ERR(...) dialogs_show_msgbox(GTK_MESSAGE_ERROR, __VA_ARGS__)
#define MAX_LEN 50

static void on_properties_dialog_response(GtkDialog *dialog, gint response,
										  PropertyDialogElements *e)
{
	if (response == GTK_RESPONSE_OK && e != NULL)
	{
		const gchar *name, *file_name, *base_path;
		gint name_len;
		GeanyProject *p;

		name = gtk_entry_get_text(GTK_ENTRY(e->name));
		name_len = strlen(name);
		if (name_len == 0)
		{
			SHOW_ERR(_("The specified project name is too short."));
			gtk_widget_grab_focus(e->name);
			return;
		}
		else if (name_len > MAX_LEN)
		{
			SHOW_ERR(_("The specified project name is too long (max. %d characters)."), MAX_LEN);
			gtk_widget_grab_focus(e->name);
			return;
		}

		file_name = gtk_entry_get_text(GTK_ENTRY(e->file_name));
		if (strlen(file_name) == 0)
		{
			SHOW_ERR(_("You have specified an invalid project file location."));
			gtk_widget_grab_focus(e->file_name);
			return;
		}

		base_path = gtk_entry_get_text(GTK_ENTRY(e->base_path));
		if (strlen(base_path) == 0)
		{
			SHOW_ERR(_("You have specified an invalid project base path."));
			gtk_widget_grab_focus(e->base_path);
			return;
		}
		else
		{	// check whether the given directory actually exists
			gchar *locale_path = utils_get_locale_from_utf8(base_path);
			if (! g_file_test(locale_path, G_FILE_TEST_IS_DIR))
			{
				if (dialogs_show_question(
					_("The specified project base path does not exist. Should it be created?")))
				{
					utils_mkdir(locale_path);
				}
				else
				{
					g_free(locale_path);
					gtk_widget_grab_focus(e->base_path);
					return;
				}
			}
			g_free(locale_path);
		}

		// finally test whether the given project file can be written
		if (utils_write_file(file_name, "") != 0)
		{
			SHOW_ERR(_("Project file could not be written."));
			gtk_widget_grab_focus(e->file_name);
			return;
		}

		app->project = g_new0(GeanyProject, 1);
		p = app->project;

		p->name = g_strdup(name);
		{	// get and set the project description
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(e->description));
			GtkTextIter start, end;
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			p->description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		}
		p->file_name = g_strdup(file_name);
		p->base_path = g_strdup(base_path);

		{	// get and set the project file patterns
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(e->patterns));
			GtkTextIter start, end;
			gchar *tmp;
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			tmp = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
			g_strfreev(p->file_patterns);
			p->file_patterns = g_strsplit(tmp, "\n", -1);
			g_free(tmp);
		}
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(e);
}


static void on_file_open_button_clicked(GtkButton *button, GtkWidget *entry)
{
#ifdef G_OS_WIN32
	/// TODO write me
	//win32_show_project_file_dialog(item);
#else
	GtkWidget *dialog;

	// initialise the dialog
	dialog = gtk_file_chooser_dialog_new(_("Choose project filename"), NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	{	// set filename
		gchar *locale_filename = utils_get_locale_from_utf8(gtk_entry_get_text(GTK_ENTRY(entry)));

		if (g_path_is_absolute(locale_filename))
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), locale_filename);
		else
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), locale_filename);
		g_free(locale_filename);
	}

	// run it
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gchar *utf8_filename = utils_get_utf8_from_locale(filename);

		gtk_entry_set_text(GTK_ENTRY(entry), utf8_filename);

		g_free(utf8_filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
#endif
}


static void on_folder_open_button_clicked(GtkButton *button, GtkWidget *entry)
{
#ifdef G_OS_WIN32
	/// TODO write me
	//win32_show_project_folder_dialog(item);
#else
	GtkWidget *dialog;

	// initialise the dialog
	dialog = gtk_file_chooser_dialog_new(_("Choose project base path"), NULL,
					GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	{	// set filename
		gchar *locale_filename = utils_get_locale_from_utf8(gtk_entry_get_text(GTK_ENTRY(entry)));

		if (g_path_is_absolute(locale_filename))
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), locale_filename);
		else
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), locale_filename);
		g_free(locale_filename);
	}

	// run it
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gchar *utf8_filename = utils_get_utf8_from_locale(filename);

		gtk_entry_set_text(GTK_ENTRY(entry), utf8_filename);

		g_free(utf8_filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
#endif
}

// "projects" is part of the default project base path so be carefully when translating
// please avoid special characters and spaces, look at the source for details or ask Frank
#define PROJECT_DIR _("projects")

/* sets the project base path and the project file name according to the project name */
/// TODO cancel the process once base_path resp. file_name has been changed manually
static void on_name_entry_changed(GtkEditable *editable, PropertyDialogElements *e)
{
	gchar *base_path;
	gchar *file_name;
	gchar *name;

	name = gtk_editable_get_chars(editable, 0, -1);
	if (name != NULL && strlen(name) > 0)
	{
		base_path = g_strconcat(
			GEANY_HOME_DIR, G_DIR_SEPARATOR_S, PROJECT_DIR, G_DIR_SEPARATOR_S,
			name, G_DIR_SEPARATOR_S, NULL);
		file_name = g_strconcat(
			GEANY_HOME_DIR, G_DIR_SEPARATOR_S, PROJECT_DIR, G_DIR_SEPARATOR_S,
			name, G_DIR_SEPARATOR_S, name, ".geany", NULL);
		g_free(name);
	}
	else
	{
		base_path = g_strconcat(
			GEANY_HOME_DIR, G_DIR_SEPARATOR_S, PROJECT_DIR, G_DIR_SEPARATOR_S, NULL);
		file_name = g_strconcat(
			GEANY_HOME_DIR, G_DIR_SEPARATOR_S,PROJECT_DIR, G_DIR_SEPARATOR_S, NULL);
	}

	gtk_entry_set_text(GTK_ENTRY(e->base_path), base_path);
	gtk_entry_set_text(GTK_ENTRY(e->file_name), file_name);

	g_free(base_path);
	g_free(file_name);
}
