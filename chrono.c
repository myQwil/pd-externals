#include "m_pd.h"

#define timesince clock_gettimesincewithunits
EXTERN void parsetimeunits(void *x ,t_float amount ,t_symbol *unitname
	,t_float *unit ,int *samps);

/* -------------------------- chrono ------------------------------ */
static t_class *chrono_class;

typedef struct {
	t_object obj;
	t_symbol *unitname;
	t_float  unit;
	int      samps;
	double   settime ,laptime;
	double   setmore ,lapmore; /* paused time and tempo changes */
	unsigned pause:1;
	t_outlet *o_lap;
} t_chrono;

static void chrono_bang(t_chrono *x) {
	x->settime = x->laptime = clock_getlogicaltime();
	x->setmore = x->lapmore = x->pause = 0;
}

static void chrono_push(t_chrono *x ,t_floatarg f) {
	x->setmore -= f;
}

static void chrono_float(t_chrono *x ,t_floatarg f) {
	chrono_bang(x);
	chrono_push(x ,f);
}

static void chrono_bang2(t_chrono *x) {
	outlet_float(x->obj.ob_outlet ,x->setmore + (x->pause ? 0 :
		timesince(x->settime ,x->unit ,x->samps) ));
}

static void chrono_lap(t_chrono *x) {
	double settime ,laptime;
	if (x->pause)
		settime = laptime = clock_getlogicaltime();
	else
	{	settime = x->settime;
		laptime = x->laptime;
		x->laptime = clock_getlogicaltime();   }
	t_atom lap[] =
	{	 { A_FLOAT ,{timesince(laptime ,x->unit ,x->samps) + x->lapmore} }
		,{ A_FLOAT ,{timesince(settime ,x->unit ,x->samps) + x->setmore} }   };
	x->lapmore = 0;
	outlet_list(x->o_lap ,0 ,2 ,lap);
}

static void chrono_pause(t_chrono *x) {
	x->pause = !x->pause;
	if (x->pause)
	{	x->setmore += timesince(x->settime ,x->unit ,x->samps);
		x->lapmore += timesince(x->laptime ,x->unit ,x->samps);   }
	else
		x->settime = x->laptime = clock_getlogicaltime();
}

static void chrono_tempo(t_chrono *x ,t_symbol *s ,int ac ,t_atom *av) {
	if (!x->pause)
	{	x->setmore += timesince(x->settime ,x->unit ,x->samps);
		x->lapmore += timesince(x->laptime ,x->unit ,x->samps);
		x->settime = x->laptime = clock_getlogicaltime();   }
	if (ac > 2) ac = 2;
	while (ac--)
	{	switch (av[ac].a_type)
		{	case A_FLOAT  :x->unit     = av[ac].a_w.w_float  ;break;
			case A_SYMBOL :x->unitname = av[ac].a_w.w_symbol ;break;
			default: break;   }   }
	parsetimeunits(x ,x->unit ,x->unitname ,&x->unit ,&x->samps);
}

static void *chrono_new(t_symbol *s ,int argc ,t_atom *argv) {
	t_chrono *x = (t_chrono *)pd_new(chrono_class);
	inlet_new(&x->obj ,&x->obj.ob_pd ,&s_bang ,gensym("bang2"));
	outlet_new(&x->obj ,&s_float);
	x->o_lap = outlet_new(&x->obj ,0);

	x->unit = 1 ,x->samps = 0;
	x->unitname = gensym("msec");
	chrono_bang(x);
	chrono_tempo(x ,0 ,argc ,argv);
	return (x);
}

void chrono_setup(void) {
	chrono_class = class_new(gensym("chrono")
		,(t_newmethod)chrono_new ,0
		,sizeof(t_chrono) ,0
		,A_GIMME ,0);
	class_addbang  (chrono_class ,chrono_bang);
	class_addfloat (chrono_class ,chrono_float);
	class_addmethod(chrono_class ,(t_method)chrono_bang2
		,gensym("bang2") ,0);
	class_addmethod(chrono_class ,(t_method)chrono_pause
		,gensym("pause") ,0);
	class_addmethod(chrono_class ,(t_method)chrono_lap
		,gensym("lap")   ,0);
	class_addmethod(chrono_class ,(t_method)chrono_push
		,gensym("push")  ,A_FLOAT ,0);
	class_addmethod(chrono_class ,(t_method)chrono_tempo
		,gensym("tempo") ,A_GIMME ,0);
}
