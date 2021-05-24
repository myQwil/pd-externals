#include "flin.h"
#include "note.h"
#include <stdlib.h> // strtof
#include <string.h> // memcpy

static inline int isoperator(char c) {
	return (c=='^' || c=='v' || c=='+' || c=='-' || c=='*' || c=='/');
}

static inline int gcd(int a ,int b) {
	int r;
	while (b)
	{	r = a % b;
		a = b;
		b = r;   }
	return a;
}

typedef struct {
	t_flin flin;
	t_note note;
	t_float oct;       /* # of semitone steps per octave */
	uint16_t siz;      /* current scale size */
	unsigned strict:1; /* strict scale size toggle */
} t_music;

static inline t_float music_step(t_music *x ,t_float *fp ,int d) {
	int n = x->siz;
	int i = d % n;
	int neg = (i < 0);
	i += n * neg;
	return (i ? fp[i] : 0) + x->oct * (d/n - neg);
}

static inline t_float music_interval(t_music *x ,t_float *fp ,t_float f) {
	int d = f;
	t_float step = music_step(x ,fp ,d);
	if (f != d) // between two intervals
	{	int dir = f<0 ? -1 : 1;
		t_float next = music_step(x ,fp ,d+dir);
		step += dir * (f-d) * (next-step);   }
	return step;
}

static inline t_float music_getfloat(t_music *x ,t_float *temp ,const char *cp) {
	int ref = (*cp=='&');
	t_float f = cp[ref] ? strtof(cp+ref ,0) : 1;
	if (ref) f = music_interval(x ,temp ,f);
	return f;
}

static inline void music_operate(t_float *fp ,char c ,t_float f) {
	switch (c)
	{	case '+':
		case '^': *fp += f; break;

		case '-':
		case 'v': *fp -= f; break;

		case '*': *fp *= f; break;

		case '/': *fp /= f; break;   }
}

static void music_ptr(t_music *x ,t_symbol *s) {
	post("%s%s%d" ,s->s_name ,*s->s_name?": ":"" ,x->flin.siz);
}

static void music_peek(t_music *x ,t_symbol *s) {
	t_float *fp = x->flin.fp;
	if (*s->s_name) startpost("%s: " ,s->s_name);
	for (int i=x->siz; i--; fp++) startpost("%g " ,*fp);
	endpost();
}

static void music_invert(t_music *x ,t_float *fp ,int mvrt ,int n ,int d) {
	int neg = d<0 ,octs = d/n;
	d %= n;
	if (!d)
	{	fp[0] += mvrt * x->oct * octs;
		return;   }
	d += n * neg;
	float shift = fp[d];
	fp[d] += fp[0] + mvrt * (shift + x->oct * (octs - neg));
	fp[0] = 0;

	int g = gcd(d ,n) ,r = n-d ,i;
	for (i = 0; i < g; i++)
	{	int temp = fp[i];
		int j = i;
		for (;;)
		{	int k = j + d;
			if (k >= n) k -= n;
			if (k == i) break;
			fp[j] = fp[k] - shift + x->oct * (j >= r);
			j = k;   }
		fp[j] = temp - shift + x->oct * (j >= r);   }
}

static int music_scale(t_music *x ,int i ,int ac ,t_atom *av) {
	if (flin_resize(&x->flin ,i+ac) < 0) // resize error
		return x->siz;

	t_float temp[x->siz]; // in case there are '&' operations
	memcpy(temp ,x->flin.fp ,x->siz * sizeof(t_float));

	int n = i;
	t_float *fp = x->flin.fp + i;
	for (; ac--; av++)
	{	if (av->a_type == A_FLOAT)
		{	*fp = av->a_w.w_float;
			n++ ,fp++;   }
		else if (av->a_type == A_SYMBOL)
		{	const char *cp = av->a_w.w_symbol->s_name;
			char *p;
			int m = x->siz - n;
			if (m <= 0) m = 1;
			i = strtol(cp ,&p ,10);
			if (cp != p) // arg count was manually set
			{	cp = p;
				if (i < 0)
					i = i % m + m;
				int res = flin_resize(&x->flin ,i+ac+n);
				if      (res > 0) fp = x->flin.fp + n;
				else if (res < 0) break;   }
			else i = m;

			char c = cp[0];
			if (c=='&')
				*fp = music_interval(x ,temp ,(cp[1] ? strtof(cp+1 ,0) : 1));
			else if (isoperator(c))
			{	if (cp[0] == cp[1]) // ++, --, etc.
				{	// do the same shift for all intervals that follow
					t_float f = music_getfloat(x ,temp ,cp+2);
					for (; i--; n++ ,fp++)
						music_operate(fp ,c ,f);
					continue;   }
				else music_operate(fp ,c ,music_getfloat(x ,temp ,cp+1));   }
			else if (c=='<' || c=='>')
			{	int mvrt = cp[0] == cp[1]; // << or >> - move the root
				cp += 1 + mvrt;
				music_invert(x ,fp ,mvrt ,i ,(c-'=') * (*cp ? atoi(cp) : 1));
				n+=i ,fp+=i;
				continue;   }
			else if (isoperator(cp[1])) // current value ± semitones
				music_operate(fp ,cp[1] ,music_getfloat(x ,temp ,cp+2));

			n++ ,fp++;   }   }
	return n;
}

static void music_i(t_music *x ,int i ,int ac ,t_atom *av) {
	x->siz = music_scale(x ,i ,ac ,av);
}

static void music_x(t_music *x ,int i ,int ac ,t_atom *av) {
	music_scale(x ,i ,ac ,av);
}

static void music_z(t_music *x ,int i ,int ac ,t_atom *av) {
	if (x->strict)
	     music_x(x ,i ,ac ,av);
	else music_i(x ,i ,ac ,av);
}

static void music_list(t_music *x ,t_symbol *s ,int ac ,t_atom *av) {
	music_z(x ,0 ,ac ,av);
}

static void music_f(t_music *x ,t_float f ,char c ,t_float g);

static void music_float(t_music *x ,t_float f) {
	music_f(x ,f ,0 ,0);
}

static void music_anything(t_music *x ,t_symbol *s ,int ac ,t_atom *av) {
	const char *cp = s->s_name;
	if (!ac)
	{	char *p;
		t_float f = strtof(cp ,&p);
		if (cp!=p && p[0]!=p[1] && isoperator(*p)) // interval ± semitones
			music_f(x ,f ,*p ,strtof(p+1 ,0));
		else
		{	t_atom atom = {A_SYMBOL ,{.w_symbol = s}};
			music_z(x ,0 ,1 ,&atom);   }   }
	else if (*cp == '!') music_i(x ,strtol(cp+1 ,0 ,10) ,ac ,av); // lazy
	else if (*cp == '@') music_z(x ,strtol(cp+1 ,0 ,10) ,ac ,av); // default
	else if (*cp == '#') music_x(x ,strtol(cp+1 ,0 ,10) ,ac ,av); // strict
	else
	{	t_atom atoms[ac+1];
		atoms[0] = (t_atom){A_SYMBOL ,{.w_symbol = s}};
		memcpy(atoms+1 ,av ,ac * sizeof(t_atom));
		music_z(x ,0 ,ac+1 ,atoms);   }
}

static void music_symbol(t_music *x ,t_symbol *s) {
	music_anything(x ,s ,0 ,0);
}

static void music_size(t_music *x ,t_floatarg n) {
	if (flin_resize(&x->flin ,n) < 0)
		return;
	x->siz = n;
}

static void music_strict(t_music *x ,t_floatarg f) {
	x->strict = f;
}

static void music_octave(t_music *x ,t_floatarg f) {
	x->oct = f;
}

static void music_ref(t_music *x ,t_floatarg f) {
	note_ref(&x->note ,f);
}

static void music_tet(t_music *x ,t_floatarg f) {
	note_tet(&x->note ,f);
}

static void music_octet(t_music *x ,t_floatarg f) {
	music_octave(x ,f);
	music_tet(x ,f);
}

static void music_set(t_music *x ,t_symbol *s ,int ac ,t_atom *av) {
	note_set(&x->note ,ac ,av);
}

static t_music *music_new(t_class *mclass ,int ac) {
	t_music *x = (t_music *)pd_new(mclass);

	x->siz = x->flin.ins = ac;
	flin_alloc(&x->flin ,ac);

	t_atom atms[] = { {A_FLOAT ,{440}} ,{A_FLOAT ,{12}} };
	note_set(&x->note ,2 ,atms);
	x->oct = x->note.tet;
	x->strict = 0;

	return x;
}

static t_class *music_setup
(t_symbol *s ,t_newmethod newm ,t_method free ,size_t size) {
	t_class *mclass = class_new(s ,newm ,free ,size ,0 ,A_GIMME ,0);
	class_addfloat   (mclass ,music_float);
	class_addsymbol  (mclass ,music_symbol);
	class_addlist    (mclass ,music_list);
	class_addanything(mclass ,music_anything);

	class_addmethod(mclass ,(t_method)music_ptr
		,gensym("ptr")  ,A_DEFSYM ,0);
	class_addmethod(mclass ,(t_method)music_peek
		,gensym("peek") ,A_DEFSYM ,0);
	class_addmethod(mclass ,(t_method)music_set
		,gensym("set")  ,A_GIMME  ,0);
	class_addmethod(mclass ,(t_method)music_size
		,gensym("n")    ,A_FLOAT  ,0);
	class_addmethod(mclass ,(t_method)music_strict
		,gensym("strict") ,A_FLOAT  ,0);
	class_addmethod(mclass ,(t_method)music_octave
		,gensym("oct")  ,A_FLOAT  ,0);
	class_addmethod(mclass ,(t_method)music_ref
		,gensym("ref")  ,A_FLOAT  ,0);
	class_addmethod(mclass ,(t_method)music_tet
		,gensym("tet")  ,A_FLOAT  ,0);
	class_addmethod(mclass ,(t_method)music_octet
		,gensym("ot")   ,A_FLOAT  ,0);

	return mclass;
}
