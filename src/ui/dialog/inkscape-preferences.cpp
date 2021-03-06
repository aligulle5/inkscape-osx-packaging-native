/** @file
 * @brief Inkscape Preferences dialog - implementation
 */
/* Authors:
 *   Carl Hetherington
 *   Marco Scholten
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Copyright (C) 2004-2007 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtkmm/main.h>
#include <gtkmm/frame.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/alignment.h>

#include "preferences.h"
#include "inkscape-preferences.h"
#include "verbs.h"
#include "selcue.h"
#include "unit-constants.h"
#include <iostream>
#include "enums.h"
// #include "inkscape.h"
#include "desktop-handles.h"
#include "message-stack.h"
#include "style.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "xml/repr.h"
#include "ui/widget/style-swatch.h"
#include "display/nr-filter-gaussian.h"
#include "display/nr-filter-types.h"
#include "color-profile-fns.h"
#include "color-profile.h"
#include "display/canvas-grid.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

InkscapePreferences::InkscapePreferences()
    : UI::Widget::Panel ("", "/dialogs/preferences", SP_VERB_DIALOG_DISPLAY),
      _max_dialog_width(0),
      _max_dialog_height(0),
      _current_page(0)
{
    //get the width of a spinbutton
    Gtk::SpinButton* sb = new Gtk::SpinButton;
    sb->set_width_chars(6);
    _getContents()->add(*sb);
    show_all_children();
    Gtk::Requisition sreq;
    sb->size_request(sreq);
    _sb_width = sreq.width;
    _getContents()->remove(*sb);
    delete sb;

    //Main HBox
    Gtk::HBox* hbox_list_page = Gtk::manage(new Gtk::HBox());
    hbox_list_page->set_border_width(12);
    hbox_list_page->set_spacing(12);
    _getContents()->add(*hbox_list_page);

    //Pagelist
    Gtk::Frame* list_frame = Gtk::manage(new Gtk::Frame());
    Gtk::ScrolledWindow* scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    hbox_list_page->pack_start(*list_frame, false, true, 0);
    _page_list.set_headers_visible(false);
    scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scrolled_window->add(_page_list);
    list_frame->set_shadow_type(Gtk::SHADOW_IN);
    list_frame->add(*scrolled_window);
    _page_list_model = Gtk::TreeStore::create(_page_list_columns);
    _page_list.set_model(_page_list_model);
    _page_list.append_column("name",_page_list_columns._col_name);
    Glib::RefPtr<Gtk::TreeSelection> page_list_selection = _page_list.get_selection();
    page_list_selection->signal_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::on_pagelist_selection_changed));
    page_list_selection->set_mode(Gtk::SELECTION_BROWSE);

    //Pages
    Gtk::VBox* vbox_page = Gtk::manage(new Gtk::VBox());
    Gtk::Frame* title_frame = Gtk::manage(new Gtk::Frame());

    Gtk::ScrolledWindow* pageScroller = Gtk::manage(new Gtk::ScrolledWindow());
    pageScroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    pageScroller->add(*vbox_page);
    hbox_list_page->pack_start(*pageScroller, true, true, 0);

    title_frame->add(_page_title);
    vbox_page->pack_start(*title_frame, false, false, 0);
    vbox_page->pack_start(_page_frame, true, true, 0);
    _page_frame.set_shadow_type(Gtk::SHADOW_IN);
    title_frame->set_shadow_type(Gtk::SHADOW_IN);

    initPageTools();
    initPageSelecting();
    initPageTransforms();
    initPageClones();
    initPageMasks();
    initPageFilters();
    initPageBitmaps();
    initPageCMS();
    initPageGrids();
    initPageSVGOutput();
    initPageAutosave();
    initPageImportExport();
    initPageUI();
    initPageMouse();
    initPageScrolling();
    initPageSnapping();
    initPageSteps();
    initPageWindows();
    initPageMisc();
    
    signalPresent().connect(sigc::mem_fun(*this, &InkscapePreferences::_presentPages));

    //calculate the size request for this dialog
    this->show_all_children();
    _page_list.expand_all();
    _page_list_model->foreach_iter(sigc::mem_fun(*this, &InkscapePreferences::SetMaxDialogSize));
    _getContents()->set_size_request(_max_dialog_width, _max_dialog_height);
    _page_list.collapse_all();
}

InkscapePreferences::~InkscapePreferences()
{
}

Gtk::TreeModel::iterator InkscapePreferences::AddPage(DialogPage& p, Glib::ustring title, int id)
{
    return AddPage(p, title, Gtk::TreeModel::iterator() , id);
}

Gtk::TreeModel::iterator InkscapePreferences::AddPage(DialogPage& p, Glib::ustring title, Gtk::TreeModel::iterator parent, int id)
{
    Gtk::TreeModel::iterator iter;
    if (parent)
       iter = _page_list_model->append((*parent).children());
    else
       iter = _page_list_model->append();
    Gtk::TreeModel::Row row = *iter;
    row[_page_list_columns._col_name] = title;
    row[_page_list_columns._col_id] = id;
    row[_page_list_columns._col_page] = &p;
    return iter;
}

void InkscapePreferences::initPageMouse()
{
    this->AddPage(_page_mouse, _("Mouse"), PREFS_PAGE_MOUSE);
    _mouse_sens.init ( "/options/cursortolerance/value", 0.0, 30.0, 1.0, 1.0, 8.0, true, false);
    _page_mouse.add_line( false, _("Grab sensitivity:"), _mouse_sens, _("pixels"),
                           _("How close on the screen you need to be to an object to be able to grab it with mouse (in screen pixels)"), false);
    _mouse_thres.init ( "/options/dragtolerance/value", 0.0, 20.0, 1.0, 1.0, 4.0, true, false);
    _page_mouse.add_line( false, _("Click/drag threshold:"), _mouse_thres, _("pixels"),
                           _("Maximum mouse drag (in screen pixels) which is considered a click, not a drag"), false);

    _mouse_use_ext_input.init( _("Use pressure-sensitive tablet (requires restart)"), "/options/useextinput/value", true);
    _page_mouse.add_line(true, "",_mouse_use_ext_input, "",
                        _("Use the capabilities of a tablet or other pressure-sensitive device. Disable this only if you have problems with the tablet (you can still use it as a mouse)"));

    _mouse_switch_on_ext_input.init( _("Switch tool based on tablet device (requires restart)"), "/options/switchonextinput/value", false);
    _page_mouse.add_line(true, "",_mouse_switch_on_ext_input, "",
                        _("Change tool as different devices are used on the tablet (pen, eraser, mouse)"));
}

void InkscapePreferences::initPageScrolling()
{
    this->AddPage(_page_scrolling, _("Scrolling"), PREFS_PAGE_SCROLLING);
    _scroll_wheel.init ( "/options/wheelscroll/value", 0.0, 1000.0, 1.0, 1.0, 40.0, true, false);
    _page_scrolling.add_line( false, _("Mouse wheel scrolls by:"), _scroll_wheel, _("pixels"),
                           _("One mouse wheel notch scrolls by this distance in screen pixels (horizontally with Shift)"), false);
    _page_scrolling.add_group_header( _("Ctrl+arrows"));
    _scroll_arrow_px.init ( "/options/keyscroll/value", 0.0, 1000.0, 1.0, 1.0, 10.0, true, false);
    _page_scrolling.add_line( true, _("Scroll by:"), _scroll_arrow_px, _("pixels"),
                           _("Pressing Ctrl+arrow key scrolls by this distance (in screen pixels)"), false);
    _scroll_arrow_acc.init ( "/options/scrollingacceleration/value", 0.0, 5.0, 0.01, 1.0, 0.35, false, false);
    _page_scrolling.add_line( true, _("Acceleration:"), _scroll_arrow_acc, "",
                           _("Pressing and holding Ctrl+arrow will gradually speed up scrolling (0 for no acceleration)"), false);
    _page_scrolling.add_group_header( _("Autoscrolling"));
    _scroll_auto_speed.init ( "/options/autoscrollspeed/value", 0.0, 5.0, 0.01, 1.0, 0.7, false, false);
    _page_scrolling.add_line( true, _("Speed:"), _scroll_auto_speed, "",
                           _("How fast the canvas autoscrolls when you drag beyond canvas edge (0 to turn autoscroll off)"), false);
    _scroll_auto_thres.init ( "/options/autoscrolldistance/value", -600.0, 600.0, 1.0, 1.0, -10.0, true, false);
    _page_scrolling.add_line( true, _("Threshold:"), _scroll_auto_thres, _("pixels"),
                           _("How far (in screen pixels) you need to be from the canvas edge to trigger autoscroll; positive is outside the canvas, negative is within the canvas"), false);
    _scroll_space.init ( _("Left mouse button pans when Space is pressed"), "/options/spacepans/value", false);
    _page_scrolling.add_line( false, "", _scroll_space, "",
                            _("When on, pressing and holding Space and dragging with left mouse button pans canvas (as in Adobe Illustrator). When off, Space temporarily switches to Selector tool (default)."));
    _wheel_zoom.init ( _("Mouse wheel zooms by default"), "/options/wheelzooms/value", false);
    _page_scrolling.add_line( false, "", _wheel_zoom, "",
                            _("When on, mouse wheel zooms without Ctrl and scrolls canvas with Ctrl; when off, it zooms with Ctrl and scrolls without Ctrl."));
}

void InkscapePreferences::initPageSnapping()
{
	
	_snap_indicator.init( _("Enable snap indicator"), "/options/snapindicator/value", true);
	_page_snapping.add_line( false, "", _snap_indicator, "",
			_("After snapping, a symbol is drawn at the point that has snapped"));
	    
	_snap_delay.init("/options/snapdelay/value", 0, 1000, 50, 100, 300, 0);
	_page_snapping.add_line( false, _("Delay (in msec):"), _snap_delay, "",
			_("Postpone snapping as long as the mouse is moving, and then wait an additional fraction of a second. This additional delay is specified here. When set to zero or to a very small number, snapping will be immediate"), true);
	
	_snap_closest_only.init( _("Only snap the node closest to the pointer"), "/options/snapclosestonly/value", false);
	_page_snapping.add_line( false, "", _snap_closest_only, "",
	_("Only try to snap the node that is initialy closest to the mouse pointer"));
	
	_snap_weight.init("/options/snapweight/value", 0, 1, 0.1, 0.2, 0.5, 1);
	_page_snapping.add_line( false, _("Weight factor:"), _snap_weight, "",
			_("When multiple snap solutions are found, then Inkscape can either prefer the closest transformation (when set to 0), or prefer the node that was initially the closest to the pointer (when set to 1)"), true);
	
	this->AddPage(_page_snapping, _("Snapping"), PREFS_PAGE_SNAPPING);
}

void InkscapePreferences::initPageSteps()
{
    this->AddPage(_page_steps, _("Steps"), PREFS_PAGE_STEPS);

    _steps_arrow.init ( "/options/nudgedistance/value", 0.0, 1000.0, 0.01, 1.0, 2.0, false, false);
    //nudgedistance is limited to 1000 in select-context.cpp: use the same limit here
    _page_steps.add_line( false, _("Arrow keys move by:"), _steps_arrow, _("px"),
                          _("Pressing an arrow key moves selected object(s) or node(s) by this distance (in px units)"), false);
    _steps_scale.init ( "/options/defaultscale/value", 0.0, 1000.0, 0.01, 1.0, 2.0, false, false);
    //defaultscale is limited to 1000 in select-context.cpp: use the same limit here
    _page_steps.add_line( false, _("> and < scale by:"), _steps_scale, _("px"),
                          _("Pressing > or < scales selection up or down by this increment (in px units)"), false);
    _steps_inset.init ( "/options/defaultoffsetwidth/value", 0.0, 3000.0, 0.01, 1.0, 2.0, false, false);
    _page_steps.add_line( false, _("Inset/Outset by:"), _steps_inset, _("px"),
                          _("Inset and Outset commands displace the path by this distance (in px units)"), false);
    _steps_compass.init ( _("Compass-like display of angles"), "/options/compassangledisplay/value", true);
    _page_steps.add_line( false, "", _steps_compass, "",
                            _("When on, angles are displayed with 0 at north, 0 to 360 range, positive clockwise; otherwise with 0 at east, -180 to 180 range, positive counterclockwise"));
    int const num_items = 17;
    Glib::ustring labels[num_items] = {"90", "60", "45", "36", "30", "22.5", "18", "15", "12", "10", "7.5", "6", "3", "2", "1", "0.5", _("None")};
    int values[num_items] = {2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 24, 30, 60, 90, 180, 360, 0};
    _steps_rot_snap.set_size_request(_sb_width);
    _steps_rot_snap.init("/options/rotationsnapsperpi/value", labels, values, num_items, 12);
    _page_steps.add_line( false, _("Rotation snaps every:"), _steps_rot_snap, _("degrees"),
                           _("Rotating with Ctrl pressed snaps every that much degrees; also, pressing [ or ] rotates by this amount"), false);
    _steps_zoom.init ( "/options/zoomincrement/value", 101.0, 500.0, 1.0, 1.0, 1.414213562, true, true);
    _page_steps.add_line( false, _("Zoom in/out by:"), _steps_zoom, _("%"),
                          _("Zoom tool click, +/- keys, and middle click zoom in and out by this multiplier"), false);
}

void InkscapePreferences::AddSelcueCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value)
{
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Show selection cue"), prefs_path + "/selcue", def_value);
    p.add_line( false, "", *cb, "", _("Whether selected objects display a selection cue (the same as in selector)"));
}

void InkscapePreferences::AddGradientCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value)
{
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Enable gradient editing"), prefs_path + "/gradientdrag", def_value);
    p.add_line( false, "", *cb, "", _("Whether selected objects display gradient editing controls"));
}

void InkscapePreferences::AddConvertGuidesCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value) {
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Conversion to guides uses edges instead of bounding box"), prefs_path + "/convertguides", def_value);
    p.add_line( false, "", *cb, "", _("Converting an object to guides places these along the object's true edges (imitating the object's shape), not along the bounding box."));
}

void InkscapePreferences::AddDotSizeSpinbutton(DialogPage &p, Glib::ustring const &prefs_path, double def_value)
{
    PrefSpinButton* sb = Gtk::manage( new PrefSpinButton);
    sb->init ( prefs_path + "/dot-size", 0.0, 1000.0, 0.1, 10.0, def_value, false, false);
    p.add_line( false, _("Ctrl+click dot size:"), *sb, _("times current stroke width"),
                       _("Size of dots created with Ctrl+click (relative to current stroke width)"),
                       false );
}


void StyleFromSelectionToTool(Glib::ustring const &prefs_path, StyleSwatch *swatch)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty()) {
        sp_desktop_message_stack(desktop)->flash(Inkscape::ERROR_MESSAGE,
                                       _("<b>No objects selected</b> to take the style from."));
        return;
    }
    SPItem *item = selection->singleItem();
    if (!item) {
        /* TODO: If each item in the selection has the same style then don't consider it an error.
         * Maybe we should try to handle multiple selections anyway, e.g. the intersection of the
         * style attributes for the selected items. */
        sp_desktop_message_stack(desktop)->flash(Inkscape::ERROR_MESSAGE,
                                       _("<b>More than one object selected.</b>  Cannot take style from multiple objects."));
        return;
    }

    SPCSSAttr *css = take_style_from_item (item);

    if (!css) return;

    // only store text style for the text tool
    if (prefs_path == "/tools/text") {
        css = sp_css_attr_unset_text (css);
    }

    // we cannot store properties with uris - they will be invalid in other documents
    css = sp_css_attr_unset_uris (css);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setStyle(prefs_path + "/style", css);
    sp_repr_css_attr_unref (css);

    // update the swatch
    if (swatch) {
        SPCSSAttr *css = prefs->getInheritedStyle(prefs_path + "/style");
        swatch->setStyle (css);
        sp_repr_css_attr_unref(css);
    }
}

void InkscapePreferences::AddNewObjectsStyle(DialogPage &p, Glib::ustring const &prefs_path, const gchar *banner)
{
    if (banner)
        p.add_group_header(banner);
    else
        p.add_group_header( _("Create new objects with:"));
    PrefRadioButton* current = Gtk::manage( new PrefRadioButton);
    current->init ( _("Last used style"), prefs_path + "/usecurrent", 1, true, 0);
    p.add_line( true, "", *current, "",
                _("Apply the style you last set on an object"));

    PrefRadioButton* own = Gtk::manage( new PrefRadioButton);
    Gtk::HBox* hb = Gtk::manage( new Gtk::HBox);
    Gtk::Alignment* align = Gtk::manage( new Gtk::Alignment);
    own->init ( _("This tool's own style:"), prefs_path + "/usecurrent", 0, false, current);
    align->set(0,0,0,0);
    align->add(*own);
    hb->add(*align);
    p.set_tip( *own, _("Each tool may store its own style to apply to the newly created objects. Use the button below to set it."));
    p.add_line( true, "", *hb, "", "");

    // style swatch
    Gtk::Button* button = Gtk::manage( new Gtk::Button(_("Take from selection"),true));
    StyleSwatch *swatch = 0;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    
    SPCSSAttr *css = prefs->getStyle(prefs_path + "/style");
    swatch = new StyleSwatch(css, _("This tool's style of new objects"));
    hb->add(*swatch);
    sp_repr_css_attr_unref(css);

    button->signal_clicked().connect( sigc::bind( sigc::ptr_fun(StyleFromSelectionToTool), prefs_path, swatch)  );
    own->changed_signal.connect( sigc::mem_fun(*button, &Gtk::Button::set_sensitive) );
    p.add_line( true, "", *button, "",
                _("Remember the style of the (first) selected object as this tool's style"));
}

void InkscapePreferences::initPageTools()
{
    Gtk::TreeModel::iterator iter_tools = this->AddPage(_page_tools, _("Tools"), PREFS_PAGE_TOOLS);
    _path_tools = _page_list.get_model()->get_path(iter_tools);

    _page_tools.add_group_header( _("Bounding box to use:"));
    _t_bbox_visual.init ( _("Visual bounding box"), "/tools/bounding_box", 0, false, 0); // 0 means visual
    _page_tools.add_line( true, "", _t_bbox_visual, "",
                            _("This bounding box includes stroke width, markers, filter margins, etc."));
    _t_bbox_geometric.init ( _("Geometric bounding box"), "/tools/bounding_box", 1, true, &_t_bbox_visual); // 1 means geometric
    _page_tools.add_line( true, "", _t_bbox_geometric, "",
                            _("This bounding box includes only the bare path"));

    _page_tools.add_group_header( _("Conversion to guides:"));
    _t_cvg_keep_objects.init ( _("Keep objects after conversion to guides"), "/tools/cvg_keep_objects", false);
    _page_tools.add_line( true, "", _t_cvg_keep_objects, "",
                            _("When converting an object to guides, don't delete the object after the conversion."));
    _t_cvg_convert_whole_groups.init ( _("Treat groups as a single object"), "/tools/cvg_convert_whole_groups", false);
    _page_tools.add_line( true, "", _t_cvg_convert_whole_groups, "",
                            _("Treat groups as a single object during conversion to guides rather than converting each child separately."));

    _pencil_average_all_sketches.init ( _("Average all sketches"), "/tools/freehand/pencil/average_all_sketches", false);
    _calligrapy_use_abs_size.init ( _("Width is in absolute units"), "/tools/calligraphic/abs_width", false);
    _calligrapy_keep_selected.init ( _("Select new path"), "/tools/calligraphic/keep_selected", true);
    _connector_ignore_text.init( _("Don't attach connectors to text objects"), "/tools/connector/ignoretext", true);

    //Selector
    this->AddPage(_page_selector, _("Selector"), iter_tools, PREFS_PAGE_TOOLS_SELECTOR);

    AddSelcueCheckbox(_page_selector, "/tools/select", false);
    _page_selector.add_group_header( _("When transforming, show:"));
    _t_sel_trans_obj.init ( _("Objects"), "/tools/select/show", "content", true, 0);
    _page_selector.add_line( true, "", _t_sel_trans_obj, "",
                            _("Show the actual objects when moving or transforming"));
    _t_sel_trans_outl.init ( _("Box outline"), "/tools/select/show", "outline", false, &_t_sel_trans_obj);
    _page_selector.add_line( true, "", _t_sel_trans_outl, "",
                            _("Show only a box outline of the objects when moving or transforming"));
    _page_selector.add_group_header( _("Per-object selection cue:"));
    _t_sel_cue_none.init ( _("None"), "/options/selcue/value", Inkscape::SelCue::NONE, false, 0);
    _page_selector.add_line( true, "", _t_sel_cue_none, "",
                            _("No per-object selection indication"));
    _t_sel_cue_mark.init ( _("Mark"), "/options/selcue/value", Inkscape::SelCue::MARK, true, &_t_sel_cue_none);
    _page_selector.add_line( true, "", _t_sel_cue_mark, "",
                            _("Each selected object has a diamond mark in the top left corner"));
    _t_sel_cue_box.init ( _("Box"), "/options/selcue/value", Inkscape::SelCue::BBOX, false, &_t_sel_cue_none);
    _page_selector.add_line( true, "", _t_sel_cue_box, "",
                            _("Each selected object displays its bounding box"));

    //Node
    this->AddPage(_page_node, _("Node"), iter_tools, PREFS_PAGE_TOOLS_NODE);
    AddSelcueCheckbox(_page_node, "/tools/nodes", true);
    AddGradientCheckbox(_page_node, "/tools/nodes", true);
    _page_node.add_group_header( _("Path outline:"));
    _t_node_pathoutline_color.init(_("Path outline color"), "/tools/nodes/highlight_color", 0xff0000ff);
    _page_node.add_line( false, _("Path outline color"), _t_node_pathoutline_color, "", _("Selects the color used for showing the path outline."), false);
    _t_node_pathflash_enabled.init ( _("Path outline flash on mouse-over"), "/tools/nodes/pathflash_enabled", false);
    _page_node.add_line( true, "", _t_node_pathflash_enabled, "", _("When hovering over a path, briefly flash its outline."));
    _t_node_pathflash_timeout.init("/tools/nodes/pathflash_timeout", 0, 10000.0, 100.0, 100.0, 1000.0, true, false);
    _page_node.add_line( false, _("Flash time"), _t_node_pathflash_timeout, "ms", _("Specifies how long the path outline will be visible after a mouse-over (in milliseconds). Specify 0 to have the outline shown until mouse leaves the path."), false);

    //Tweak
    this->AddPage(_page_tweak, _("Tweak"), iter_tools, PREFS_PAGE_TOOLS_TWEAK);
    this->AddNewObjectsStyle(_page_tweak, "/tools/tweak", _("Paint objects with:"));
    AddSelcueCheckbox(_page_tweak, "/tools/tweak", true);
    AddGradientCheckbox(_page_tweak, "/tools/tweak", false);

    //Zoom
    this->AddPage(_page_zoom, _("Zoom"), iter_tools, PREFS_PAGE_TOOLS_ZOOM);
    AddSelcueCheckbox(_page_zoom, "/tools/zoom", true);
    AddGradientCheckbox(_page_zoom, "/tools/zoom", false);

    //Shapes
    Gtk::TreeModel::iterator iter_shapes = this->AddPage(_page_shapes, _("Shapes"), iter_tools, PREFS_PAGE_TOOLS_SHAPES);
    _path_shapes = _page_list.get_model()->get_path(iter_shapes);
    this->AddSelcueCheckbox(_page_shapes, "/tools/shapes", true);
    this->AddGradientCheckbox(_page_shapes, "/tools/shapes", true);

    //Rectangle
    this->AddPage(_page_rectangle, _("Rectangle"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_RECT);
    this->AddNewObjectsStyle(_page_rectangle, "/tools/shapes/rect");
    this->AddConvertGuidesCheckbox(_page_rectangle, "/tools/shapes/rect", true);

    //3D box
    this->AddPage(_page_3dbox, _("3D Box"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_3DBOX);
    this->AddNewObjectsStyle(_page_3dbox, "/tools/shapes/3dbox");
    this->AddConvertGuidesCheckbox(_page_3dbox, "/tools/shapes/3dbox", true);

    //ellipse
    this->AddPage(_page_ellipse, _("Ellipse"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_ELLIPSE);
    this->AddNewObjectsStyle(_page_ellipse, "/tools/shapes/arc");

    //star
    this->AddPage(_page_star, _("Star"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_STAR);
    this->AddNewObjectsStyle(_page_star, "/tools/shapes/star");

    //spiral
    this->AddPage(_page_spiral, _("Spiral"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_SPIRAL);
    this->AddNewObjectsStyle(_page_spiral, "/tools/shapes/spiral");

    //Pencil
    this->AddPage(_page_pencil, _("Pencil"), iter_tools, PREFS_PAGE_TOOLS_PENCIL);
    this->AddSelcueCheckbox(_page_pencil, "/tools/freehand/pencil", true);
    this->AddNewObjectsStyle(_page_pencil, "/tools/freehand/pencil");
    this->AddDotSizeSpinbutton(_page_pencil, "/tools/freehand/pencil", 3.0);
    _page_pencil.add_group_header( _("Sketch mode"));
    _page_pencil.add_line( true, "", _pencil_average_all_sketches, "",
                            _("If on, the sketch result will be the normal average of all sketches made, instead of averaging the old result with the new sketch."));

    //Pen
    this->AddPage(_page_pen, _("Pen"), iter_tools, PREFS_PAGE_TOOLS_PEN);
    this->AddSelcueCheckbox(_page_pen, "/tools/freehand/pen", true);
    this->AddNewObjectsStyle(_page_pen, "/tools/freehand/pen");
    this->AddDotSizeSpinbutton(_page_pen, "/tools/freehand/pen", 3.0);

    //Calligraphy
    this->AddPage(_page_calligraphy, _("Calligraphy"), iter_tools, PREFS_PAGE_TOOLS_CALLIGRAPHY);
    this->AddSelcueCheckbox(_page_calligraphy, "/tools/calligraphic", false);
    this->AddNewObjectsStyle(_page_calligraphy, "/tools/calligraphic");
    _page_calligraphy.add_line( false, "", _calligrapy_use_abs_size, "",
                            _("If on, pen width is in absolute units (px) independent of zoom; otherwise pen width depends on zoom so that it looks the same at any zoom"));
    _page_calligraphy.add_line( false, "", _calligrapy_keep_selected, "",
                            _("If on, each newly created object will be selected (deselecting previous selection)"));
    //Paint Bucket
    this->AddPage(_page_paintbucket, _("Paint Bucket"), iter_tools, PREFS_PAGE_TOOLS_PAINTBUCKET);
    this->AddSelcueCheckbox(_page_paintbucket, "/tools/paintbucket", false);
    this->AddNewObjectsStyle(_page_paintbucket, "/tools/paintbucket");

    //Eraser
    this->AddPage(_page_eraser, _("Eraser"), iter_tools, PREFS_PAGE_TOOLS_ERASER);
    this->AddNewObjectsStyle(_page_eraser, "/tools/eraser");

    //LPETool
    this->AddPage(_page_lpetool, _("LPE Tool"), iter_tools, PREFS_PAGE_TOOLS_LPETOOL);
    this->AddNewObjectsStyle(_page_lpetool, "/tools/lpetool");

    //Text
    this->AddPage(_page_text, _("Text"), iter_tools, PREFS_PAGE_TOOLS_TEXT);
    this->AddSelcueCheckbox(_page_text, "/tools/text", true);
    this->AddGradientCheckbox(_page_text, "/tools/text", true);
    this->AddNewObjectsStyle(_page_text, "/tools/text");

    //Gradient
    this->AddPage(_page_gradient, _("Gradient"), iter_tools, PREFS_PAGE_TOOLS_GRADIENT);
    this->AddSelcueCheckbox(_page_gradient, "/tools/gradient", true);

    //Connector
    this->AddPage(_page_connector, _("Connector"), iter_tools, PREFS_PAGE_TOOLS_CONNECTOR);
    this->AddSelcueCheckbox(_page_connector, "/tools/connector", true);
    _page_connector.add_line(false, "", _connector_ignore_text, "",
            _("If on, connector attachment points will not be shown for text objects"));
    //Dropper
    this->AddPage(_page_dropper, _("Dropper"), iter_tools, PREFS_PAGE_TOOLS_DROPPER);
    this->AddSelcueCheckbox(_page_dropper, "/tools/dropper", true);
    this->AddGradientCheckbox(_page_dropper, "/tools/dropper", true);
}

void InkscapePreferences::initPageWindows()
{
    _win_save_geom.init ( _("Save and restore window geometry for each document"), "/options/savewindowgeometry/value", 1, true, 0);
    _win_save_geom_prefs.init ( _("Remember and use last window's geometry"), "/options/savewindowgeometry/value", 2, false, &_win_save_geom);
    _win_save_geom_off.init ( _("Don't save window geometry"), "/options/savewindowgeometry/value", 0, false, &_win_save_geom);

    _win_dockable.init ( _("Dockable"), "/options/dialogtype/value", 1, true, 0);
    _win_floating.init ( _("Floating"), "/options/dialogtype/value", 0, false, &_win_dockable);

    _win_hide_task.init ( _("Dialogs are hidden in taskbar"), "/options/dialogsskiptaskbar/value", true);
    _win_zoom_resize.init ( _("Zoom when window is resized"), "/options/stickyzoom/value", false);
    _win_show_close.init ( _("Show close button on dialogs"), "/dialogs/showclose", false);
    _win_ontop_none.init ( _("None"), "/options/transientpolicy/value", 0, false, 0);
    _win_ontop_normal.init ( _("Normal"), "/options/transientpolicy/value", 1, true, &_win_ontop_none);
    _win_ontop_agressive.init ( _("Aggressive"), "/options/transientpolicy/value", 2, false, &_win_ontop_none);

    _page_windows.add_group_header( _("Saving window geometry (size and position):"));
    _page_windows.add_line( true, "", _win_save_geom_off, "",
                            _("Let the window manager determine placement of all windows"));
    _page_windows.add_line( true, "", _win_save_geom_prefs, "",
                            _("Remember and use the last window's geometry (saves geometry to user preferences)"));
    _page_windows.add_line( true, "", _win_save_geom, "",
                            _("Save and restore window geometry for each document (saves geometry in the document)"));

    _page_windows.add_group_header( _("Dialog behavior (requires restart):"));
    _page_windows.add_line( true, "", _win_dockable, "",
                            _("Dockable"));
    _page_windows.add_line( true, "", _win_floating, "",
                            _("Floating"));

#ifndef WIN32 // non-Win32 special code to enable transient dialogs
    _page_windows.add_group_header( _("Dialogs on top:"));

    _page_windows.add_line( true, "", _win_ontop_none, "",
                            _("Dialogs are treated as regular windows"));
    _page_windows.add_line( true, "", _win_ontop_normal, "",
                            _("Dialogs stay on top of document windows"));
    _page_windows.add_line( true, "", _win_ontop_agressive, "",
                            _("Same as Normal but may work better with some window managers"));
#endif

#if GTK_VERSION_GE(2, 12)
	_page_windows.add_group_header( _("Dialog Transparency:"));
	_win_trans_focus.init("/dialogs/transparency/on-focus", 0.5, 1.0, 0.01, 0.1, 1.0, false, false);
	_page_windows.add_line( true, _("Opacity when focused:"), _win_trans_focus, "", "");
	_win_trans_blur.init("/dialogs/transparency/on-blur", 0.0, 1.0, 0.01, 0.1, 0.5, false, false);
	_page_windows.add_line( true, _("Opacity when unfocused:"), _win_trans_blur, "", "");
	_win_trans_time.init("/dialogs/transparency/animate-time", 0, 1000, 10, 100, 100, true, false);
	_page_windows.add_line( true, _("Time of opacity change animation:"), _win_trans_time, "ms", "");
#endif

    _page_windows.add_group_header( _("Miscellaneous:"));
#ifndef WIN32 // FIXME: Temporary Win32 special code to enable transient dialogs
    _page_windows.add_line( false, "", _win_hide_task, "",
                            _("Whether dialog windows are to be hidden in the window manager taskbar"));
#endif
    _page_windows.add_line( false, "", _win_zoom_resize, "",
                            _("Zoom drawing when document window is resized, to keep the same area visible (this is the default which can be changed in any window using the button above the right scrollbar)"));
    _page_windows.add_line( false, "", _win_show_close, "",
                            _("Whether dialog windows have a close button (requires restart)"));
    this->AddPage(_page_windows, _("Windows"), PREFS_PAGE_WINDOWS);
}

void InkscapePreferences::initPageClones()
{
    _clone_option_parallel.init ( _("Move in parallel"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_PARALLEL, true, 0);
    _clone_option_stay.init ( _("Stay unmoved"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_UNMOVED, false, &_clone_option_parallel);
    _clone_option_transform.init ( _("Move according to transform"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_NONE, false, &_clone_option_parallel);
    _clone_option_unlink.init ( _("Are unlinked"), "/options/cloneorphans/value",
                                  SP_CLONE_ORPHANS_UNLINK, true, 0);
    _clone_option_delete.init ( _("Are deleted"), "/options/cloneorphans/value",
                                  SP_CLONE_ORPHANS_DELETE, false, &_clone_option_unlink);

    _page_clones.add_group_header( _("When the original moves, its clones and linked offsets:"));
    _page_clones.add_line( true, "", _clone_option_parallel, "",
                           _("Clones are translated by the same vector as their original."));
    _page_clones.add_line( true, "", _clone_option_stay, "",
                           _("Clones preserve their positions when their original is moved."));
    _page_clones.add_line( true, "", _clone_option_transform, "",
                           _("Each clone moves according to the value of its transform= attribute. For example, a rotated clone will move in a different direction than its original."));
    _page_clones.add_group_header( _("When the original is deleted, its clones:"));
    _page_clones.add_line( true, "", _clone_option_unlink, "",
                           _("Orphaned clones are converted to regular objects."));
    _page_clones.add_line( true, "", _clone_option_delete, "",
                           _("Orphaned clones are deleted along with their original."));

    _page_clones.add_group_header( _("When duplicating original+clones:"));

    _clone_relink_on_duplicate.init ( _("Relink duplicated clones"), "/options/relinkclonesonduplicate/value", false);
    _page_clones.add_line(true, "", _clone_relink_on_duplicate, "",
                        _("When duplicating a selection containing both a clone and its original (possibly in groups), relink the duplicated clone to the duplicated original instead of the old original"));

    //TRANSLATORS: Heading for the Inkscape Preferences "Clones" Page
    this->AddPage(_page_clones, _("Clones"), PREFS_PAGE_CLONES);
}

void InkscapePreferences::initPageMasks()
{
    _mask_mask_on_top.init ( _("When applying, use the topmost selected object as clippath/mask"), "/options/maskobject/topmost", true);
    _page_mask.add_line(true, "", _mask_mask_on_top, "",
                        _("Uncheck this to use the bottom selected object as the clipping path or mask"));
    _mask_mask_remove.init ( _("Remove clippath/mask object after applying"), "/options/maskobject/remove", true);
    _page_mask.add_line(true, "", _mask_mask_remove, "",
                        _("After applying, remove the object used as the clipping path or mask from the drawing"));
    this->AddPage(_page_mask, _("Clippaths and masks"), PREFS_PAGE_MASKS);
}

void InkscapePreferences::initPageTransforms()
{
    _trans_scale_stroke.init ( _("Scale stroke width"), "/options/transform/stroke", true);
    _trans_scale_corner.init ( _("Scale rounded corners in rectangles"), "/options/transform/rectcorners", false);
    _trans_gradient.init ( _("Transform gradients"), "/options/transform/gradient", true);
    _trans_pattern.init ( _("Transform patterns"), "/options/transform/pattern", false);
    _trans_optimized.init ( _("Optimized"), "/options/preservetransform/value", 0, true, 0);
    _trans_preserved.init ( _("Preserved"), "/options/preservetransform/value", 1, false, &_trans_optimized);

    _page_transforms.add_line( false, "", _trans_scale_stroke, "",
                               _("When scaling objects, scale the stroke width by the same proportion"));
    _page_transforms.add_line( false, "", _trans_scale_corner, "",
                               _("When scaling rectangles, scale the radii of rounded corners"));
    _page_transforms.add_line( false, "", _trans_gradient, "",
                               _("Move gradients (in fill or stroke) along with the objects"));
    _page_transforms.add_line( false, "", _trans_pattern, "",
                               _("Move patterns (in fill or stroke) along with the objects"));
    _page_transforms.add_group_header( _("Store transformation:"));
    _page_transforms.add_line( true, "", _trans_optimized, "",
                               _("If possible, apply transformation to objects without adding a transform= attribute"));
    _page_transforms.add_line( true, "", _trans_preserved, "",
                               _("Always store transformation as a transform= attribute on objects"));

    this->AddPage(_page_transforms, _("Transforms"), PREFS_PAGE_TRANSFORMS);
}

void InkscapePreferences::initPageFilters()
{
    /* blur quality */
    _blur_quality_best.init ( _("Best quality (slowest)"), "/options/blurquality/value",
                                  BLUR_QUALITY_BEST, false, 0);
    _blur_quality_better.init ( _("Better quality (slower)"), "/options/blurquality/value",
                                  BLUR_QUALITY_BETTER, false, &_blur_quality_best);
    _blur_quality_normal.init ( _("Average quality"), "/options/blurquality/value",
                                  BLUR_QUALITY_NORMAL, true, &_blur_quality_best);
    _blur_quality_worse.init ( _("Lower quality (faster)"), "/options/blurquality/value",
                                  BLUR_QUALITY_WORSE, false, &_blur_quality_best);
    _blur_quality_worst.init ( _("Lowest quality (fastest)"), "/options/blurquality/value",
                                  BLUR_QUALITY_WORST, false, &_blur_quality_best);

    _page_filters.add_group_header( _("Gaussian blur quality for display:"));
    _page_filters.add_line( true, "", _blur_quality_best, "",
                           _("Best quality, but display may be very slow at high zooms (bitmap export always uses best quality)"));
    _page_filters.add_line( true, "", _blur_quality_better, "",
                           _("Better quality, but slower display"));
    _page_filters.add_line( true, "", _blur_quality_normal, "",
                           _("Average quality, acceptable display speed"));
    _page_filters.add_line( true, "", _blur_quality_worse, "",
                           _("Lower quality (some artifacts), but display is faster"));
    _page_filters.add_line( true, "", _blur_quality_worst, "",
                           _("Lowest quality (considerable artifacts), but display is fastest"));

    /* filter quality */
    _filter_quality_best.init ( _("Best quality (slowest)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_BEST, false, 0);
    _filter_quality_better.init ( _("Better quality (slower)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_BETTER, false, &_filter_quality_best);
    _filter_quality_normal.init ( _("Average quality"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_NORMAL, true, &_filter_quality_best);
    _filter_quality_worse.init ( _("Lower quality (faster)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_WORSE, false, &_filter_quality_best);
    _filter_quality_worst.init ( _("Lowest quality (fastest)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_WORST, false, &_filter_quality_best);

    _page_filters.add_group_header( _("Filter effects quality for display:"));
    _page_filters.add_line( true, "", _filter_quality_best, "",
                           _("Best quality, but display may be very slow at high zooms (bitmap export always uses best quality)"));
    _page_filters.add_line( true, "", _filter_quality_better, "",
                           _("Better quality, but slower display"));
    _page_filters.add_line( true, "", _filter_quality_normal, "",
                           _("Average quality, acceptable display speed"));
    _page_filters.add_line( true, "", _filter_quality_worse, "",
                           _("Lower quality (some artifacts), but display is faster"));
    _page_filters.add_line( true, "", _filter_quality_worst, "",
                           _("Lowest quality (considerable artifacts), but display is fastest"));

    /* show infobox */
    _show_filters_info_box.init( _("Show filter primitives infobox"), "/options/showfiltersinfobox/value", true);
    _page_filters.add_line(true, "", _show_filters_info_box, "",
                        _("Show icons and descriptions for the filter primitives available at the filter effects dialog."));

    this->AddPage(_page_filters, _("Filters"), PREFS_PAGE_FILTERS);
}


void InkscapePreferences::initPageSelecting()
{
    _sel_all.init ( _("Select in all layers"), "/options/kbselection/inlayer", PREFS_SELECTION_ALL, false, 0);
    _sel_current.init ( _("Select only within current layer"), "/options/kbselection/inlayer", PREFS_SELECTION_LAYER, true, &_sel_all);
    _sel_recursive.init ( _("Select in current layer and sublayers"), "/options/kbselection/inlayer", PREFS_SELECTION_LAYER_RECURSIVE, false, &_sel_all);
    _sel_hidden.init ( _("Ignore hidden objects and layers"), "/options/kbselection/onlyvisible", true);
    _sel_locked.init ( _("Ignore locked objects and layers"), "/options/kbselection/onlysensitive", true);
    _sel_layer_deselects.init ( _("Deselect upon layer change"), "/options/selection/layerdeselect", true);

    _page_select.add_group_header( _("Ctrl+A, Tab, Shift+Tab:"));
    _page_select.add_line( true, "", _sel_all, "",
                           _("Make keyboard selection commands work on objects in all layers"));
    _page_select.add_line( true, "", _sel_current, "",
                           _("Make keyboard selection commands work on objects in current layer only"));
    _page_select.add_line( true, "", _sel_recursive, "",
                           _("Make keyboard selection commands work on objects in current layer and all its sublayers"));
    _page_select.add_line( true, "", _sel_hidden, "",
                           _("Uncheck this to be able to select objects that are hidden (either by themselves or by being in a hidden layer)"));
    _page_select.add_line( true, "", _sel_locked, "",
                           _("Uncheck this to be able to select objects that are locked (either by themselves or by being in a locked layer)"));

    _page_select.add_line( false, "", _sel_layer_deselects, "",
                           _("Uncheck this to be able to keep the current objects selected when the current layer changes"));

    this->AddPage(_page_select, _("Selecting"), PREFS_PAGE_SELECTING);
}


void InkscapePreferences::initPageImportExport()
{
    _importexport_export.init("/dialogs/export/defaultxdpi/value", 0.0, 6000.0, 1.0, 1.0, PX_PER_IN, true, false);
    _page_importexport.add_line( false, _("Default export resolution:"), _importexport_export, _("dpi"),
                            _("Default bitmap resolution (in dots per inch) in the Export dialog"), false);
    _importexport_ocal_url.init("/options/ocalurl/str", true, g_strdup_printf("openclipart.org"));
    _page_importexport.add_line( false, _("Open Clip Art Library Server Name:"), _importexport_ocal_url, "",
        _("The server name of the Open Clip Art Library webdav server. It's used by the Import and Export to OCAL function."), true);
    _importexport_ocal_username.init("/options/ocalusername/str", true);
    _page_importexport.add_line( false, _("Open Clip Art Library Username:"), _importexport_ocal_username, "",
            _("The username used to log into Open Clip Art Library."), true);
    _importexport_ocal_password.init("/options/ocalpassword/str", false);
    _page_importexport.add_line( false, _("Open Clip Art Library Password:"), _importexport_ocal_password, "",
            _("The password used to log into Open Clip Art Library."), true);

    this->AddPage(_page_importexport, _("Import/Export"), PREFS_PAGE_IMPORTEXPORT);
}

#if ENABLE_LCMS
static void profileComboChanged( Gtk::ComboBoxText* combo )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int rowNum = combo->get_active_row_number();
    if ( rowNum < 1 ) {
        prefs->setString("/options/displayprofile/uri", "");
    } else {
        Glib::ustring active = combo->get_active_text();

        Glib::ustring path = get_path_for_profile(active);
        if ( !path.empty() ) {
            prefs->setString("/options/displayprofile/uri", path);
        }
    }
}

static void proofComboChanged( Gtk::ComboBoxText* combo )
{
    Glib::ustring active = combo->get_active_text();
    Glib::ustring path = get_path_for_profile(active);
    
    if ( !path.empty() ) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString("/options/softproof/uri", path);
    }
}

static void gamutColorChanged( Gtk::ColorButton* btn ) {
    Gdk::Color color = btn->get_color();
    gushort r = color.get_red();
    gushort g = color.get_green();
    gushort b = color.get_blue();

    gchar* tmp = g_strdup_printf("#%02x%02x%02x", (r >> 8), (g >> 8), (b >> 8) );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/options/softproof/gamutcolor", tmp);
    g_free(tmp);
}
#endif // ENABLE_LCMS

void InkscapePreferences::initPageCMS()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int const numIntents = 4;
    /* TRANSLATORS: see http://www.newsandtech.com/issues/2004/03-04/pt/03-04_rendering.htm */
    Glib::ustring intentLabels[numIntents] = {_("Perceptual"), _("Relative Colorimetric"), _("Saturation"), _("Absolute Colorimetric")};
    int intentValues[numIntents] = {0, 1, 2, 3};

#if !ENABLE_LCMS
    Gtk::Label* lbl = new Gtk::Label(_("(Note: Color management has been disabled in this build)"));
    _page_cms.add_line( false, "", *lbl, "", "", true);
#endif // !ENABLE_LCMS

    _page_cms.add_group_header( _("Display adjustment"));

    Glib::ustring tmpStr;
    std::list<Glib::ustring> sources = ColorProfile::getProfileDirs();
    for ( std::list<Glib::ustring>::const_iterator it = sources.begin(); it != sources.end(); ++it ) {
        gchar* part = g_strdup_printf( "\n%s", it->c_str() );
        tmpStr += part;
        g_free(part);
    }

    gchar* profileTip = g_strdup_printf(_("The ICC profile to use to calibrate display output.\nSearched directories:%s"), tmpStr.c_str());
    _page_cms.add_line( false, _("Display profile:"), _cms_display_profile, "",
                        profileTip, false);
    g_free(profileTip);
    profileTip = 0;

    _cms_from_display.init( _("Retrieve profile from display"), "/options/displayprofile/from_display", false);
    _page_cms.add_line( false, "", _cms_from_display, "",
#ifdef GDK_WINDOWING_X11
                        _("Retrieve profiles from those attached to displays via XICC."), false);
#else
                        _("Retrieve profiles from those attached to displays."), false);
#endif // GDK_WINDOWING_X11


    _cms_intent.init("/options/displayprofile/intent", intentLabels, intentValues, numIntents, 0);
    _page_cms.add_line( false, _("Display rendering intent:"), _cms_intent, "",
                        _("The rendering intent to use to calibrate display output."), false);

    _page_cms.add_group_header( _("Proofing"));

    _cms_softproof.init( _("Simulate output on screen"), "/options/softproof/enable", false);
    _page_cms.add_line( false, "", _cms_softproof, "",
                        _("Simulates output of target device."), false);

    _cms_gamutwarn.init( _("Mark out of gamut colors"), "/options/softproof/gamutwarn", false);
    _page_cms.add_line( false, "", _cms_gamutwarn, "",
                        _("Highlights colors that are out of gamut for the target device."), false);

    Glib::ustring colorStr = prefs->getString("/options/softproof/gamutcolor");
    Gdk::Color tmpColor( colorStr.empty() ? "#00ff00" : colorStr);
    _cms_gamutcolor.set_color( tmpColor );
    _page_cms.add_line( true, _("Out of gamut warning color:"), _cms_gamutcolor, "",
                        _("Selects the color used for out of gamut warning."), false);

    _page_cms.add_line( false, _("Device profile:"), _cms_proof_profile, "",
                        _("The ICC profile to use to simulate device output."), false);

    _cms_proof_intent.init("/options/softproof/intent", intentLabels, intentValues, numIntents, 0);
    _page_cms.add_line( false, _("Device rendering intent:"), _cms_proof_intent, "",
                        _("The rendering intent to use to calibrate display output."), false);

    _cms_proof_blackpoint.init( _("Black point compensation"), "/options/softproof/bpc", false);
    _page_cms.add_line( false, "", _cms_proof_blackpoint, "",
                        _("Enables black point compensation."), false);

    _cms_proof_preserveblack.init( _("Preserve black"), "/options/softproof/preserveblack", false);
    _page_cms.add_line( false, "", _cms_proof_preserveblack,
#if defined(cmsFLAGS_PRESERVEBLACK)
                        "",
#else
                        _("(LittleCMS 1.15 or later required)"),
#endif // defined(cmsFLAGS_PRESERVEBLACK)
                        _("Preserve K channel in CMYK -> CMYK transforms"), false);

#if !defined(cmsFLAGS_PRESERVEBLACK)
    _cms_proof_preserveblack.set_sensitive( false );
#endif // !defined(cmsFLAGS_PRESERVEBLACK)


#if ENABLE_LCMS
    {
        std::vector<Glib::ustring> names = ::Inkscape::colorprofile_get_display_names();
        Glib::ustring current = prefs->getString( "/options/displayprofile/uri" );

        gint index = 0;
        _cms_display_profile.append_text(_("<none>"));
        index++;
        for ( std::vector<Glib::ustring>::iterator it = names.begin(); it != names.end(); ++it ) {
            _cms_display_profile.append_text( *it );
            Glib::ustring path = get_path_for_profile(*it);
            if ( !path.empty() && path == current ) {
                _cms_display_profile.set_active(index);
            }
            index++;
        }
        if ( current.empty() ) {
            _cms_display_profile.set_active(0);
        }

        names = ::Inkscape::colorprofile_get_softproof_names();
        current = prefs->getString("/options/softproof/uri");
        index = 0;
        for ( std::vector<Glib::ustring>::iterator it = names.begin(); it != names.end(); ++it ) {
            _cms_proof_profile.append_text( *it );
            Glib::ustring path = get_path_for_profile(*it);
            if ( !path.empty() && path == current ) {
                _cms_proof_profile.set_active(index);
            }
            index++;
        }
    }

    _cms_gamutcolor.signal_color_set().connect( sigc::bind( sigc::ptr_fun(gamutColorChanged), &_cms_gamutcolor) );

    _cms_display_profile.signal_changed().connect( sigc::bind( sigc::ptr_fun(profileComboChanged), &_cms_display_profile) );
    _cms_proof_profile.signal_changed().connect( sigc::bind( sigc::ptr_fun(proofComboChanged), &_cms_proof_profile) );
#else
    // disable it, but leave it visible
    _cms_intent.set_sensitive( false );
    _cms_display_profile.set_sensitive( false );
    _cms_from_display.set_sensitive( false );
    _cms_softproof.set_sensitive( false );
    _cms_gamutwarn.set_sensitive( false );
    _cms_gamutcolor.set_sensitive( false );
    _cms_proof_intent.set_sensitive( false );
    _cms_proof_profile.set_sensitive( false );
    _cms_proof_blackpoint.set_sensitive( false );
    _cms_proof_preserveblack.set_sensitive( false );
#endif // ENABLE_LCMS

    this->AddPage(_page_cms, _("Color management"), PREFS_PAGE_CMS);
}

void InkscapePreferences::initPageGrids()
{
    _page_grids.add_group_header( _("Major grid line emphasizing"));

    _grids_no_emphasize_on_zoom.init( _("Don't emphasize gridlines when zoomed out"), "/options/grids/no_emphasize_when_zoomedout", false);
    _page_grids.add_line( false, "", _grids_no_emphasize_on_zoom, "", _("If set and zoomed out, the gridlines will be shown in normal color instead of major grid line color."), false);

    _page_grids.add_group_header( _("Default grid settings"));

    _page_grids.add_line( false, "", _grids_notebook, "", "", false);
    _grids_notebook.append_page(_grids_xy,     CanvasGrid::getName( GRID_RECTANGULAR ));
    _grids_notebook.append_page(_grids_axonom, CanvasGrid::getName( GRID_AXONOMETRIC ));
        _grids_xy_units.init("/options/grids/units");
        _grids_xy.add_line( false, _("Grid units:"), _grids_xy_units, "", "", false);
        _grids_xy_origin_x.init("/options/grids/xy/origin_x", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_xy_origin_y.init("/options/grids/xy/origin_y", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_xy.add_line( false, _("Origin X:"), _grids_xy_origin_x, "", _("X coordinate of grid origin"), false);
        _grids_xy.add_line( false, _("Origin Y:"), _grids_xy_origin_y, "", _("Y coordinate of grid origin"), false);
        _grids_xy_spacing_x.init("/options/grids/xy/spacing_x", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_xy_spacing_y.init("/options/grids/xy/spacing_y", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_xy.add_line( false, _("Spacing X:"), _grids_xy_spacing_x, "", _("Distance between vertical grid lines"), false);
        _grids_xy.add_line( false, _("Spacing Y:"), _grids_xy_spacing_y, "", _("Distance between horizontal grid lines"), false);

        _grids_xy_color.init(_("Grid line color:"), "/options/grids/xy/color", 0x0000ff20);
        _grids_xy.add_line( false, _("Grid line color:"), _grids_xy_color, "", _("Color used for normal grid lines"), false);
        _grids_xy_empcolor.init(_("Major grid line color:"), "/options/grids/xy/empcolor", 0x0000ff40);
        _grids_xy.add_line( false, _("Major grid line color:"), _grids_xy_empcolor, "", _("Color used for major (highlighted) grid lines"), false);
        _grids_xy_empspacing.init("/options/grids/xy/empspacing", 1.0, 1000.0, 1.0, 5.0, 5.0, true, false);
        _grids_xy.add_line( false, _("Major grid line every:"), _grids_xy_empspacing, "", "", false);
        _grids_xy_dotted.init( _("Show dots instead of lines"), "/options/grids/xy/dotted", false);
        _grids_xy.add_line( false, "", _grids_xy_dotted, "", _("If set, display dots at gridpoints instead of gridlines"), false);

    // CanvasAxonomGrid properties:
        _grids_axonom_units.init("/options/grids/axonom/units");
        _grids_axonom.add_line( false, _("Grid units:"), _grids_axonom_units, "", "", false);
        _grids_axonom_origin_x.init("/options/grids/axonom/origin_x", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_axonom_origin_y.init("/options/grids/axonom/origin_y", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_axonom.add_line( false, _("Origin X:"), _grids_axonom_origin_x, "", _("X coordinate of grid origin"), false);
        _grids_axonom.add_line( false, _("Origin Y:"), _grids_axonom_origin_y, "", _("Y coordinate of grid origin"), false);
        _grids_axonom_spacing_y.init("/options/grids/axonom/spacing_y", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_axonom.add_line( false, _("Spacing Y:"), _grids_axonom_spacing_y, "", _("Base length of z-axis"), false);
        _grids_axonom_angle_x.init("/options/grids/axonom/angle_x", -360.0, 360.0, 1.0, 10.0, 30.0, false, false);
        _grids_axonom_angle_z.init("/options/grids/axonom/angle_z", -360.0, 360.0, 1.0, 10.0, 30.0, false, false);
        _grids_axonom.add_line( false, _("Angle X:"), _grids_axonom_angle_x, "", _("Angle of x-axis"), false);
        _grids_axonom.add_line( false, _("Angle Z:"), _grids_axonom_angle_z, "", _("Angle of z-axis"), false);
        _grids_axonom_color.init(_("Grid line color:"), "/options/grids/axonom/color", 0x0000ff20);
        _grids_axonom.add_line( false, _("Grid line color:"), _grids_axonom_color, "", _("Color used for normal grid lines"), false);
        _grids_axonom_empcolor.init(_("Major grid line color:"), "/options/grids/axonom/empcolor", 0x0000ff40);
        _grids_axonom.add_line( false, _("Major grid line color:"), _grids_axonom_empcolor, "", _("Color used for major (highlighted) grid lines"), false);
        _grids_axonom_empspacing.init("/options/grids/axonom/empspacing", 1.0, 1000.0, 1.0, 5.0, 5.0, true, false);
        _grids_axonom.add_line( false, _("Major grid line every:"), _grids_axonom_empspacing, "", "", false);

    this->AddPage(_page_grids, _("Grids"), PREFS_PAGE_GRIDS);
}

void InkscapePreferences::initPageSVGOutput()
{
    _svgoutput_usenamedcolors.init( _("Use named colors"), "/options/svgoutput/usenamedcolors", false);
    _page_svgoutput.add_line( false, "", _svgoutput_usenamedcolors, "", _("If set, write the CSS name of the color when available (e.g. 'red' or 'magenta') instead of the numeric value"), false);

    _page_svgoutput.add_group_header( _("XML formatting"));

    _svgoutput_inlineattrs.init( _("Inline attributes"), "/options/svgoutput/inlineattrs", false);
    _page_svgoutput.add_line( false, "", _svgoutput_inlineattrs, "", _("Put attributes on the same line as the element tag"), false);

    _svgoutput_indent.init("/options/svgoutput/indent", 0.0, 1000.0, 1.0, 2.0, 2.0, true, false);
    _page_svgoutput.add_line( false, _("Indent, spaces:"), _svgoutput_indent, "", _("The number of spaces to use for indenting nested elements; set to 0 for no indentation"), false);

    _page_svgoutput.add_group_header( _("Path data"));

    _svgoutput_allowrelativecoordinates.init( _("Allow relative coordinates"), "/options/svgoutput/allowrelativecoordinates", true);
    _page_svgoutput.add_line( false, "", _svgoutput_allowrelativecoordinates, "", _("If set, relative coordinates may be used in path data"), false);

    _svgoutput_forcerepeatcommands.init( _("Force repeat commands"), "/options/svgoutput/forcerepeatcommands", false);
    _page_svgoutput.add_line( false, "", _svgoutput_forcerepeatcommands, "", _("Force repeating of the same path command (for example, 'L 1,2 L 3,4' instead of 'L 1,2 3,4')"), false);

    _page_svgoutput.add_group_header( _("Numbers"));

    _svgoutput_numericprecision.init("/options/svgoutput/numericprecision", 1.0, 16.0, 1.0, 2.0, 8.0, true, false);
    _page_svgoutput.add_line( false, _("Numeric precision:"), _svgoutput_numericprecision, "", _("How many digits to write after the decimal dot"), false);

    _svgoutput_minimumexponent.init("/options/svgoutput/minimumexponent", -32.0, -1, 1.0, 2.0, -8.0, true, false);
    _page_svgoutput.add_line( false, _("Minimum exponent:"), _svgoutput_minimumexponent, "", _("The smallest number written to SVG is 10 to the power of this exponent; anything smaller is written as zero."), false);

    this->AddPage(_page_svgoutput, _("SVG output"), PREFS_PAGE_SVGOUTPUT);
}

void InkscapePreferences::initPageUI()
{
    Glib::ustring languages[] = {_("System default"), _("am Amharic"), _("ar Arabic"), _("az Azerbaijani"), _("be Belarusian"), 
        _("bg Bulgarian"), _("bn Bengali"), _("br Breton"), _("ca Catalan"), _("ca@valencia Valencian Catalan"), _("cs Czech"), 
        _("da Danish"), _("de German"), _("dz Dzongkha"), _("el Greek"), _("en English"), _("en_AU English, as spoken in Australia"), 
        _("en_CA English, as spoken in Canada"), _("en_GB English, as spoken in Great Britain"), _("en_US@piglatin Pig Latin"), 
        _("eo Esperanto"),  _("es Spanish"), _("es_MX Spanish, as spoken in Mexico"),_("et Estonian"), _("eu Basque"), _("fi Finnish"), 
        _("fr French"), _("ga Irish"), _("gl Galician"), _("he Hebrew"), _("hr Croatian"), _("hu Hungarian"), _("hy Armenian"), 
        _("id Indonesian"), _("it Italian"), _("ja Japanese"), _("km Khmer"), _("ko Korean"), _("lt Lithuanian"), _("mk Macedonian"), 
        _("mn Mongolian"), _("nb Norwegian Bokmål"), _("ne Nepali"), _("nl Dutch"), _("nn Norwegian Nynorsk"), _("pa Panjabi"),
        _("pl Polish"), _("pt Portuguese"), _("pt_BR Portuguese, as spoken in Brazil"), _("ro Romanian"), _("ru Russian"), 
        _("rw Kinyarwanda"), _("sk Slovak"), _("sl Slovenian"), _("sq Albanian"), _("sr Serbian"), _("sr@latin Serbian in Latin script"), 
        _("sv Swedish"), _("th Thai"), _("tr Turkish"), _("uk Ukrainian"), _("vi Vietnamese"), _("zh_CN Chinese, as spoken in China"), 
        _("zh_TW Chinese, as spoken in Taiwan")};
    Glib::ustring langValues[] = {"", "am", "ar", "az", "be", "bg", "bn", "br", "ca", "ca@valencia", "cs", "da", "de",
        "dz", "el", "en", "en_AU", "en_CA", "en_GB", "en_US@piglatin", "eo", "es", "es_MX", "et", "eu", "fi", "fr", "ga",
        "gl", "he", "hr", "hu", "hy", "id", "it", "ja", "km", "ko", "lt", "mk", "mn", "nb", "ne", "nl", "nn", "pa",
        "pl", "pt", "pt_BR", "ro", "ru", "rw", "sk", "sl", "sq", "sr", "sr@latin", "sv", "th", "tr", "uk", "vi",
        "zh_CN", "zh_TW"};

    _ui_languages.init( "/ui/language", languages, langValues, G_N_ELEMENTS(languages), languages[0]);
    _page_ui.add_line( false, _("Language (requires restart):"), _ui_languages, "",
                              _("Set the language for menus and number-formats"), false);

     Glib::ustring sizeLabels[] = {_("Normal"), _("Medium"), _("Small")};
    int sizeValues[] = {0, 1, 2};

    _misc_small_toolbar.init( "/toolbox/small", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 0 );
    _page_ui.add_line( false, _("Commands bar icon size"), _misc_small_toolbar, "",
                              _("Set the size for the commands toolbar to use (requires restart)"), false);

    _misc_small_secondary.init( "/toolbox/secondary", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 1 );
    _page_ui.add_line( false, _("Tool controls bar icon size"), _misc_small_secondary, "",
                              _("Set the size for the secondary toolbar to use (requires restart)"), false);

    _misc_small_tools.init( "/toolbox/tools/small", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 0 );
    _page_ui.add_line( false, _("Main toolbar icon size"), _misc_small_tools, "",
                              _("Set the size for the main tools to use (requires restart)"), false);

    _misc_recent.init("/options/maxrecentdocuments/value", 0.0, 1000.0, 1.0, 1.0, 1.0, true, false);

    Gtk::HBox* recent_hbox = Gtk::manage(new Gtk::HBox());
    Gtk::Button* reset_recent = Gtk::manage(new Gtk::Button(_("Clear list")));
    reset_recent->signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::on_reset_open_recent_clicked));
    recent_hbox->pack_start(_misc_recent, false, false);
    recent_hbox->pack_start(*reset_recent, false, false);

    _page_ui.add_line( false, _("Maximum documents in Open Recent:"), *recent_hbox, "",
                              _("Set the maximum length of the Open Recent list in the File menu, or clear the list"), false);

    _ui_zoom_correction.init(300, 30, 1.00, 200.0, 1.0, 10.0, 1.0);
    _page_ui.add_line( false, _("Zoom correction factor (in %):"), _ui_zoom_correction, "",
                              _("Adjust the slider until the length of the ruler on your screen matches its real length. This information is used when zooming to 1:1, 1:2, etc., to display objects in their true sizes"), true);

    this->AddPage(_page_ui, _("Interface"), PREFS_PAGE_UI);
}


void InkscapePreferences::initPageAutosave()
{
    // Autosave options 
    _autosave_autosave_enable.init( _("Enable autosave (requires restart)"), "/options/autosave/enable", false);
    _page_autosave.add_line(false, "", _autosave_autosave_enable, "", _("Automatically save the current document(s) at a given interval, thus minimizing loss in case of a crash"), false);
    _autosave_autosave_interval.init("/options/autosave/interval", 1.0, 10800.0, 1.0, 10.0, 10.0, true, false);
    _page_autosave.add_line(true, _("Interval (in minutes):"), _autosave_autosave_interval, "", _("Interval (in minutes) at which document will be autosaved"), false);
    _autosave_autosave_path.init("/options/autosave/path", true);
    _page_autosave.add_line(true, _("Path:"), _autosave_autosave_path, "", _("The directory where autosaves will be written"), false);
    _autosave_autosave_max.init("/options/autosave/max", 1.0, 100.0, 1.0, 10.0, 10.0, true, false);
    _page_autosave.add_line(true, _("Maximum number of autosaves:"), _autosave_autosave_max, "", _("Maximum number of autosaved files; use this to limit the storage space used"), false);

    /* When changing the interval or enabling/disabling the autosave function,
     * update our running configuration
     *
     * FIXME!
     * the inkscape_autosave_init should be called AFTER the values have been changed
     * (which cannot be guaranteed from here) - use a PrefObserver somewhere
     */
    /*
    _autosave_autosave_enable.signal_toggled().connect( sigc::ptr_fun(inkscape_autosave_init), TRUE );
    _autosave_autosave_interval.signal_changed().connect( sigc::ptr_fun(inkscape_autosave_init), TRUE );
    */

    // -----------

    this->AddPage(_page_autosave, _("Autosave"), PREFS_PAGE_AUTOSAVE);
}

void InkscapePreferences::initPageBitmaps()
{
    {
        Glib::ustring labels[] = {_("None"), _("2x2"), _("4x4"), _("8x8"), _("16x16")};
        int values[] = {0, 1, 2, 3, 4};
        _misc_overs_bitmap.set_size_request(_sb_width);
        _misc_overs_bitmap.init("/options/bitmapoversample/value", labels, values, G_N_ELEMENTS(values), 1);
        _page_bitmaps.add_line( false, _("Oversample bitmaps:"), _misc_overs_bitmap, "", "", false);
    }

    _misc_bitmap_autoreload.init(_("Automatically reload bitmaps"), "/options/bitmapautoreload/value", true);
    _page_bitmaps.add_line( false, "", _misc_bitmap_autoreload, "",
                           _("Automatically reload linked images when file is changed on disk"));
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring choices = prefs->getString("/options/bitmapeditor/choices");
    if (!choices.empty()) {
        gchar** splits = g_strsplit(choices.data(), ",", 0);
        gint numIems = g_strv_length(splits);

        Glib::ustring labels[numIems];
        int values[numIems];
        for ( gint i = 0; i < numIems; i++) {
            values[i] = i;
            labels[i] = splits[i];
        }
        _misc_bitmap_editor.init("/options/bitmapeditor/value", labels, values, numIems, 0);
        _page_bitmaps.add_line( false, _("Bitmap editor:"), _misc_bitmap_editor, "", "", false);

        g_strfreev(splits);
    }

    _bitmap_copy_res.init("/options/createbitmap/resolution", 1.0, 6000.0, 1.0, 1.0, PX_PER_IN, true, false);
    _page_bitmaps.add_line( false, _("Resolution for Create Bitmap Copy:"), _bitmap_copy_res, _("dpi"),
                            _("Resolution used by the Create Bitmap Copy command"), false);

    this->AddPage(_page_bitmaps, _("Bitmaps"), PREFS_PAGE_BITMAPS);
}


void InkscapePreferences::initPageMisc()
{
    _misc_comment.init( _("Add label comments to printing output"), "/printing/debug/show-label-comments", false);
    _page_misc.add_line( false, "", _misc_comment, "",
                           _("When on, a comment will be added to the raw print output, marking the rendered output for an object with its label"), true);

    _misc_forkvectors.init( _("Prevent sharing of gradient definitions"), "/options/forkgradientvectors/value", true);
    _page_misc.add_line( false, "", _misc_forkvectors, "",
                           _("When on, shared gradient definitions are automatically forked on change; uncheck to allow sharing of gradient definitions so that editing one object may affect other objects using the same gradient"), true);

    _misc_simpl.init("/options/simplifythreshold/value", 0.0001, 1.0, 0.0001, 0.0010, 0.0010, false, false);
    _page_misc.add_line( false, _("Simplification threshold:"), _misc_simpl, "",
                           _("How strong is the Simplify command by default. If you invoke this command several times in quick succession, it will act more and more aggressively; invoking it again after a pause restores the default threshold."), false);

    _misc_latency_skew.init("/debug/latency/skew", 0.5, 2.0, 0.01, 0.10, 1.0, false, false);
    _page_misc.add_line( false, _("Latency skew:"), _misc_latency_skew, _("(requires restart)"),
                           _("Factor by which the event clock is skewed from the actual time (0.9766 on some systems)."), false);

    _misc_namedicon_delay.init( _("Pre-render named icons"), "/options/iconrender/named_nodelay", false);
    _page_misc.add_line( false, "", _misc_namedicon_delay, "",
                           _("When on, named icons will be rendered before displaying the ui. This is for working around bugs in GTK+ named icon notification"), true);

    this->AddPage(_page_misc, _("Misc"), PREFS_PAGE_MISC);
}

bool InkscapePreferences::SetMaxDialogSize(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    DialogPage* page = row[_page_list_columns._col_page];
    _page_frame.add(*page);
    this->show_all_children();
    Gtk:: Requisition sreq;
    this->size_request(sreq);
    _max_dialog_width=std::max(_max_dialog_width, sreq.width);
    _max_dialog_height=std::max(_max_dialog_height, sreq.height);
    _page_frame.remove();
    return false;
}

bool InkscapePreferences::PresentPage(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int desired_page = prefs->getInt("/dialogs/preferences/page", 0);
    if (desired_page == row[_page_list_columns._col_id])
    {
        if (desired_page >= PREFS_PAGE_TOOLS && desired_page <= PREFS_PAGE_TOOLS_DROPPER)
            _page_list.expand_row(_path_tools, false);
        if (desired_page >= PREFS_PAGE_TOOLS_SHAPES && desired_page <= PREFS_PAGE_TOOLS_SHAPES_SPIRAL)
            _page_list.expand_row(_path_shapes, false);
        _page_list.get_selection()->select(iter);
        return true;
    }
    return false;
}

void InkscapePreferences::on_reset_open_recent_clicked()
{
    GtkRecentManager* manager = gtk_recent_manager_get_default();
    GList* recent_list = gtk_recent_manager_get_items(manager);
    GList* element;
    GError* error;

    //Remove only elements that were added by Inkscape
    for (element = g_list_first(recent_list); element; element = g_list_next(element)){
        error = NULL;
        GtkRecentInfo* info = (GtkRecentInfo*) element->data;
        if (gtk_recent_info_has_application(info, g_get_prgname())){
            gtk_recent_manager_remove_item(manager, gtk_recent_info_get_uri(info), &error);
        }
        gtk_recent_info_unref (info);
    }
    g_list_free(recent_list);
}

void InkscapePreferences::on_pagelist_selection_changed()
{
    // show new selection
    Glib::RefPtr<Gtk::TreeSelection> selection = _page_list.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter)
    {
        if (_current_page)
            _page_frame.remove();
        Gtk::TreeModel::Row row = *iter;
        _current_page = row[_page_list_columns._col_page];
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt("/dialogs/preferences/page", row[_page_list_columns._col_id]);
        _page_title.set_markup("<span size='large'><b>" + row[_page_list_columns._col_name] + "</b></span>");
        _page_frame.add(*_current_page);
        _current_page->show();
        while (Gtk::Main::events_pending())
        {
            Gtk::Main::iteration();
        }
        this->show_all_children();
    }
}

void InkscapePreferences::_presentPages()
{
    _page_list_model->foreach_iter(sigc::mem_fun(*this, &InkscapePreferences::PresentPage));
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
