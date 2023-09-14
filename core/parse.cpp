#include "parse.hpp"
#include <memory>

namespace bril {

Type *type_from_json(const json &j) {
  if (j.is_object())
    return new PtrType(type_from_json(j.at("ptr")));

  const auto str = j.template get<std::string>();
  if (str == "int")
    return new IntType();
  else if (str == "bool")
    return new BoolType();
  else if (str == "float")
    return new FloatType();
  else if (str == "char")
    return new CharType();
  assert(false);
}
void from_json(const json &j, Type *&t) { t = type_from_json(j); }
Const *const_from_json(const json &j) {
  std::string dest = j.at("dest").template get<std::string>();
  auto t = type_from_json(j.at("type"));
  switch (t->kind) {
  case TypeKind::Bool:
    return new Const(std::move(dest), t, j.at("value").template get<bool>());
  case TypeKind::Int:
    return new Const(std::move(dest), t, j.at("value").template get<int>());
  case TypeKind::Float:
    return new Const(std::move(dest), t, j.at("value").template get<float>());
  case TypeKind::Char:
    return new Const(std::move(dest), t, j.at("value").template get<char>());
  default:
    break;
  }
  assert(false);
}
Instr *instr_from_json(const json &j) {
  if (j.contains("label"))
    return new Label(j.at("label").template get<std::string>());
  std::string op = j.at("op").template get<std::string>();
  if (op == "const")
    return const_from_json(j);
  if (j.contains("dest")) {
    auto v = new Value(j.at("dest").template get<std::string>(),
                       type_from_json(j.at("type")), std::move(op));
    if (j.contains("args"))
      j.at("args").get_to(v->args);
    if (j.contains("funcs"))
      j.at("funcs").get_to(v->funcs);
    if (j.contains("labels"))
      j.at("labels").get_to(v->labels);
    return v;
  } else {
    auto v = new Effect(std::move(op));
    if (j.contains("args"))
      j.at("args").get_to(v->args);
    if (j.contains("funcs"))
      j.at("funcs").get_to(v->funcs);
    if (j.contains("labels"))
      j.at("labels").get_to(v->labels);
    return v;
  }
  assert(false);
  bril::unreachable();
}
void from_json(const json &j, Instr *&i) { i = instr_from_json(j); }
void from_json(const json &j, Func &fn) {
  j.at("name").get_to(fn.name);
  if (j.contains("type"))
    j.at("type").get_to(fn.ret_type);
  if (j.contains("args")) {
    for (const auto &arg : j.at("args")) {
      fn.args.emplace_back(arg.at("name").template get<std::string>(),
                           type_from_json(arg.at("type")));
    }
  }
  fn.bbs.push_back(new BasicBlock(0, "_start"));
  auto &bb = *fn.bbs.back();
  for (const auto &instr : j.at("instrs")) {
    bb.code.push_back(instr.template get<Instr *>());
  }
}
void from_json(const json &j, Prog &p) { j.at("functions").get_to(p.fns); }
void to_json(json &j, Type *const &t) {
  switch (t->kind) {
  case TypeKind::Int:
    j = "int";
    break;
  case TypeKind::Bool:
    j = "bool";
    break;
  case TypeKind::Float:
    j = "float";
    break;
  case TypeKind::Char:
    j = "char";
    break;
  case TypeKind::Ptr:
    j = json{{"ptr", static_cast<const PtrType *>(t)->type}};
    break;
  }
}
void to_json(json &j, Arg const &a) {
  j = json{{"name", a.name}, {"type", a.type}};
}
void to_json(json &j, Value *i) {
  j = json{{"dest", i->dest}, {"op", i->op},       {"type", i->type},
           {"args", i->args}, {"funcs", i->funcs}, {"labels", i->labels}};
  if (!i->args.empty())
    j.emplace("args", i->args);
  // j["args"] = i->args;
  if (!i->funcs.empty())
    j["funcs"] = i->funcs;
  if (!i->labels.empty())
    j["labels"] = i->labels;
}
void to_json(json &j, Label *i) { j = json{{"label", i->name}}; }
void to_json(json &j, Effect *i) {
  j = json{{"op", i->op}};
  if (!i->args.empty())
    j.emplace("args", i->args);
  //   j["args"] = i->args;
  if (!i->funcs.empty())
    j["funcs"] = i->funcs;
  if (!i->labels.empty())
    j["labels"] = i->labels;
}
void to_json(json &j, Const *i) {
  j = json{{"dest", i->dest}, {"op", "const"}, {"type", i->type}};
  switch (i->type->kind) {
  case TypeKind::Int:
    j["value"] = i->value.int_val;
    break;
  case TypeKind::Bool:
    j["value"] = i->value.bool_val;
    break;
  case TypeKind::Float:
    j["value"] = i->value.fp_val;
    break;
  case TypeKind::Char:
    j["value"] = i->value.char_val;
    break;
  default:
    assert(false);
    bril::unreachable();
  }
}
void to_json(json &j, Instr *i) {
  switch (i->kind) {
  case InstrKind::Label:
    to_json(j, static_cast<Label *const>(i));
    break;
  case InstrKind::Const:
    to_json(j, static_cast<Const *const>(i));
    break;
  case InstrKind::Value:
    to_json(j, static_cast<Value *const>(i));
    break;
  case InstrKind::Effect:
    to_json(j, static_cast<Effect *const>(i));
    break;
  }
}
void to_json(json &j, const Func &fn) {
  j = json{{"name", fn.name}, {"args", fn.args}, {"instrs", fn.allInstrs()}};
  if (fn.ret_type)
    j["type"] = fn.ret_type;
}
void to_json(json &j, const Prog &p) { j = json{{"functions", p.fns}}; }

} // namespace bril