/* 
 * Clipper Ruby Bindings
 * Copyright 2010 Mike Owens <http://mike.filespanker.com/>
 *
 * Released under the same terms as Clipper.
 *
 */

#include <ruby.h>
#include <clipper.hpp>

using namespace std;
using namespace clipper;

extern "C" {

#define XCLIPPER(x)      ((Clipper*) DATA_PTR(x))

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
rbclipper_add_polygon(VALUE self, VALUE polygon, VALUE polytype)
{
  TPolygon tmp;
  ary_to_polygon(polygon, &tmp);
  XCLIPPER(self)->AddPolygon(tmp, (TPolyType) NUM2INT(polytype));
  return Qnil;
}

static VALUE
rbclipper_add_poly_polygon(VALUE self, VALUE polypoly, VALUE polytype)
{
  TPolyPolygon tmp;
  ary_to_polypolygon(polypoly, &tmp);
  XCLIPPER(self)->AddPolyPolygon(tmp, (TPolyType) NUM2INT(polytype));
  return Qnil;
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
    subjfill = INT2FIX(pftEvenOdd);
  if (NIL_P(clipfill))
    clipfill = INT2FIX(pftEvenOdd);

  TPolyPolygon solution;
  XCLIPPER(self)->Execute((TClipType) cliptype,
                          solution,
                          (TPolyFillType) NUM2INT(subjfill),
                          (TPolyFillType) NUM2INT(clipfill));
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
    subjfill = INT2FIX(pftNonZero);
    clipfill = INT2FIX(pftNonZero);
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
#define M(x) ((ruby_method)(x))

void Init_clipper() {
  VALUE mod   = rb_define_module("Clipper");

  VALUE polytype = rb_define_module_under(mod, "PolyType");
  rb_define_const(polytype, "SUBJECT", INT2FIX(ptSubject));
  rb_define_const(polytype, "CLIP",   INT2FIX(ptClip));

  VALUE polyfilltype = rb_define_module_under(mod, "PolyFillType");
  rb_define_const(polyfilltype, "EVEN_ODD", INT2FIX(pftEvenOdd));
  rb_define_const(polyfilltype, "NON_ZERO", INT2FIX(pftNonZero));

  VALUE k = rb_define_class_under(mod, "Clipper", rb_cObject);
  rb_define_singleton_method(k, "new", M(rbclipper_new), 0);
  rb_define_method(k, "add_polygon", M(rbclipper_add_polygon), 2);
  rb_define_method(k, "add_poly_polygon", M(rbclipper_add_poly_polygon), 2);
  rb_define_method(k, "clear!", M(rbclipper_clear), 0);
  rb_define_method(k, "force_orientation", M(rbclipper_force_orientation), 0);
  rb_define_method(k, "force_orientation=",
                      M(rbclipper_force_orientation_eq), 1);

  rb_define_method(k, "intersection", M(rbclipper_intersection), -1);
  rb_define_method(k, "union", M(rbclipper_union), -1);
  rb_define_method(k, "difference", M(rbclipper_difference), -1);
  rb_define_method(k, "xor", M(rbclipper_xor), -1);
}
#undef M

} /* extern 'C' */
