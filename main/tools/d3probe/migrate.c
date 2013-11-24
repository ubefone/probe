
// #include "d3probe.h"

// // from mruby-thread
// static mrb_value migrate_simple_value(mrb_state *mrb, mrb_value v, mrb_state *mrb2) {
//   mrb_value nv;
//   const char *s;
//   int len;

//   nv.tt = v.tt;
//   switch (mrb_type(v)) {
//   case MRB_TT_FALSE:
//   case MRB_TT_TRUE:
//   case MRB_TT_FIXNUM:
//     nv.value.i = v.value.i;
//     break;
//   case MRB_TT_SYMBOL:
//     nv = mrb_symbol_value(mrb_intern_str(mrb2, v));
//     break;
//   case MRB_TT_FLOAT:
//     nv.value.f = v.value.f;
//     break;
//   case MRB_TT_STRING:
//     {
//       struct RString *str = mrb_str_ptr(v);

//       s = str->ptr;
//       len = str->len;
//       nv = mrb_str_new(mrb2, s, len);
//     }
//     break;
//   case MRB_TT_ARRAY:
//     {
//       struct RArray *a0, *a1;
//       int i;

//       a0 = mrb_ary_ptr(v);
//       nv = mrb_ary_new_capa(mrb2, a0->len);
//       a1 = mrb_ary_ptr(nv);
//       for (i=0; i<a0->len; i++) {
//         int ai = mrb_gc_arena_save(mrb2);
//         a1->ptr[i] = migrate_simple_value(mrb, a0->ptr[i], mrb2);
//         a1->len++;
//         mrb_gc_arena_restore(mrb2, ai);
//       }
//     }
//     break;
//   case MRB_TT_HASH:
//     {
//       mrb_value ka;
//       int i, l;

//       nv = mrb_hash_new(mrb2);
//       ka = mrb_hash_keys(mrb, v);
//       l = RARRAY_LEN(ka);
//       for (i = 0; i < l; i++) {
//         int ai = mrb_gc_arena_save(mrb2);
//         mrb_value k = migrate_simple_value(mrb, mrb_ary_entry(ka, i), mrb2);
//         mrb_value o = migrate_simple_value(mrb, mrb_hash_get(mrb, v, k), mrb2);
//         mrb_hash_set(mrb2, nv, k, o);
//         mrb_gc_arena_restore(mrb2, ai);
//       }
//     }
//     break;
//   default:
//     mrb_raise(mrb, E_TYPE_ERROR, "cannot migrate object");
//     break;
//   }
//   return nv;
// }
