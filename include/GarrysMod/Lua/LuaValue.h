#ifndef _GLOO_LUA_VALUE_H_
#define _GLOO_LUA_VALUE_H_

#include <map>
#include <string>
#include <memory>
#include <cassert>
#include <variant>
#include <algorithm>
#include "GarrysMod/Lua/Interface.h"

namespace GarrysMod {
namespace Lua {
  class LuaValue
  {
  public:
    typedef std::map<LuaValue, LuaValue> table_t;
    typedef double                       number_t;
    typedef std::string                  string_t;
    typedef bool                         boolean_t;
    typedef CFunc                        function_t;
    typedef void*                        userdata_t;
    typedef std::variant<
      table_t,
      number_t,
      string_t,
      boolean_t,
      function_t,
      userdata_t
    > value_t;
  private:
    int     _type;
    value_t _value;
  public:
    int type() const { return _type; }
  public:
    LuaValue() { _type = Type::NIL; }
    LuaValue(table_t value) { _type = Type::TABLE; _value = value; }
    LuaValue(number_t value) { _type = Type::NUMBER; _value = value; }
    LuaValue(string_t value) { _type = Type::STRING; _value = value; }
    LuaValue(boolean_t value) { _type = Type::BOOL; _value = value; }
    LuaValue(function_t value) { _type = Type::FUNCTION; _value = value; }
    LuaValue(userdata_t value) { _type = Type::USERDATA; _value = value; }
    LuaValue(int type, userdata_t value) { _type = type; _value = value; }
    LuaValue(const LuaValue &value) { Copy(value); }
  public:
    void Copy(const LuaValue &that)
    {
      _type = that._type;
      _value = that._value;
    }

    /**
     * Push table value to lua stack
     * @param state - Lua state
     */
    void PushTable(lua_State *state) const
    {
      if (_type != Type::TABLE)
        throw new std::runtime_error("Unable to push type '" + std::string(LUA->GetTypeName(_type)) + "' as table");

      LUA->CreateTable();
      int table_ref = LUA->ReferenceCreate();
      LUA->ReferencePush(table_ref);

      for (const auto &pair : std::get<table_t>(_value))
      {
        auto key = pair.first;
        auto value = pair.second;

        if (key == *this)
          LUA->ReferencePush(table_ref);
        else
          key.Push(state);

        if (value == *this)
          LUA->ReferencePush(table_ref);
        else
          value.Push(state);

        LUA->SetTable(-3);
      }

      LUA->ReferenceFree(table_ref);
    }

    /**
     * Push value to lua stack 
     * @param state - Lua state
     */
    void Push(lua_State *state) const
    {
      switch (_type)
      {
        case Type::NIL: LUA->PushNil(); break;
        case Type::TABLE: PushTable(state); break;
        case Type::NUMBER: LUA->PushNumber(std::get<number_t>(_value)); break;
        case Type::STRING: LUA->PushString(std::get<string_t>(_value).c_str()); break;
        case Type::BOOL: LUA->PushBool(std::get<boolean_t>(_value)); break;
        case Type::FUNCTION: LUA->PushCFunction(std::get<function_t>(_value)); break;
        default:
          LUA->PushUserdata(std::get<userdata_t>(_value));
          break;
      }
    }
  private:
    void assert_type(int type) const
    {
      if (_type != type)
        throw std::runtime_error("Expected type '" + std::string("") + "' not type '" + std::string("") + "'");
    }
  public:
    inline LuaValue& operator= (const LuaValue& rhs)
    {
      Copy(rhs);
      return *this;
    }

    inline bool operator< (const LuaValue& rhs) const
    {
      if (_type != rhs._type) return _type < rhs._type;

      switch (_type)
      {
        case Type::NIL: return false;
        case Type::BOOL: return std::get<boolean_t>(_value) < std::get<boolean_t>(rhs._value);
        case Type::NUMBER: return std::get<number_t>(_value) < std::get<number_t>(rhs._value);
        case Type::STRING: return std::get<string_t>(_value) < std::get<string_t>(rhs._value);
        case Type::FUNCTION: return std::get<function_t>(_value) < std::get<function_t>(rhs._value);
        default: return std::get<userdata_t>(_value) < std::get<userdata_t>(rhs._value);
      }
    }
    inline bool operator> (const LuaValue& rhs) const { return rhs < *this; }
    inline bool operator<=(const LuaValue& rhs) const { return !(*this < rhs); }
    inline bool operator>=(const LuaValue& rhs) const { return !(rhs < *this); }
    inline bool operator==(const LuaValue& rhs) const
    {
      if (_type != rhs._type)
        return false;

      switch (_type)
      {
        case Type::NIL: return true;
        case Type::BOOL: return std::get<boolean_t>(_value) == std::get<boolean_t>(rhs._value);
        case Type::TABLE: return std::get<table_t>(_value) == std::get<table_t>(rhs._value);
        case Type::NUMBER: return std::get<number_t>(_value) == std::get<number_t>(rhs._value);
        case Type::STRING: return std::get<string_t>(_value) == std::get<string_t>(rhs._value);
        case Type::FUNCTION: return std::get<function_t>(_value) == std::get<function_t>(rhs._value);
        default: return std::get<userdata_t>(_value) == std::get<userdata_t>(rhs._value);
      }
    }
    inline bool operator!=(const LuaValue& rhs) const { return !(*this == rhs); }
    inline LuaValue& operator[](LuaValue idx)
    {
      assert_type(Type::TABLE);
      return std::get<table_t>(_value)[idx];
    }

    template<typename T>
    inline LuaValue& operator= (T rhs) { return *this = LuaValue(rhs); }
    template<typename T>
    inline bool operator< (T rhs) const { return *this < LuaValue(rhs); }
    template<typename T>
    inline bool operator> (T rhs) const { return *this > LuaValue(rhs); }
    template<typename T>
    inline bool operator<=(T rhs) const { return *this <= LuaValue(rhs); }
    template<typename T>
    inline bool operator>=(T rhs) const { return *this >= LuaValue(rhs); }
    template<typename T>
    inline bool operator==(T rhs) const { return *this == LuaValue(rhs); }
    template<typename T>
    inline bool operator!=(T rhs) const { return *this != LuaValue(rhs); }
  public:
    /**
     * Pop lua table from stack
     * @param state    - Lua state
     * @param position - Lua stack position
     * @returns New lua table value
     */
    static inline LuaValue PopTable(lua_State *state, int position = 1)
    {
      int     table_ref;
      auto    table_value = LuaValue(table_t());
      int     type = LUA->GetType(position);

      if (type != Type::TABLE)
        throw std::runtime_error("Unable to pop type '" + std::string(LUA->GetTypeName(type)) + "' as table");

      LUA->Push(position);
      table_ref = LUA->ReferenceCreate();
      LUA->ReferencePush(table_ref);
      LUA->PushNil();

      while (LUA->Next(-2))
      {
        LuaValue key;
        LuaValue value;

        LUA->Push(-2);
        LUA->ReferencePush(table_ref);

        if (LUA->Equal(-1, -2))
          throw std::runtime_error("Unable to pop table with cyclic reference");
        else
          key = LuaValue::Pop(state, -2);

        if (LUA->Equal(-1, -3))
          throw std::runtime_error("Unable to pop table with cyclic reference");
        else
          value = LuaValue::Pop(state, -3);

        std::get<table_t>(table_value._value)[key] = value;

        LUA->Pop(3);
      }

      LUA->Pop();
      LUA->ReferenceFree(table_ref);

      return table_value;
    }

    /**
     * Pop lua value from stack
     * @param state    - Lua state
     * @param position - Lua stack position
     * @returns New lua value
     */
    static inline LuaValue Pop(lua_State *state, int position = 1)
    {
      int type = LUA->GetType(position);

      switch (type)
      {
        case Type::NIL:
          return LuaValue();
        case Type::BOOL:
          return LuaValue(LUA->GetBool(position));
        case Type::TABLE:
          return PopTable(state, position);
        case Type::NUMBER:
          return LuaValue(LUA->GetNumber(position));
        case Type::STRING:
          return LuaValue(std::string(LUA->GetString(position)));
        case Type::FUNCTION:
          return LuaValue(LUA->GetCFunction(position));
        case Type::USERDATA:
          return LuaValue(LUA->GetUserdata(position));
      }

      return LuaValue();
    }

    /**
     * Creates empty LuaValue shared_ptr
     * @param type - Lua type
     */
    static inline LuaValue Make(int type)
    {
      switch (type)
      {
        case Type::NIL:
          return LuaValue();
        case Type::BOOL:
          return LuaValue(false);
        case Type::TABLE:
          return LuaValue(table_t());
        case Type::NUMBER:
          return LuaValue(0.0);
        case Type::FUNCTION:
          return LuaValue(__empty);
        case Type::USERDATA:
          return LuaValue((void*)nullptr);
        default:
          return LuaValue(type, nullptr);
      }
    }
  private:
    static int __empty(lua_State *state) { return 0; }
  }; // LuaValue

}} // GarrysMod::Lua

#endif//_GLOO_LUA_VALUE_H_