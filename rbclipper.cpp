/* 
 * Clipper Ruby Bindings
 * Copyright 2010 Mike Owens <http://mike.filespanker.com/>
 *
 * Released under the same terms as Clipper.
 *
 */

#include <clipper.hpp>
#include <ruby.h>

using namespace std;
using namespace clipper;

static ID id_even_odd;
static ID id_non_zero;

static inline Clipper*
XCLIPPER(VALUE x)
{
  Clipper* clipper;
  Data_Get_Struct(x, Clipper, clipper);
  return clipper;
}

static inline TPolyFillType
sym_to_filltype(VALUE sym)
{
  ID inp = rb_to_id(sym);

  if (inp == id_even_odd) {
    return pftEvenOdd;
  } else if (inp == id_non_zero) {
    return pftNonZero;
  }

  rb_raise(rb_eArgError, "%s", "Expected :even_odd or :non_zero");
}

extern "C" {

static void
ary_to_polygon(VALUE ary, TPolygon* poly)
{
  const char* earg =
    "Polygons have format: [[p0_x, p0_y], [p1_x, p1_y], ...]";

  Check_Type(ary, T_ARRAY);

  for(long i = 0; i != RARRAY_LEN(ary); i++) {
    VALUE sub = rb_ary_entry(ary, i);
    Check_Type(sub, T_ARRAY);

    if(RARRAY_LEN(sub) != 2) {
      rb_raise(rb_eArgError, "%s", earg);
    }

    VALUE px = rb_ary_entry(sub, 0);
    VALUE py = rb_ary_entry(sub, 1);
    poly->push_back(DoublePoint(NUM2DBL(px), NUM2DBL(py)));
  }
}

static void
ary_to_polypolygon(VALUE ary, TPolyPolygon* polypoly)
{
  Check_Type(ary, T_ARRAY);
  for(long i = 0; i != RARRAY_LEN(ary); i++) {
    TPolygon p;
    VALUE sub = rb_ary_entry(ary, i);
    Check_Type(sub, T_ARRAY);
    ary_to_polygon(sub, &p);
    polypoly->push_back(p);
  }
}

static void
rbclipper_free(void* ptr)
{
  delete (Clipper*) ptr;
}

static VALUE
rbclipper_new(VALUE klass)
{
  Clipper* ptr = new Clipper;
  VALUE r = Data_Wrap_Struct(klass, 0, rbclipper_free, ptr);
  rb_obj_call_init(r, 0, 0);
  return r;
}

static VALUE
rbclipper_add_polygon_internal(VALUE self, VALUE polygon,
                               TPolyType polytype)
{
  TPolygon tmp;
  ary_to_polygon(polygon, &tmp);
  XCLIPPER(self)->AddPolygon(tmp, polytype);
  return Qnil;
}

static VALUE
rbclipper_add_subject_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_polygon_internal(self, polygon, ptSubject);
}

static VALUE
rbclipper_add_clip_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_polygon_internal(self, polygon, ptClip);
}


static VALUE
rbclipper_add_poly_polygon_internal(VALUE self, VALUE polypoly,
                                    TPolyType polytype)
{
  TPolyPolygon tmp;
  ary_to_polypolygon(polypoly, &tmp);
  XCLIPPER(self)->AddPolyPolygon(tmp, polytype);
  return Qnil;
}

static VALUE
rbclipper_add_subject_poly_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_poly_polygon_internal(self, polygon, ptSubject);
}

static VALUE
rbclipper_add_clip_poly_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_poly_polygon_internal(self, polygon, ptClip);
}


static VALUE
rbclipper_clear(VALUE self)
{
  XCLIPPER(self)->Clear();
  return Qnil;
}

static VALUE
rbclipper_force_orientation(VALUE self)
{
  return XCLIPPER(self)->ForceOrientation() ? Qtrue : Qfalse;
}

static VALUE
rbclipper_force_orientation_eq(VALUE self, VALUE b)
{
  XCLIPPER(self)->ForceOrientation(b == Qtrue);
  return b;
}

static VALUE
rbclipper_execute_internal(VALUE self, TClipType cliptype,
                           VALUE subjfill, VALUE clipfill)
{
  if (NIL_P(subjfill))
    subjfill = ID2SYM(id_even_odd);

  if (NIL_P(clipfill))
    clipfill = ID2SYM(id_even_odd);

  TPolyPolygon solution;
  XCLIPPER(self)->Execute((TClipType) cliptype,
                          solution,
                          sym_to_filltype(subjfill),
                          sym_to_filltype(clipfill));
  VALUE r = rb_ary_new();
  for(TPolyPolygon::iterator i = solution.begin();
                             i != solution.end();
                             ++i) {
    VALUE sub = rb_ary_new();
    for(TPolygon::iterator p = i->begin(); p != i->end(); ++p) {
      rb_ary_push(sub, rb_ary_new3(2, DBL2NUM(p->X), DBL2NUM(p->Y)));
    }
    rb_ary_push(r, sub);
  }

  return r;
}

static VALUE
rbclipper_intersection(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctIntersection, subjfill, clipfill);
}

static VALUE
rbclipper_union(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;

  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);

  /* For union, we really wanna default to non-zero */
  if(NIL_P(subjfill) && NIL_P(clipfill)) {
    subjfill = ID2SYM(id_non_zero);
    clipfill = ID2SYM(id_non_zero);
  }

  return rbclipper_execute_internal(self, ctUnion, subjfill, clipfill);
}


static VALUE
rbclipper_difference(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctDifference, subjfill, clipfill);
}


static VALUE
rbclipper_xor(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctXor, subjfill, clipfill);
}


typedef VALUE (*ruby_method)(...);

void Init_clipper() {
  id_even_odd = rb_intern("even_odd");
  id_non_zero = rb_intern("non_zero");

  VALUE mod   = rb_define_module("Clipper");

  VALUE k = rb_define_class_under(mod, "Clipper", rb_cObject);
  rb_define_singleton_method(k, "new",
                             (ruby_method) rbclipper_new, 0);

  rb_define_method(k, "add_subject_polygon",
                   (ruby_method) rbclipper_add_subject_polygon, 1);
  rb_define_method(k, "add_clip_polygon",
                   (ruby_method) rbclipper_add_clip_polygon, 1);

  rb_define_method(k, "add_subject_poly_polygon",
                   (ruby_method) rbclipper_add_subject_poly_polygon, 1);
  rb_define_method(k, "add_clip_poly_polygon",
                   (ruby_method) rbclipper_add_clip_poly_polygon, 1);


  rb_define_method(k, "clear!",
                   (ruby_method) rbclipper_clear, 0);
  rb_define_method(k, "force_orientation",
                   (ruby_method) rbclipper_force_orientation, 0);
  rb_define_method(k, "force_orientation=",
                   (ruby_method) rbclipper_force_orientation_eq, 1);

  rb_define_method(k, "intersection",
                   (ruby_method) rbclipper_intersection, -1);
  rb_define_method(k, "union",
                   (ruby_method) rbclipper_union, -1);
  rb_define_method(k, "difference",
                   (ruby_method) rbclipper_difference, -1);
  rb_define_method(k, "xor",
                   (ruby_method) rbclipper_xor, -1);
}

} /* extern "C" */
