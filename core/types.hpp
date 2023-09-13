#pragma once

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
};

struct BoolType : Type {
  BoolType() : Type(TypeKind::Bool) {}
};

struct FloatType : Type {
  FloatType() : Type(TypeKind::Float) {}
};

struct CharType : Type {
  CharType() : Type(TypeKind::Char) {}
};

struct PtrType : Type {
  PtrType(Type *type_) : Type(TypeKind::Ptr), type(type_) {}
  Type *type;
};

// INSTRUCTION

enum class InstrKind : char { Label, Const, Value, Effect };

struct Instr {
  const InstrKind kind;

protected:
  Instr(const InstrKind kind_) : kind(kind_) {}
};

struct Label : Instr {
  std::string name;

  Label(std::string &&name_)
      : Instr(InstrKind::Label), name(std::move(name_)) {}
};

struct Const : Instr {
  std::string dest;
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
  Const(std::string &&dest_, Type *type_, cpp_type val_)                       \
      : Instr(InstrKind::Const), dest(std::move(dest_)), type(type_),          \
        value(val_) {}

  CONST_CONS(int)
  CONST_CONS(float)
  CONST_CONS(double)
  CONST_CONS(char)
#undef CONST_CONS
};

struct Value : Instr {
  std::string dest;
  Type *type;
  std::string op;
  std::vector<std::string> args;
  std::vector<std::string> labels;
  std::vector<std::string> funcs;

  Value(std::string &&dest_, Type *type_, std::string &&op_)
      : Instr(InstrKind::Value), dest(std::move(dest_)), type(type_),
        op(std::move(op_)) {}
};

struct Effect : Instr {
  std::string op;
  std::vector<std::string> args;
  std::vector<std::string> labels;
  std::vector<std::string> funcs;

  Effect(std::string &&op_) : Instr(InstrKind::Effect), op(std::move(op_)) {}
};

// FUNCTION AND PROGRAM

struct Arg {
  std::string name;
  Type *type;

  Arg(std::string &&name_, Type *type_) : name(name_), type(type_) {}
};

struct Func {
  std::string name;
  Type *ret_type;
  std::vector<Arg> args;
  std::list<Instr *> instrs;
};

struct Prog {
  std::vector<Func> fns;
};

} // namespace bril
