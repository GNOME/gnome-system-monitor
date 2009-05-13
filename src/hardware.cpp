/* Gnome System Monitor - hardware.cpp
 * Copyright (C) 2008 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include <glib/gi18n.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkscrolledwindow.h>

#include <sigc++/bind.h>
#include <gtkmm/treeiter.h>

#include <hd.h>

#include "hardware.h"
#include "interface.h"

namespace {

enum HwColumns
{
	HW_DEVICE,
	HW_TYPE,
	HW_ICON,
	HW_N_COLUMNS
};


struct hw_map_data_t {
	hd_hw_item_t key;
	const char * icon;
	const char * type_label;
};

typedef std::map<hd_hw_item_t, const hw_map_data_t*> hw_map_t;

const hw_map_t & get_map_data()
{
	static hw_map_t s_map;
	static const hw_map_data_t _s_map_data[] = {
		{ hw_sys, "system", "" },
		{ hw_cpu, "", _("CPU") },
		{ hw_keyboard, "keyboard", _("Keyboard") },
		{ hw_braille, "braille", _("Braille device") },
		{ hw_mouse, "mouse", _("Mouse") },
		{ hw_joystick, "joystick", _("Joystick") },
		{ hw_printer, "printer", _("Printer") },
		{ hw_scanner, "scanner", _("Scanner") },
		{ hw_chipcard, "", "chipcard??" },
		{ hw_monitor, "display", _("Monitor") },
		{ hw_tv, "", _("TV") },
		{ hw_framebuffer, "", _("Framebuffer") },
		{ hw_camera, "camera", _("Camera") },
		{ hw_sound, "audio-card", _("Sound card") },
		{ hw_storage_ctrl, "", _("Storage controller") },
		{ hw_network_ctrl, "", _("Network controller") },
		{ hw_isdn, "", _("ISDN adapter") },
		{ hw_modem, "modem", _("Modem") },
		{ hw_network, "network-wired", _("Network interface") },
		{ hw_disk, "drive-harddisk", _("Hard drive") },
		{ hw_partition, "", _("Partition") },
		{ hw_cdrom, "drive-cdrom", _("CDROM Drive") },
		{ hw_floppy, "media-floppy", _("Floppy Drive") },
		{ hw_manual, "", "manual???" },
		{ hw_usb_ctrl, "", _("USB Controller") },
		{ hw_usb, "", _("USB Device") },
		{ hw_bios, "", _("BIOS") },
		{ hw_pci, "", _("PCI Device") },
		{ hw_isapnp, "", _("ISA PnP Device") },
		{ hw_bridge, "", _("Bridge") },
		{ hw_hub, "", _("Hub") },
		{ hw_scsi, "", _("SCSI Device") },
		{ hw_ide, "", _("IDE Device") },
		{ hw_memory, "", _("Memory") },
		{ hw_dvb, "", _("DV-B Receiver") },
		{ hw_pcmcia, "", _("PCMCIA Card") },
		{ hw_pcmcia_ctrl, "", _("PCMCIA Controller") },
		{ hw_ieee1394, "", _("Firewire Device") },
		{ hw_ieee1394_ctrl, "", _("Firewire Controller") },
		{ hw_hotplug, "", _("Hotplug Device") },
		{ hw_hotplug_ctrl, "", _("Hotplug Controller") },
		{ hw_zip, "media-zip", _("Zip Drive") },
		{ hw_pppoe, "", _("PPPoE Interface") },
		{ hw_wlan, "", _("WLAN Interface") },
		{ hw_redasd, "", "redasd??" },
		{ hw_dsl, "", _("DSL Modem") },
		{ hw_block, "", _("Block Device") },
		{ hw_tape, "media-tape", _("Tape Drive") },
		{ hw_vbe, "", _("vbe???") },
		{ hw_bluetooth, "", _("Bluetooth Device") },
		{ hw_fingerprint, "", _("Fingerprint Reader") },

		{ hw_none, NULL, NULL }
	};
	if (s_map.empty()) {
		const struct hw_map_data_t *p = _s_map_data;
		while ((p->key != hw_none) &&  (p->key != hw_bridge)) {
			s_map.insert(std::make_pair(p->key, p));
			p++;
		}
	}
	return s_map;
}

const char * get_hardware_type(hd_hw_item_t hwclass)
{
	const hw_map_t & hw_map(get_map_data());

	hw_map_t::const_iterator iter = hw_map.find(hwclass);
	if(iter != hw_map.end() && iter->second) {
		return iter->second->type_label;
	}
	return NULL;
}


Glib::RefPtr<Gdk::Pixbuf> get_hardware_icon(hd_hw_item_t hwclass)
{
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;

	Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();

	const hw_map_t & hw_map(get_map_data());

	hw_map_t::const_iterator iter = hw_map.find(hwclass);
	if(iter != hw_map.end() && iter->second && iter->second->icon
		&& *iter->second->icon) {
		pixbuf = icon_theme->load_icon(iter->second->icon, 16,
									   Gtk::ICON_LOOKUP_USE_BUILTIN);
	}

	return pixbuf;
}


void populate_hardware_view(GtkTreeStore * model)
{
	hd_data_t hd_data = { 0 };
	hd_t *hd;

	g_object_freeze_notify(G_OBJECT(model));

	std::map<std::string, Gtk::TreeIter> item_map;
	std::list<Gtk::TreeIter> unparented;

	hd = hd_list(&hd_data, hw_all, 1, NULL);
	for(; hd; hd = hd->next) {
		const char * name = hd->model;
		hd_hw_item_t iclass = hd->hw_class;
		// FIXME: for now skip network interfaces.
		// why ???
		if(iclass == hw_network)
			continue;
		GtkTreeIter *parent = NULL;
		if(hd->parent_id) {
			std::map<std::string, Gtk::TreeIter>::iterator iter;
			iter = item_map.find(hd->parent_id);
			if(iter != item_map.end()) {
				parent = iter->second.gobj();
			}
		}

		Glib::RefPtr<Gdk::Pixbuf> pixbuf = get_hardware_icon(iclass);
		const char * device_type = get_hardware_type(iclass);
		if(name && *name) {
			Gtk::TreeIter iter;
			gtk_tree_store_append(model, iter.gobj(), parent);
			gtk_tree_store_set(model, iter.gobj(), HW_DEVICE, name,
							   HW_ICON, pixbuf ? pixbuf->gobj() : NULL,
							   HW_TYPE, device_type ? device_type : "",
							   -1);
			item_map[hd->unique_id] = iter;
			if(hd->parent_id && !parent) {
				// has a parent id but no parent found
				unparented.push_front(iter);
			}
		}
	}

	while(!unparented.empty()) {
		Gtk::TreeIter treeiter(unparented.front());

		printf("TODO reparent\n");
		// reparent
		unparented.pop_front();
	}


	hd_free_hd_list(hd);          /* free it */
	hd_free_hd_data(&hd_data);
	free(hd);
	g_object_thaw_notify(G_OBJECT(model));
	g_object_unref(model);
}

} // anonymous namespace


namespace procman {

GtkWidget* create_hardware_view()
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	GtkWidget *hw_tree;
	GtkTreeStore *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;

	const gchar * const titles[] = {
		N_("Device"),
		N_("Type")
	};

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	label = make_title_label(_("Hardware"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
					    GTK_SHADOW_IN);

	gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

	model = gtk_tree_store_new(HW_N_COLUMNS,	/* n columns */
							   G_TYPE_STRING,	/* HW_DEVICE */
							   G_TYPE_STRING,   /* HW_TYPE */
							   GDK_TYPE_PIXBUF	/* HW_ICON */
		);

	hw_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_container_add(GTK_CONTAINER(scrolled), hw_tree);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(hw_tree), TRUE);
	g_object_unref(G_OBJECT(model));

	/* icon + device */

	col = gtk_tree_view_column_new();
	cell = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, cell, FALSE);
	gtk_tree_view_column_set_attributes(col, cell, "pixbuf", HW_ICON,
					    NULL);
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, cell, FALSE);
	gtk_tree_view_column_set_attributes(col, cell, "text", HW_DEVICE,
					    NULL);
	gtk_tree_view_column_set_title(col, titles[HW_DEVICE]);
	gtk_tree_view_column_set_sort_column_id(col, HW_DEVICE);
	gtk_tree_view_column_set_reorderable(col, TRUE);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(hw_tree), col);

	/* type */
	col = gtk_tree_view_column_new();
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, cell, FALSE);
	gtk_tree_view_column_set_attributes(col, cell, "text", HW_TYPE,
										NULL);
	gtk_tree_view_column_set_title(col, titles[HW_TYPE]);
	gtk_tree_view_column_set_sort_column_id(col, HW_TYPE);
	gtk_tree_view_column_set_reorderable(col, TRUE);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(hw_tree), col);

	/* 
	if(!Glib::thread_supported())
		Glib::thread_init();
	Glib::Thread::create(sigc::bind(&populate_hardware_view, GTK_TREE_STORE(g_object_ref(model))), false);
	*/

	populate_hardware_view(GTK_TREE_STORE(g_object_ref(model)));

	gtk_widget_show_all(vbox);

	return vbox;
}

} // namespace procman
