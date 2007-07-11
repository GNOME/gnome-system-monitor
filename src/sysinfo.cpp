#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <glibtop/fsusage.h>
#include <glibtop/mountlist.h>
#include <glibtop/mem.h>
#include <glibtop/sysinfo.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>

#include "regex.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/utsname.h>

#include "sysinfo.h"
#include "util.h"


using std::string;
using std::vector;


namespace {


  class SysInfo
  {
  public:
    string hostname;
    string distro_name;
    string distro_release;
    string kernel;
    string gnome_version;
    guint64 memory_bytes;
    guint64 free_space_bytes;

    guint n_processors;
    vector<string> processors;


    SysInfo()
    {
      this->load_processors_info();
      this->load_memory_info();
      this->load_disk_info();
      this->load_uname_info();
      this->load_gnome_version();
    }

    virtual ~SysInfo()
    { }

  private:

    void load_memory_info()
    {
      glibtop_mem mem;

      glibtop_get_mem(&mem);
      this->memory_bytes = mem.total;
    }


    void load_processors_info()
    {
      const glibtop_sysinfo *info = glibtop_get_sysinfo();

      for (guint i = 0; i != info->ncpu; ++i) {
	const char * const keys[] = { "model name", "cpu" };
	gchar *model = 0;

	for (guint j = 0; !model && j != G_N_ELEMENTS(keys); ++j)
	  model = static_cast<char*>(g_hash_table_lookup(info->cpuinfo[i].values,
							 keys[j]));

	if (!model)
	  model = _("Unknown CPU model");

	this->processors.push_back(model);
      }
    }



    void load_disk_info()
    {
      glibtop_mountentry *entries;
      glibtop_mountlist mountlist;

      entries = glibtop_get_mountlist(&mountlist, 0);

      this->free_space_bytes = 0;

      for (guint i = 0; i != mountlist.number; ++i) {

	if (string(entries[i].devname).find("/dev/") != 0)
	  continue;

	if (string(entries[i].mountdir).find("/media/") == 0)
	  continue;

	glibtop_fsusage usage;
	glibtop_get_fsusage(&usage, entries[i].mountdir);
	this->free_space_bytes += usage.bavail * usage.block_size;
      }

      g_free(entries);
    }


    void load_uname_info()
    {
      struct utsname name;

      uname(&name);

      this->hostname = name.nodename;
      this->kernel = string(name.sysname) + ' ' + name.release;
    }


    void load_gnome_version()
    {
      xmlDocPtr document;
      xmlXPathContextPtr context;
      const string nodes[3] = { "string(/gnome-version/platform)",
				"string(/gnome-version/minor)",
				"string(/gnome-version/micro)" };
      string values[3];

      if (not (document = xmlParseFile(DATADIR "/gnome-about/gnome-version.xml")))
	return;

      if (not (context = xmlXPathNewContext(document)))
	return;

      for (size_t i = 0; i != 3; ++i)
	{
	  xmlXPathObjectPtr xpath;
	  xpath = xmlXPathEvalExpression(BAD_CAST nodes[i].c_str(), context);
	  
	  if (xpath and xpath->type == XPATH_STRING)
	    values[i] = reinterpret_cast<const char*>(xpath->stringval);

	  xmlXPathFreeObject(xpath);
	}

      xmlXPathFreeContext(context);
      xmlFreeDoc(document);

      this->gnome_version = values[0] + '.' + values[1] + '.' + values[2];
    }
  };



  class SolarisSysInfo
    : public SysInfo
  {
  public:
    SolarisSysInfo()
    {
      this->load_solaris_info();
    }

  private:
    void load_solaris_info()
    {
      this->distro_name = "Solaris";

      std::ifstream input("/etc/release");

      if (input)
	std::getline(input, this->distro_release);
    }
  };


  class LSBSysInfo
    : public SysInfo
  {
  public:
    LSBSysInfo()
      : re("^.+?:\\s*(.+)\\s*$")
    {
      this->lsb_release();
    }

  private:

    void strip_description(string &s) const
    {
      // make a copy to avoid aliasing
      this->re.PartialMatch(string(s), &s);
    }

    std::istream& get_value(std::istream &is, string &s) const
    {
      if (std::getline(is, s))
	this->strip_description(s);
      return is;
    }

    void lsb_release()
    {
      char *out= 0;
      GError *error = 0;
      int status;

      if (g_spawn_command_line_sync("lsb_release -irc",
				    &out,
				    0,
				    &status,
				    &error)) {
	string release, codename;
	if (!error and WIFEXITED(status) and WEXITSTATUS(status) == 0) {
	  std::istringstream input(out);
	  this->get_value(input, this->distro_name)
	    and this->get_value(input, release)
	    and this->get_value(input, codename);
	  this->distro_release = release;
	  if (codename != "")
	    this->distro_release += " (" + codename + ')';
	}
      }

      if (error)
	g_error_free(error);

      g_free(out);
    }

  private:
    const pcrecpp::RE re;
  };


  SysInfo* get_sysinfo()
  {
    if (char *p = g_find_program_in_path("lsb_release")) {
      g_free(p);
      return new LSBSysInfo;
    }
    else if (g_file_test("/etc/release", G_FILE_TEST_EXISTS))
      return new SolarisSysInfo;

    return new SysInfo;
  }
}


#define X_PAD  5
#define Y_PAD  12
#define LOGO_W 92
#define LOGO_H 351
#define RADIUS 5

static gboolean
sysinfo_logo_expose (GtkWidget *widget,
		     GdkEventExpose *event,
		     gpointer data_ptr)
{
  cairo_t *cr;
  cairo_pattern_t *cp;

  cr = gdk_cairo_create(widget->window);

  cairo_translate(cr, event->area.x, event->area.y);

  cairo_move_to(cr, X_PAD + RADIUS, Y_PAD);
  cairo_line_to(cr, X_PAD + LOGO_W - RADIUS, Y_PAD);
  cairo_arc(cr, X_PAD + LOGO_W - RADIUS, Y_PAD + RADIUS, RADIUS, -0.5 * M_PI, 0);
  cairo_line_to(cr, X_PAD + LOGO_W, Y_PAD + LOGO_H - RADIUS);
  cairo_arc(cr, X_PAD + LOGO_W - RADIUS, Y_PAD + LOGO_H - RADIUS, RADIUS, 0, 0.5 * M_PI);
  cairo_line_to(cr, X_PAD + RADIUS, Y_PAD + LOGO_H);
  cairo_arc(cr, X_PAD + RADIUS, Y_PAD + LOGO_H - RADIUS, RADIUS, 0.5 * M_PI, -1.0 * M_PI);
  cairo_line_to(cr, X_PAD, Y_PAD + RADIUS);
  cairo_arc(cr,  X_PAD + RADIUS, Y_PAD + RADIUS, RADIUS, -1.0 * M_PI, -0.5 * M_PI);

  cp = cairo_pattern_create_linear(0, Y_PAD, 0, Y_PAD + LOGO_H);
  cairo_pattern_add_color_stop_rgba(cp, 0.0,
				    widget->style->base[GTK_STATE_SELECTED].red / 65535.0,
				    widget->style->base[GTK_STATE_SELECTED].green / 65535.0,
				    widget->style->base[GTK_STATE_SELECTED].blue / 65535.0,
				    1.0);
  cairo_pattern_add_color_stop_rgba(cp, 1.0,
				    widget->style->base[GTK_STATE_SELECTED].red / 65535.0,
				    widget->style->base[GTK_STATE_SELECTED].green / 65535.0,
				    widget->style->base[GTK_STATE_SELECTED].blue / 65535.0,
				    0.0);
  cairo_set_source(cr, cp);
  cairo_fill(cr);

  cairo_pattern_destroy(cp);
  cairo_destroy(cr);

  return FALSE;
}

GtkWidget *
procman_create_sysinfo_view(void)
{
  GtkWidget *hbox;
  GtkWidget *vbox;

  SysInfo *data = get_sysinfo();;

  GtkWidget * logo;

  GtkWidget *distro_frame;
  GtkWidget *distro_release_label;

  GtkWidget *hardware_frame;
  GtkWidget *hardware_table;
  GtkWidget *memory_label;
  GtkWidget *processor_label;

  GtkWidget *disk_space_frame;
  GtkWidget *disk_space_table;
  GtkWidget *disk_space_label;

  GtkWidget *header;
  GtkWidget *alignment;

  gchar *markup;


  hbox = gtk_hbox_new(FALSE, 12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

  /* left-side logo */

  logo = gtk_image_new_from_file(DATADIR "/pixmaps/gnome-system-monitor/side.png");
  gtk_misc_set_alignment(GTK_MISC(logo), 0.5, 0.0);
  gtk_misc_set_padding(GTK_MISC(logo), 5, 12);
  gtk_box_pack_start(GTK_BOX(hbox), logo, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(logo), "expose-event",
		   G_CALLBACK(sysinfo_logo_expose), NULL);

  vbox = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

  // hostname

  markup = g_strdup_printf("<big><big><b><u>%s</u></b></big></big>",
    data->hostname.c_str());
  GtkWidget *hostname_frame = gtk_frame_new(markup);
  g_free(markup);
  gtk_frame_set_label_align(GTK_FRAME(hostname_frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(hostname_frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(hostname_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), hostname_frame, FALSE, FALSE, 0);


  /* distro section */

  markup = g_strdup_printf("<big><big><b>%s</b></big></big>",
    data->distro_name.c_str());
  distro_frame = gtk_frame_new(markup);
  g_free(markup);
  gtk_frame_set_label_align(GTK_FRAME(distro_frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(distro_frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(distro_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), distro_frame, FALSE, FALSE, 0);
  alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(distro_frame), alignment);

  GtkWidget* distro_box = gtk_hbox_new(FALSE, 12);
  gtk_container_add(GTK_CONTAINER(alignment), distro_box);

  GtkWidget* distro_inner_box = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(distro_box), distro_inner_box, FALSE, FALSE, 0);

  if (data->distro_release != "")
    {
      markup = g_strdup_printf(_("Release %s"), data->distro_release.c_str());
      distro_release_label = gtk_label_new(markup);
      gtk_misc_set_alignment(GTK_MISC(distro_release_label), 0.0, 0.5);
      g_free(markup);
      gtk_box_pack_start(GTK_BOX(distro_inner_box), distro_release_label, FALSE, FALSE, 0);
    }

  markup = g_strdup_printf(_("Kernel %s"), data->kernel.c_str());
  GtkWidget* kernel_label = gtk_label_new(markup);
  gtk_misc_set_alignment(GTK_MISC(kernel_label), 0.0, 0.5);
  g_free(markup);
  gtk_box_pack_start(GTK_BOX(distro_inner_box), kernel_label, FALSE, FALSE, 0);

  if (data->gnome_version != "")
    {
      markup = g_strdup_printf(_("GNOME %s"), data->gnome_version.c_str());
      GtkWidget* gnome_label = gtk_label_new(markup);
      gtk_misc_set_alignment(GTK_MISC(gnome_label), 0.0, 0.5);
      g_free(markup);
      gtk_box_pack_start(GTK_BOX(distro_inner_box), gnome_label, FALSE, FALSE, 0);
    }

  /* hardware section */

  markup = g_strdup_printf(_("<b>Hardware</b>"));
  hardware_frame = gtk_frame_new(markup);
  g_free(markup);
  gtk_frame_set_label_align(GTK_FRAME(hardware_frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(hardware_frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(hardware_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), hardware_frame, FALSE, FALSE, 0);

  alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(hardware_frame), alignment);

  hardware_table = gtk_table_new(data->processors.size(), 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(hardware_table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(hardware_table), 6);
  gtk_container_set_border_width(GTK_CONTAINER(hardware_table), 6);
  gtk_container_add(GTK_CONTAINER(alignment), hardware_table);

  header = gtk_label_new(_("Memory:"));
  gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(hardware_table), header,
		   0, 1, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );

  markup = SI_gnome_vfs_format_file_size_for_display(data->memory_bytes);
  memory_label = gtk_label_new(markup);
  g_free(markup);
  gtk_misc_set_alignment(GTK_MISC(memory_label), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(hardware_table), memory_label,
		   1, 2, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );

  for (guint i = 0; i < data->processors.size(); ++i) {
    if (data->processors.size() > 1) {
      markup = g_strdup_printf(_("Processor %d:"), i);
      header = gtk_label_new(markup);
      g_free(markup);
    }
    else
      header = gtk_label_new(_("Processor:"));

    gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
    gtk_table_attach(
		     GTK_TABLE(hardware_table), header,
		     0, 1, 1 + i, 2 + i,
		     GTK_FILL, GTK_FILL, 0, 0
		     );

    processor_label = gtk_label_new(data->processors[i].c_str());
    gtk_misc_set_alignment(GTK_MISC(processor_label), 0.0, 0.5);
    gtk_table_attach(
		     GTK_TABLE(hardware_table), processor_label,
		     1, 2, 1 + i, 2 + i,
		     GTK_FILL, GTK_FILL, 0, 0
		     );
  }

  /* disk space section */

  markup = g_strdup_printf(_("<b>System Status</b>"));
  disk_space_frame = gtk_frame_new(markup);
  g_free(markup);
  gtk_frame_set_label_align(GTK_FRAME(disk_space_frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(disk_space_frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(disk_space_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), disk_space_frame, FALSE, FALSE, 0);

  alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(disk_space_frame), alignment);

  disk_space_table = gtk_table_new(1, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(disk_space_table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(disk_space_table), 6);
  gtk_container_set_border_width(GTK_CONTAINER(disk_space_table), 6);
  gtk_container_add(GTK_CONTAINER(alignment), disk_space_table);

  header = gtk_label_new(_("Available disk space:"));
  gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(disk_space_table), header,
		   0, 1, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );

  markup = SI_gnome_vfs_format_file_size_for_display(data->free_space_bytes);
  disk_space_label = gtk_label_new(markup);
  g_free(markup);
  gtk_misc_set_alignment(GTK_MISC(disk_space_label), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(disk_space_table), disk_space_label,
		   1, 2, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );

  return hbox;
}

