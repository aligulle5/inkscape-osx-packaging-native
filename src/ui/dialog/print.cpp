/** @file
 * @brief Print dialog
 */
/* Authors:
 *   Kees Cook <kees@outflux.net>
 *
 * Copyright (C) 2007 Kees Cook
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef WIN32
#include <io.h>
#endif

#include <gtkmm/stock.h>
#include "print.h"

#include "extension/internal/cairo-render-context.h"
#include "extension/internal/cairo-renderer.h"
#include "ui/widget/rendering-options.h"
#include "document.h"

#include "unit-constants.h"
#include "helper/png-write.h"
#include "svg/svg-color.h"
#include "io/sys.h"



#ifdef WIN32
#include <windows.h>
#include <commdlg.h>
#include <cairo.h>
#include <cairo-win32.h>



static cairo_surface_t *
_cairo_win32_printing_surface_create (HDC hdc)
{
    int x, y, x_dpi, y_dpi, x_off, y_off, depth;
    XFORM xform;
    cairo_surface_t *surface;

    x = GetDeviceCaps (hdc, HORZRES);
    y = GetDeviceCaps (hdc, VERTRES);

    x_dpi = GetDeviceCaps (hdc, LOGPIXELSX);
    y_dpi = GetDeviceCaps (hdc, LOGPIXELSY);

    x_off = GetDeviceCaps (hdc, PHYSICALOFFSETX);
    y_off = GetDeviceCaps (hdc, PHYSICALOFFSETY);

    depth = GetDeviceCaps(hdc, BITSPIXEL);

    SetGraphicsMode (hdc, GM_ADVANCED);
    xform.eM11 = x_dpi/72.0;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = y_dpi/72.0;
    xform.eDx = -x_off;
    xform.eDy = -y_off;
    SetWorldTransform (hdc, &xform);

    surface = cairo_win32_printing_surface_create (hdc);
    
    /**
		Read fallback dpi from device capabilities. Was a workaround for a bug patched
		in cairo 1.5.14. Without this, fallback defaults to 300dpi, which is quite acceptable.
		Going higher can cause spool size and memory problems.
	*/
    // cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);

    return surface;
}
#endif



static void
draw_page (GtkPrintOperation */*operation*/,
           GtkPrintContext   *context,
           gint               /*page_nr*/,
           gpointer           user_data)
{
    struct workaround_gtkmm *junk = (struct workaround_gtkmm*)user_data;
    //printf("%s %d\n",__FUNCTION__, page_nr);

    if (junk->_tab->as_bitmap()) {
        // Render as exported PNG
        gdouble width = sp_document_width(junk->_doc);
        gdouble height = sp_document_height(junk->_doc);
        gdouble dpi = junk->_tab->bitmap_dpi();
        std::string tmp_png;
        std::string tmp_base = "inkscape-print-png-XXXXXX";

        int tmp_fd;
        if ( (tmp_fd = Inkscape::IO::file_open_tmp (tmp_png, tmp_base)) >= 0) {
            close(tmp_fd);

            guint32 bgcolor = 0x00000000;
            Inkscape::XML::Node *nv = sp_repr_lookup_name (junk->_doc->rroot, "sodipodi:namedview");
            if (nv && nv->attribute("pagecolor"))
                bgcolor = sp_svg_read_color(nv->attribute("pagecolor"), 0xffffff00);
            if (nv && nv->attribute("inkscape:pageopacity"))
                bgcolor |= SP_COLOR_F_TO_U(sp_repr_get_double_attribute (nv, "inkscape:pageopacity", 1.0));

            sp_export_png_file(junk->_doc, tmp_png.c_str(), 0.0, 0.0,
                width, height,
                (unsigned long)(width * dpi / PX_PER_IN),
                (unsigned long)(height * dpi / PX_PER_IN),
                dpi, dpi, bgcolor, NULL, NULL, true, NULL);

            // This doesn't seem to work:
            //context->set_cairo_context ( Cairo::Context::create (Cairo::ImageSurface::create_from_png (tmp_png) ), dpi, dpi );
            //
            // so we'll use a surface pattern blat instead...
            //
            // but the C++ interface isn't implemented in cairomm:
            //context->get_cairo_context ()->set_source_surface(Cairo::ImageSurface::create_from_png (tmp_png) );
            //
            // so do it in C:
            {
                Cairo::RefPtr<Cairo::ImageSurface> png = Cairo::ImageSurface::create_from_png (tmp_png);
                cairo_t *cr = gtk_print_context_get_cairo_context (context);
		cairo_matrix_t m;
		cairo_get_matrix(cr, &m);
		cairo_scale(cr, PT_PER_IN / dpi, PT_PER_IN / dpi);
                // FIXME: why is the origin offset??
                cairo_set_source_surface(cr, png->cobj(), -16.0, -16.0);
		cairo_paint(cr);
		cairo_set_matrix(cr, &m);
            }

            // Clean up
            unlink (tmp_png.c_str());
        }
        else {
            g_warning(_("Could not open temporary PNG for bitmap printing"));
        }
    }
    else {
        // Render as vectors
        Inkscape::Extension::Internal::CairoRenderer renderer;
        Inkscape::Extension::Internal::CairoRenderContext *ctx = renderer.createContext();

        // ctx->setPSLevel(CAIRO_PS_LEVEL_3);
        ctx->setTextToPath(false);
        ctx->setFilterToBitmap(true);
        ctx->setBitmapResolution(72);

        cairo_t *cr = gtk_print_context_get_cairo_context (context);
        cairo_surface_t *surface = cairo_get_target(cr);


/**
	Call cairo_win32_printing_surface directly as a workaround until GTK uses this call.
	When GTK uses cairo_win32_printing_surface this automatically reverts.
*/
#ifdef WIN32
        if (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_WIN32) {
        HDC dc = cairo_win32_surface_get_dc (surface);
        surface = _cairo_win32_printing_surface_create (dc);
        }
#endif

        bool ret = ctx->setSurfaceTarget (surface, true);        if (ret) {
            ret = renderer.setupDocument (ctx, junk->_doc, TRUE, NULL);
            if (ret) {
                renderer.renderItem(ctx, junk->_base);
                ret = ctx->finish();
            }
            else {
                g_warning(_("Could not set up Document"));
            }
        }
        else {
            g_warning(_("Failed to set CairoRenderContext"));
        }

        // Clean up
        renderer.destroyContext(ctx);
    }

}

static GObject*
create_custom_widget (GtkPrintOperation */*operation*/,
                      gpointer           user_data)
{
    //printf("%s\n",__FUNCTION__);
    return G_OBJECT(user_data);
}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   */*context*/,
             gpointer           /*user_data*/)
{
    //printf("%s\n",__FUNCTION__);
    gtk_print_operation_set_n_pages (operation, 1);
}

namespace Inkscape {
namespace UI {
namespace Dialog {

Print::Print(SPDocument *doc, SPItem *base) :
    _doc (doc),
    _base (base)
{
    g_assert (_doc);
    g_assert (_base);

    _printop = gtk_print_operation_new ();

    // set up dialog title, based on document name
    gchar *jobname = _doc->name ? _doc->name : _("SVG Document");
    Glib::ustring title = _("Print");
    title += " ";
    title += jobname;
    gtk_print_operation_set_job_name (_printop, title.c_str());

    // set up paper size to match the document size
    gtk_print_operation_set_unit (_printop, GTK_UNIT_POINTS);
    GtkPageSetup *page_setup = gtk_page_setup_new();
    gdouble doc_width = sp_document_width(_doc) * PT_PER_PX;
    gdouble doc_height = sp_document_height(_doc) * PT_PER_PX;
    GtkPaperSize *paper_size = gtk_paper_size_new_custom("custom", "custom",
                                doc_width, doc_height, GTK_UNIT_POINTS);
    gtk_page_setup_set_paper_size (page_setup, paper_size);
#ifndef WIN32
    gtk_print_operation_set_default_page_setup (_printop, page_setup);
#endif
    gtk_print_operation_set_use_full_page (_printop, TRUE);

    // set up signals
    _workaround._doc = _doc;
    _workaround._base = _base;
    _workaround._tab = &_tab;
    g_signal_connect (_printop, "create-custom-widget", G_CALLBACK (create_custom_widget), _tab.gobj());
    g_signal_connect (_printop, "begin-print", G_CALLBACK (begin_print), NULL);
    g_signal_connect (_printop, "draw-page", G_CALLBACK (draw_page), &_workaround);

    // build custom preferences tab
    gtk_print_operation_set_custom_tab_label (_printop, _("Rendering"));
}

Gtk::PrintOperationResult Print::run(Gtk::PrintOperationAction, Gtk::Window &parent_window)
{
    gtk_print_operation_run (_printop, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
	    parent_window.gobj(), NULL);
    return Gtk::PRINT_OPERATION_RESULT_APPLY;
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
