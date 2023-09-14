#include "parse.hpp"
#include "core/cfg.hpp"
#include <iostream>
#include <memory>

namespace bril {

Func *from_fn = nullptr;
VarPool *from_vp = nullptr;

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
  auto dest = from_vp->varRefOf(j.at("dest").template get<std::string>());
  auto t = type_from_json(j.at("type"));
  switch (t->kind) {
  case TypeKind::Bool:
    return new Const(dest, t, j.at("value").template get<bool>());
  case TypeKind::Int:
    return new Const(dest, t, j.at("value").template get<int>());
  case TypeKind::Float:
    return new Const(dest, t, j.at("value").template get<float>());
  case TypeKind::Char:
    return new Const(dest, t, j.at("value").template get<char>());
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
    auto dest = from_vp->varRefOf(j.at("dest").template get<std::string>());
    auto v = new Value(dest, type_from_json(j.at("type")), std::move(op));
    if (j.contains("args")) {
      for (auto &a : j.at("args"))
        v->args.push_back(from_vp->varRefOf(a.template get<std::string>()));
    }
    if (j.contains("funcs"))
      j.at("funcs").get_to(v->funcs);
    if (j.contains("labels"))
      j.at("labels").get_to(v->labels);
    return v;
  } else {
    auto v = new Effect(std::move(op));
    if (j.contains("args")) {
      for (auto &a : j.at("args"))
        v->args.push_back(from_vp->varRefOf(a.template get<std::string>()));
    }
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
  from_fn = &fn;
  from_vp = &fn.vp;

  j.at("name").get_to(fn.name);
  if (j.contains("type"))
    j.at("type").get_to(fn.ret_type);
  else
    fn.ret_type = nullptr;
  if (j.contains("args")) {
    for (const auto &arg : j.at("args")) {
      auto arg_ref =
          from_vp->varRefOf(arg.at("name").template get<std::string>());
      fn.args.emplace_back(arg_ref, type_from_json(arg.at("type")));
    }
  }
  std::vector<Instr *> instrs;
  for (const auto &instr : j.at("instrs")) {
    instrs.push_back(instr.template get<Instr *>());
  }
  fn.bbs = toCFG(instrs);

  from_vp = nullptr;
  from_fn = nullptr;
}
void from_json(const json &j, Prog &p) { j.at("functions").get_to(p.fns); }

const Func *to_fn = nullptr;
const VarPool *to_vp = nullptr;

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
    j = json{{"ptr", cast<PtrType>(t)->type}};
    break;
  }
}
void to_json(json &j, Arg const &a) {
  j = json{{"name", to_vp->strOf(a.name)}, {"type", a.type}};
}
void to_json(json &j, const Value &i) {
  j = json{{"dest", to_vp->strOf(i.dest)}, {"op", i.op}, {"type", i.type}};
  if (!i.args.empty()) {
    std::vector<std::string_view> args;
    for (auto a : i.args)
      args.push_back(to_vp->strOf(a));
    j["args"] = std::move(args);
  }
  if (!i.funcs.empty())
    j["funcs"] = i.funcs;
  if (!i.labels.empty())
    j["labels"] = i.labels;
}
void to_json(json &j, const Label &i) { j = json{{"label", i.name}}; }
void to_json(json &j, const Effect &i) {
  j = json{{"op", i.op}};
  if (!i.args.empty()) {
    std::vector<std::string_view> args;
    for (auto a : i.args)
      args.push_back(to_vp->strOf(a));
    j["args"] = std::move(args);
  }
  if (!i.funcs.empty())
    j["funcs"] = i.funcs;
  if (!i.labels.empty())
    j["labels"] = i.labels;
}
void to_json(json &j, const Const &i) {
  j = json{{"dest", to_vp->strOf(i.dest)}, {"op", "const"}, {"type", i.type}};
  switch (i.type->kind) {
  case TypeKind::Int:
    j["value"] = i.value.int_val;
    break;
  case TypeKind::Bool:
    j["value"] = i.value.bool_val;
    break;
  case TypeKind::Float:
    j["value"] = i.value.fp_val;
    break;
  case TypeKind::Char:
    j["value"] = i.value.char_val;
    break;
  default:
    assert(false);
    bril::unreachable();
  }
}
void to_json(json &j, const Instr &i) {
  switch (i.kind) {
  case InstrKind::Label:
    to_json(j, cast<Label>(i));
    break;
  case InstrKind::Const:
    to_json(j, cast<Const>(i));
    break;
  case InstrKind::Value:
    to_json(j, cast<Value>(i));
    break;
  case InstrKind::Effect:
    to_json(j, cast<Effect>(i));
    break;
  }
}
void to_json(json &j, const Instr &i, const Func &fn) {
  to_fn = &fn;
  to_vp = &fn.vp;

  to_json(j, i);

  to_vp = nullptr;
  to_fn = nullptr;
}
json to_json(const Instr &i, const Func &fn) {
  json j;
  to_json(j, i, fn);
  return j;
}
void to_json(json &j, const Instr *i) { to_json(j, *i); }
void to_json(json &j, const Func &fn) {
  to_fn = &fn;
  to_vp = &fn.vp;

  j = json{{"name", fn.name}, {"args", fn.args}, {"instrs", fn.allInstrs()}};
  if (fn.ret_type)
    j["type"] = fn.ret_type;

  to_vp = nullptr;
  to_fn = nullptr;
}
void to_json(json &j, const Prog &p) { j = json{{"functions", p.fns}}; }

} // namespace bril