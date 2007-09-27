#include "court_widget.h"
#include <math.h>
#include <list>

wnobj::wnobj(partic_t & p, unsigned int t) : _p(p), _t(t)
{
}

void wnobj::set_anchor(bool b)
{
	if (_t & et_center)
		return;
	_p.set_anchor(b);
}

void wnobj::set_center()
{
	_t = _t | et_center;
	_p.set_anchor(true);
}

void wnobj::draw_ball(cairo_t *cr, double x, double y)
{
	const double r = 6;
	cairo_save(cr);
	cairo_arc(cr, x + r/3, y + r/3, r, 0, 2 * M_PI);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_fill(cr);
	cairo_arc(cr, x, y, r, 0, 2 * M_PI);
	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_fill(cr);
	cairo_arc(cr, x - r/3, y - r/3, r/3, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
	cairo_fill(cr);
	cairo_restore(cr);
}

void wnobj::draw_line(cairo_t *cr, double x1, double y1, double x2, double y2)
{
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	cairo_stroke(cr);
}

void wnobj::draw_text(cairo_t *cr, double x, double y, double w, double h, PangoLayout * layout)
{
	cairo_save(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, x, y, w, h);
	cairo_fill(cr);
	cairo_restore(cr);
	cairo_move_to(cr, x, y);
	pango_cairo_show_layout(cr, layout);
}

void wnobj::draw_spring(cairo_t *cr, const spring_t & s)
{
	vector_t &a = s.getA().getP();
	vector_t &b = s.getB().getP();
	draw_line(cr, a.x, a.y, b.x, b.y);
}

void ball_t::draw(cairo_t *cr)
{
	vector_t v = getP().getP();
	draw_ball(cr, v.x, v.y);
}

word_t::~word_t()
{
	g_object_unref(_layout);
}

void word_t::draw(cairo_t *cr)
{
	point_t<single> left_top = _p.get_left_top();
	tsize_t<single> & size = _p.get_size();
	draw_text(cr, left_top.x, left_top.y, size.w, size.h, _layout);
}

wncourt_t::wncourt_t() : _env(), _scene(), _newton(_scene, _env)
{
}

wncourt_t::~wncourt_t()
{
	clear();
}

word_t * wncourt_t::create_word(PangoLayout *layout)
{
	int w, h;
	pango_layout_get_pixel_size(layout, &w, &h);
	partic_t * p = _scene.create_partic(10, w, h);
	word_t * b = new word_t(*p, layout);
	_wnobjs.push_back(b);
	return b;
}

ball_t * wncourt_t::create_ball(const char *text)
{
	partic_t * p = _scene.create_partic(10, 12, 12);
	ball_t * b = new ball_t(*p, text);
	_wnobjs.push_back(b);
	return b;
}

void wncourt_t::create_spring(const wnobj * w, const wnobj * b, float springlength, float coeff)
{
	_scene.create_spring(w->getP(), b->getP(), springlength, coeff);
}

void wncourt_t::set_center(wnobj * cb)
{
	the_center = cb;
	the_center->set_center();
	_scene.set_center(&cb->getP());
}

void wncourt_t::clear()
{
	for (std::vector<wnobj *>::const_iterator it = _wnobjs.begin();it != _wnobjs.end(); ++it) {
		delete *it;
	}
	_wnobjs.clear();
	_scene.clear();
}

bool wncourt_t::hit(int x, int y, wnobj ** b)
{
	vector_t p((single)x, (single)y, 0);
	for(std::vector<wnobj *>::iterator it = _wnobjs.begin(); it != _wnobjs.end(); ++it) {
		if ((*it)->getP().hit(p)) {
			*b = *it;
			return true;
		}
	}
	*b = NULL;
	return false;
}

gboolean WnCourt::expose_event_callback (GtkWidget *widget, GdkEventExpose *event, WnCourt *wncourt)
{
	cairo_t *cr = gdk_cairo_create(widget->window);
	wncourt->draw_wnobjs(cr);
	cairo_destroy(cr);
	return TRUE;
}

void WnCourt::draw_wnobjs(cairo_t *cr)
{
	cairo_set_line_width(cr, 1);
	vector<spring_t *> & springs = _court->get_scene().get_springs();
	for(vector<spring_t *>::iterator it = springs.begin(); it != springs.end(); ++it) {
		wnobj::draw_spring(cr, **it);
	}
	std::vector<wnobj *> & partics = _court->get_wnobjs();
	for(std::vector<wnobj *>::iterator it = partics.begin(); it != partics.end(); ++it) {
		(*it)->draw(cr);
	}
}

void WnCourt::on_destroy_callback (GtkObject *object, WnCourt *wncourt)
{
	delete wncourt;
}

void WnCourt::on_realize_callback(GtkWidget *widget, WnCourt *wncourt)
{
	GdkCursor* cursor = gdk_cursor_new(GDK_LEFT_PTR);
	gdk_window_set_cursor(widget->window, cursor);
	gdk_cursor_unref(cursor);
}

gboolean WnCourt::on_button_press_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt)
{
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 1) {
			wnobj * b;
			if (wncourt->_court->hit((int)(event->x), (int)(event->y), &b)) {
				wncourt->dragball = b;
				wncourt->dragball->set_anchor(true);
			} else {
				wncourt->panning = true;
			}
			wncourt->oldX = (int)(event->x);
			wncourt->oldY = (int)(event->y);
		}
	}
	return TRUE;
}

gboolean WnCourt::on_button_release_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt)
{
	if (event->button == 1) {
		if (wncourt->dragball) {
			wncourt->dragball->set_anchor(false);
			wncourt->_court->get_env().reset();
			wncourt->dragball = NULL;
		}
		wncourt->panning = false;
	}
	return TRUE;
}

gboolean WnCourt::on_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event , WnCourt *wncourt)
{
	if (event->state & GDK_BUTTON1_MASK) {
		if (wncourt->dragball) {
			vector_t dv((single)(event->x - wncourt->oldX), (single)(event->y - wncourt->oldY), 0);
			wncourt->dragball->getP().getP().add(dv);
		} else {
			if (wncourt->panning) {
				wncourt->_court->get_scene().pan(vector_t((single)(event->x - wncourt->oldX), (single)(event->y - wncourt->oldY), 0));
			}
		}
		wncourt->oldX = (int)(event->x);
		wncourt->oldY = (int)(event->y);
	}
	return TRUE;
}

gint WnCourt::do_render_scene(gpointer data)
{
	WnCourt *wncourt = static_cast<WnCourt *>(data);
	wncourt->render_scene();
	return TRUE;
}

void WnCourt::render_scene()
{
	_court->update(1.0f);
	gtk_widget_queue_draw(drawing_area);
}

WnCourt::WnCourt() : _init_angle(0), init_spring_length(81), dragball(NULL)
{
	_court = new wncourt_t();
	widget_width = 400;
	widget_height = 300;

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request (drawing_area, widget_width, widget_height);
	gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK);
	GdkColor color;
	color.red = 65535;
	color.green = 65535;
	color.blue = 65535;
	gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &color);
	g_signal_connect (G_OBJECT (drawing_area), "expose_event", G_CALLBACK (expose_event_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "destroy", G_CALLBACK (on_destroy_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "realize", G_CALLBACK (on_realize_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "button_press_event", G_CALLBACK (on_button_press_event_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "button_release_event", G_CALLBACK (on_button_release_event_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event", G_CALLBACK (on_motion_notify_event_callback), this);
	gtk_widget_show(drawing_area);
	timeout = g_timeout_add(100, do_render_scene, this);
}

WnCourt::~WnCourt()
{
	g_source_remove(timeout);
	delete _court;
}

struct WnUserData {
	WnUserData(const char *oword_, std::string &type_, std::list<std::string> &wordlist_, std::string &gloss_): oword(oword_), type(type_), wordlist(wordlist_), gloss(gloss_) {}
	const gchar *oword;
	std::string &type;
	std::list<std::string> &wordlist;
	std::string &gloss;
};

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	WnUserData *Data = (WnUserData *)user_data;
	if (strcmp(element, "type")==0) {
		Data->type.assign(text, text_len);
	} else if (strcmp(element, "word")==0) {
		std::string word(text, text_len);
		if (word != Data->oword) {
			Data->wordlist.push_back(word);
		}
	} else if (strcmp(element, "gloss")==0) {
		Data->gloss.assign(text, text_len);
	}
}

static void wordnet2result(gchar *Word, gchar *WordData, std::string &type, std::list<std::string> &wordlist, std::string &gloss)
{
	guint32 data_size = *reinterpret_cast<guint32 *>(WordData);
	type.clear();
	wordlist.clear();
	gloss.clear();
	WnUserData Data(Word, type, wordlist, gloss);
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, WordData + sizeof(guint32) + sizeof(char), data_size - 2, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

void WnCourt::set_word(const gchar *orig_word, gchar **Word, gchar ***WordData)
{
	if (Word == NULL) {
	}
	CreateWord(orig_word);
	Push();
	std::string type;
	std::list<std::string> wordlist;
	std::string gloss;
	size_t i = 0;
	size_t j;
	do {
		j = 0;
		do {
			CreateNode(Word[i]);
			Push();
			wordnet2result(Word[i], WordData[i][j], type, wordlist, gloss);
			for (std::list<std::string>::iterator it = wordlist.begin(); it != wordlist.end(); ++it) {
				CreateWord(it->c_str());
			}
			Pop();
			j++;
		} while (WordData[i][j]);
		i++;
	} while (Word[i]);
}

GtkWidget *WnCourt::get_widget()
{
	return drawing_area;
}

void WnCourt::CreateWord(const char *text)
{
	PangoLayout *layout = gtk_widget_create_pango_layout(drawing_area, text);
	newobj = _court->create_word(layout);
	wnobj *top = get_top();
	if (top) {
		_court->create_spring(newobj, top, init_spring_length);
		newobj->getP().getP() = get_next_pos(get_top()->getP().getP());
	} else {
		newobj->getP().getP() = get_center_pos();
		_court->set_center(newobj);
	}
}

void WnCourt::CreateNode(const char *text)
{
	newobj = _court->create_ball(text);
	wnobj *top = get_top();
	if (top) {
		_court->create_spring(newobj, top, init_spring_length, 0.4f);
		newobj->getP().getP() = get_next_pos(top->getP().getP());
	} else {
		newobj->getP().getP() = get_center_pos();
	}
}

vector_t WnCourt::get_center_pos()
{
	return vector_t(widget_width/2, widget_height/2, 0);
}

vector_t WnCourt::get_next_pos(vector_t &center)
{
	vector_t d(init_spring_length, 0, 0);
	d.rot(M_PI_10*_init_angle++);
	return center+d;
}

void WnCourt::Push()
{
	_wnstack.push_back(newobj);
}

void WnCourt::Pop()
{
	newobj = get_top();
	_wnstack.pop_back();
}

wnobj *WnCourt::get_top()
{
	if (_wnstack.empty())
		return NULL;
	else
		return _wnstack.back();
}
