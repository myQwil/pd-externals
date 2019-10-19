#include "hot.h"

/* -------------------------- hot == -------------------------- */

static t_class *hee_class;
static t_class *hee_proxy_class;

static void hee_bang(t_hot *x) {
	outlet_float(x->x_obj.ob_outlet, x->x_f1 == x->x_f2);
}

static void *hee_new(t_symbol *s, int ac, t_atom *av) {
	return (hot_new(hee_class, hee_proxy_class, hee_bang, s, ac, av));
}

void setup_0x230x3d0x3d(void) {
	hee_class = class_new(gensym("#=="),
		(t_newmethod)hee_new, (t_method)hot_free,
		sizeof(t_hot), 0,
		A_GIMME, 0);
	class_addbang(hee_class, hee_bang);
	class_addfloat(hee_class, hot_float);
	class_addmethod(hee_class, (t_method)hot_loadbang,
		gensym("loadbang"), A_DEFFLOAT, 0);

	hee_proxy_class = class_new(gensym("_#==_proxy"), 0, 0,
		sizeof(t_hot_proxy), CLASS_PD | CLASS_NOINLET, 0);
	class_addbang(hee_proxy_class, hot_proxy_bang);
	class_addfloat(hee_proxy_class, hot_proxy_float);

	class_sethelpsymbol(hee_class, gensym("hotbinops2"));
}
