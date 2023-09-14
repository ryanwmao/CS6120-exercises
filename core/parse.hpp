#pragma once

#include "types.hpp"
#include <nlohmann/json.hpp>

#include "unreachable.hpp"

#include <cassert>

namespace bril {

using json = nlohmann::json;

Type *type_from_json(const json &j);
void from_json(const json &j, Type *&t);
Const *const_from_json(const json &j);
Instr *instr_from_json(const json &j);
void from_json(const json &j, Instr *&i);
void from_json(const json &j, Func &fn);
void from_json(const json &j, Prog &p);

void to_json(json &j, Type *const &t);
void to_json(json &j, Arg const &a);
void to_json(json &j, Value *i);
void to_json(json &j, Label *i);
void to_json(json &j, Effect *i);
void to_json(json &j, Const *i);
void to_json(json &j, Instr *i);
void to_json(json &j, const Instr &i);
void to_json(json &j, const Func &fn);
void to_json(json &j, const Prog &p);

} // namespace bril
