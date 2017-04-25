#include "m_pd.h"
#include <time.h>

/* -------------------------- rand -------------------------- */

static t_class *rand_class;

typedef struct _rand {
	t_object x_obj;
	t_float x_min, x_max, *x_vec;
	unsigned x_state;
	int x_ac;
} t_rand;

static int rand_time(void) {
	int thym = time(0) % 31536000; // seconds in a year
	return (thym|1); // odd numbers only
}

static int rand_makeseed(void) {
	static unsigned rand_next = 1489853723;
	rand_next = rand_next * rand_time() + 938284287;
	return (rand_next & 0x7fffffff);
}

static void rand_seed(t_rand *x, t_symbol *s, int argc, t_atom *argv) {
	x->x_state = (argc ? atom_getfloat(argv) : rand_time());
}

static void rand_peek(t_rand *x, t_symbol *s) {
	post("%s%s%u", s->s_name, *s->s_name?": ":"", x->x_state);
}

static void rand_min(t_rand *x, t_float f) {
	x->x_min=f;
}

static void rand_max(t_rand *x, t_float f) {
	x->x_max=f;
}

static void rand_bang(t_rand *x) {
	int c=x->x_ac, nval;
	unsigned state = x->x_state;
	x->x_state = state = state * 472940017 + 832416023;

	if (c<3)
	{	int min=x->x_min, n=x->x_max-min, b=n<0,
			range = (c!=1 ? n+(b?-1:1) : (n?n:1));
		double val = (1./4294967296) * range * state + min+b;
		nval = val-(val<0);
		outlet_float(x->x_obj.ob_outlet, nval);   }
	else
	{	nval = (1./4294967296) * c * state;
		outlet_float(x->x_obj.ob_outlet, x->x_vec[nval]);   }
}

static void *rand_new(t_symbol *s, int argc, t_atom *argv) {
	t_rand *x = (t_rand *)pd_new(rand_class);
	outlet_new(&x->x_obj, &s_float);
	x->x_state = rand_makeseed();
	x->x_ac=argc;
	if (argc<3)
	{	floatinlet_new(&x->x_obj, &x->x_max);
		floatinlet_new(&x->x_obj, &x->x_min);
		t_float max=1, min=0;
		switch (argc)
		{ case 2: min=atom_getfloat(argv+1);
		  case 1: max=atom_getfloat(argv); }
		x->x_max=max, x->x_min=min;   }
	else
	{	x->x_vec = (t_float *)getbytes(argc * sizeof(t_float));
		int i; t_float *fp;
		for (i=argc, fp=x->x_vec; i--; argv++, fp++)
		{	*fp = atom_getfloat(argv);
			floatinlet_new(&x->x_obj, fp);   }   }
	return (x);
}

static void rand_free(t_rand *x) {
	if (x->x_ac>2) freebytes(x->x_vec, x->x_ac * sizeof(t_float));
}

void rand_setup(void) {
	rand_class = class_new(gensym("rand"),
		(t_newmethod)rand_new, (t_method)rand_free,
		sizeof(t_rand), 0,
		A_GIMME, 0);
	
	class_addbang(rand_class, rand_bang);
	class_addmethod(rand_class, (t_method)rand_seed,
		gensym("seed"), A_GIMME, 0);
	class_addmethod(rand_class, (t_method)rand_peek,
		gensym("peek"), A_DEFSYM, 0);
	class_addmethod(rand_class, (t_method)rand_min,
		gensym("min"), A_FLOAT, 0);
	class_addmethod(rand_class, (t_method)rand_max,
		gensym("max"), A_FLOAT, 0);
}
