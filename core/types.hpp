#pragma once

#include "casting.hpp"
#include "string_pool.hpp"

#include <boost/intrusive/list.hpp>
#include <list>
#include <string>
#include <vector>

namespace bril {

// TYPES

enum class TypeKind : char { Int, Bool, Float, Char, Ptr };

struct Type {
  const TypeKind kind;
  virtual ~Type() {}

protected:
  Type(const TypeKind kind_) : kind(kind_) {}
};

struct IntType : Type {
  IntType() : Type(TypeKind::Int) {}
  static bool classof(const Type *t) { return t->kind == TypeKind::Int; }
};

struct BoolType : Type {
  BoolType() : Type(TypeKind::Bool) {}
  static bool classof(const Type *t) { return t->kind == TypeKind::Bool; }
};

struct FloatType : Type {
  FloatType() : Type(TypeKind::Float) {}
  static bool classof(const Type *t) { return t->kind == TypeKind::Float; }
};

struct CharType : Type {
  CharType() : Type(TypeKind::Char) {}
  static bool classof(const Type *t) { return t->kind == TypeKind::Char; }
};

struct PtrType : Type {
  PtrType(Type *type_) : Type(TypeKind::Ptr), type(type_) {}
  Type *type;

  static bool classof(const Type *t) { return t->kind == TypeKind::Ptr; }
};

// INSTRUCTION

enum class InstrKind : char { Label, Const, Value, Effect };

struct Instr : public boost::intrusive::list_base_hook<> {
  const InstrKind kind;

  const std::vector<VarRef> *uses() const;
  std::vector<VarRef> *uses();

  const VarRef *def() const;
  VarRef *def();

protected:
  Instr(const InstrKind kind_) : kind(kind_) {}
};

struct Label : Instr {
  std::string name;

  Label(std::string &&name_)
      : Instr(InstrKind::Label), name(std::move(name_)) {}

  static bool classof(const Instr *t) { return t->kind == InstrKind::Label; }
};

struct Const : Instr {
  VarRef dest;
  Type *type;
  // based off of type
  union Literal {
    int int_val;
    bool bool_val;
    double fp_val;
    char char_val;

    Literal(int val) : int_val(val) {}
    Literal(bool val) : bool_val(val) {}
    Literal(double val) : fp_val(val) {}
    Literal(char val) : char_val(val) {}
  } value;

#define CONST_CONS(cpp_type)                                                   \
  Const(VarRef dest_, Type *type_, cpp_type val_)                              \
      : Instr(InstrKind::Const), dest(dest_), type(type_), value(val_) {}

  CONST_CONS(int)
  CONST_CONS(float)
  CONST_CONS(double)
  CONST_CONS(char)
#undef CONST_CONS

  static bool classof(const Instr *t) { return t->kind == InstrKind::Const; }
};

struct Value : Instr {
  VarRef dest;
  Type *type;
  std::string op;
  std::vector<VarRef> args;
  std::vector<std::string> labels;
  std::vector<std::string> funcs;

  Value(VarRef dest_, Type *type_, std::string &&op_)
      : Instr(InstrKind::Value), dest(dest_), type(type_), op(std::move(op_)) {}

  static bool classof(const Instr *t) { return t->kind == InstrKind::Value; }
};

struct Effect : Instr {
  std::string op;
  std::vector<VarRef> args;
  std::vector<std::string> labels;
  std::vector<std::string> funcs;

  Effect(std::string &&op_) : Instr(InstrKind::Effect), op(std::move(op_)) {}

  static bool classof(const Instr *t) { return t->kind == InstrKind::Effect; }
};

using InstrList = boost::intrusive::list<Instr>;

// BASIC BLOCKS

struct BasicBlock : public boost::intrusive::list_base_hook<> {
  // serial number of basic block in the function
  int id;
  std::string name;
  // predecessors of this basic block in the cfg
  std::vector<BasicBlock *> entries;
  // successors of this basic block in the cfg
  BasicBlock *exits[2] = {nullptr, nullptr};

  InstrList code;

  BasicBlock(std::string &&name_) : id(-1), name(std::move(name_)) {}
  BasicBlock(int id_, std::string &&name_) : id(id_), name(std::move(name_)) {}
};

using BBList = boost::intrusive::list<BasicBlock>;

// FUNCTION AND PROGRAM

struct Arg {
  VarRef name;
  Type *type;

  Arg(VarRef name_, Type *type_) : name(name_), type(type_) {}
};

struct Func {
  std::string name;
  Type *ret_type;
  std::vector<Arg> args;
  BBList bbs;

  StringPool *sp;
  VarPool vp;

  std::vector<const Instr *> allInstrs() const;

  Func() : sp(new StringPool()), vp(*sp) {}
};

struct Prog {
  std::vector<Func> fns;
};

} // namespace bril

namespace bril {
inline const std::vector<VarRef> *Instr::uses() const {
  switch (kind) {
  case bril::InstrKind::Const:
  case bril::InstrKind::Label:
    return nullptr;
  case bril::InstrKind::Effect:
    return &cast<Effect>(this)->args;
  case bril::InstrKind::Value:
    return &cast<Value>(this)->args;
  }
}
inline std::vector<VarRef> *Instr::uses() {
  return const_cast<std::vector<VarRef> *>(
      (const_cast<const Instr *>(this)->uses()));
}

inline const VarRef *Instr::def() const {
  switch (kind) {
  case bril::InstrKind::Const:
    return &cast<Const>(this)->dest;
  case bril::InstrKind::Value:
    return &cast<Value>(this)->dest;
  case bril::InstrKind::Label:
  case bril::InstrKind::Effect:
    return nullptr;
  }
}
inline VarRef *Instr::def() {
  return const_cast<VarRef *>((const_cast<const Instr *>(this)->def()));
}
} // namespace bril
