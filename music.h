#include "fin.h"
#include "note.h"
#include <stdlib.h> // strtof
#include <string.h> // memcpy
#include <ctype.h>

typedef struct _music {
	t_fin x_fin;
	t_note x_note;
	t_float x_oct;     /* # of semitone steps per octave */
	int x_n;           /* current scale size */
	unsigned x_expl:1; /* explicit scale size toggle */
} t_music;

static void music_ptr(t_music *x, t_symbol *s) {
	post("%s%s%d", s->s_name, *s->s_name?": ":"", fin.x_p);
}

static void music_peek(t_music *x, t_symbol *s) {
	t_float *fp = fin.x_fp;
	if (*s->s_name) startpost("%s: ", s->s_name);
	for (int i=x->x_n; i--; fp++) startpost("%g ", *fp);
	endpost();
}

static int gcd(int a, int b) {
	int r;
	while (b != 0)
	{	r = a % b;
		a = b;
		b = r;   }
	return a;
}

static void music_invert(t_music *x, t_float *fp, int mvrt, int n, int d) {
	int neg = d<0, octs = d/n;
	d %= n;
	if (!d)
	{	fp[0] += mvrt * x->x_oct * octs;
		return;   }
	d += n * neg;
	float shift = fp[d];
	fp[d] += fp[0] + mvrt * (shift + x->x_oct * (octs - neg));
	fp[0] = 0;

	int g = gcd(d, n), r = n-d, i;
	for (i = 0; i < g; i++)
	{	int temp = fp[i];
		int j = i;
		while (1)
		{	int k = j + d;
			if (k >= n) k -= n;
			if (k == i) break;
			fp[j] = fp[k] - shift + x->x_oct * (j >= r);
			j = k;   }
		fp[j] = temp - shift + x->x_oct * (j >= r);   }
}

static void music_operate(t_float *fp, t_float f, char c) {
	if      (c=='+' || c=='^') *fp += f;
	else if (c=='-' || c=='v') *fp -= f;
	else if (c=='*') *fp *= f;
	else if (c=='/') *fp /= f;
}

static void music_calc(t_music *x, t_float **fp, int *n, t_atom *av) {
	const char *cp = av->a_w.w_symbol->s_name;
	int i;
	if (isdigit(*cp) || (*cp == '-' && isdigit(cp[1])))
	{	i = atoi(cp);
		if (i < 0)
			i = i % *n + *n;
		while (isdigit(*++cp));   }
	else i = *n;

	char c = cp[0];
	if (c=='^' || c=='v' || c=='+' || c=='-' || c=='*' || c=='/')
	{	if (cp[0] == cp[1])
		{	for (;;)
			{	music_operate(*fp, (cp[2] ? strtof(cp+2, NULL) : 1), c);
				if (--i) *n -= 1, *fp += 1;
				else break;   }   }
		else music_operate(*fp, (cp[1] ? strtof(cp+1, NULL) : 1), c);   }
	else if (c=='<' || c=='>')
	{	int mvrt = cp[0] == cp[1];
		cp += mvrt;
		music_invert(x, *fp, mvrt, i,
			(cp[1] ? atoi(cp+1) : 1) * (c=='<' ? -1 : 1));
		i--, *n -= i, *fp += i;   }
	else if (cp[1])
		music_operate(*fp, (cp[2] ? strtof(cp+2, NULL) : 1), cp[1]);
}

static int music_scale(t_music *x, int ac, t_atom *av) {
	t_float *fp = fin.x_fp;
	int n = x->x_n;
	for (; ac > 0; n--, ac--, av++, fp++)
	{	if      (av->a_type == A_FLOAT)  *fp = av->a_w.w_float;
		else if (av->a_type == A_SYMBOL) music_calc(x, &fp, &n, av);   }
	return (n != ac);
}

static int music_more(t_music *x, int ac, t_atom *av) {
	int more = 0;
	for (; ac--; av++)
		if (av->a_type == A_SYMBOL)
		{	const char *cp = av->a_w.w_symbol->s_name;
			if (isdigit(*cp) || (*cp == '-' && isdigit(cp[1])))
			{	int add = atoi(cp);
				if      (add > 0) more += add - 1;
				else if (add < 0) more += add % x->x_n + x->x_n - 1;   }   }
	return more;
}

static void music_x(t_music *x, t_symbol *s, int ac, t_atom *av) {
	if (ac + music_more(x, ac, av) > fin.x_p)
		ac = fin.x_p;
	music_scale(x, ac, av);
}

static void music_i(t_music *x, t_symbol *s, int ac, t_atom *av) {
	int n = fin_resize(&fin, ac + music_more(x, ac, av), 0);
	if (music_scale(x, ac, av)) x->x_n = n;
}

static void music_list(t_music *x, t_symbol *s, int ac, t_atom *av) {
	if (x->x_expl)
		 music_x(x, s, ac, av);
	else music_i(x, s, ac, av);
}

static void music_anything(t_music *x, t_symbol *s, int ac, t_atom *av) {
	t_atom atoms[ac+1];
	atoms[0] = (t_atom){A_SYMBOL, {.w_symbol = s}};
	memcpy(atoms+1, av, ac * sizeof(t_atom));
	music_list(x, 0, ac+1, atoms);
}

static void music_at(t_music *x, t_symbol *s, int ac, t_atom *av) {
	if (ac==2 && av->a_type == A_FLOAT)
	{	int i = fin_resize(&fin, av->a_w.w_float, 1);
		t_atomtype typ = (av+1)->a_type;
		if      (typ == A_FLOAT)  fin.x_fp[i] = (av+1)->a_w.w_float;
		else if (typ == A_SYMBOL)
		{	int n = x->x_n-i;
			music_calc(x, &fin.x_fp+i, &n, av+1);   }   }
	else pd_error(x, "music_at: bad arguments");
}

static void music_size(t_music *x, t_floatarg n) {
	x->x_n = fin_resize(&fin, n, 0);
}

static void music_expl(t_music *x, t_floatarg f) {
	x->x_expl = f;
}

static void music_octave(t_music *x, t_floatarg f) {
	x->x_oct = f;
}

static void music_ref(t_music *x, t_floatarg f) {
	note_ref(&note, f, 0);
}

static void music_tet(t_music *x, t_floatarg f) {
	note_tet(&note, f, 0);
}

static void music_octet(t_music *x, t_floatarg f) {
	music_octave(x, f);
	music_tet(x, f);
}

static void music_set(t_music *x, t_symbol *s, int ac, t_atom *av) {
	note_set(&note, ac, av, 0);
}

static t_music *music_new(t_class *mclass, int ac) {
	t_music *x = (t_music *)pd_new(mclass);

	int n = x->x_n = fin.x_in = fin.x_p = ac;
	fin.x_fp = (t_float *)getbytes(fin.x_p * sizeof(t_float));

	t_atom atms[] = { {A_FLOAT, {440}}, {A_FLOAT, {12}} };
	note_set(&note, 2, atms, 0);
	x->x_oct = note.tet;
	x->x_expl = 0;

	return x;
}

static t_class *music_setup
(t_symbol *s, t_newmethod newm, t_method free, size_t size) {
	t_class *mclass = class_new(s, newm, free, size, 0, A_GIMME, 0);
	class_addlist(mclass, music_list);
	class_addanything(mclass, music_anything);
	class_addmethod(mclass, (t_method)music_ptr,
		gensym("ptr"), A_DEFSYM, 0);
	class_addmethod(mclass, (t_method)music_peek,
		gensym("peek"), A_DEFSYM, 0);
	class_addmethod(mclass, (t_method)music_set,
		gensym("set"), A_GIMME, 0);
	class_addmethod(mclass, (t_method)music_at,
		gensym("@"), A_GIMME, 0);
	class_addmethod(mclass, (t_method)music_x,
		gensym("x"), A_GIMME, 0);
	class_addmethod(mclass, (t_method)music_i,
		gensym("i"), A_GIMME, 0);
	class_addmethod(mclass, (t_method)music_size,
		gensym("n"), A_FLOAT, 0);
	class_addmethod(mclass, (t_method)music_expl,
		gensym("expl"), A_FLOAT, 0);
	class_addmethod(mclass, (t_method)music_octave,
		gensym("oct"), A_FLOAT, 0);
	class_addmethod(mclass, (t_method)music_ref,
		gensym("ref"), A_FLOAT, 0);
	class_addmethod(mclass, (t_method)music_tet,
		gensym("tet"), A_FLOAT, 0);
	class_addmethod(mclass, (t_method)music_octet,
		gensym("ot"), A_FLOAT, 0);

	return mclass;
}