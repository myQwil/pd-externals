#include "m_pd.h"

typedef union {
	t_float f;
	struct {
		unsigned int mantissa : 23;
		unsigned int exponent : 8;
		unsigned int sign : 1;
	} u;
} ufloat;

/* -------------------------- gloat -------------------------- */

static t_class *gloat_class;

typedef struct _gloat {
	t_object x_obj;
	t_float x_m, x_e, x_s;
} t_gloat;

static void gloat_bang(t_gloat *x) {
	ufloat uf;
	uf.u.sign = x->x_s;
	uf.u.exponent = x->x_e;
	uf.u.mantissa = x->x_m;
	outlet_float(x->x_obj.ob_outlet, uf.f);
}

static void gloat_float(t_gloat *x, t_float f) {
	ufloat uf;
	uf.u.sign = x->x_s;
	uf.u.exponent = x->x_e;
	uf.u.mantissa = x->x_m = f;
	outlet_float(x->x_obj.ob_outlet, uf.f);
}

static void *gloat_new(t_symbol *s, int argc, t_atom *argv) {
	t_gloat *x = (t_gloat *)pd_new(gloat_class);
	switch (argc)
	{	case 3: x->x_s = atom_getfloat(argv+2);
		case 2: x->x_e = atom_getfloat(argv+1);
		case 1: x->x_m = atom_getfloat(argv);   }
	floatinlet_new(&x->x_obj, &x->x_m);
	floatinlet_new(&x->x_obj, &x->x_e);
	floatinlet_new(&x->x_obj, &x->x_s);
	outlet_new(&x->x_obj, &s_float);
	return (x);
}

void gloat_setup(void) {
	gloat_class = class_new(gensym("gloat"),
		(t_newmethod)gloat_new, 0,
		sizeof(t_gloat), 0,
		A_GIMME, 0);
	
	class_addbang(gloat_class, gloat_bang);
	class_addfloat(gloat_class, gloat_float);
	class_sethelpsymbol(gloat_class, gensym("sploat.pd"));
}