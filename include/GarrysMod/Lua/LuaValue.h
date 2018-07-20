#ifndef _GLOO_LUA_VALUE_H_
#define _GLOO_LUA_VALUE_H_

#include <map>
#include <string>
#include <memory>
#include "GarrysMod/Lua/Interface.h"

namespace GarrysMod {
namespace Lua {
  class LuaValue
  {
  public:
    typedef std::weak_ptr<LuaValue> weak;
    typedef std::shared_ptr<LuaValue> shared;

    typedef bool boolean_t;
    typedef std::map<LuaValue, LuaValue> table_t;
    typedef double number_t;
    typedef std::string string_t;
    typedef CFunc function_t;
    typedef void* userdata_t;

    union value_t
    {
      table_t table;
      number_t number;
      string_t string;
      boolean_t boolean;
      function_t function;
      userdata_t userdata;

      value_t() noexcept {}
      value_t(table_t value) noexcept : table(value) {}
      value_t(number_t value) noexcept : number(value) {}
      value_t(string_t value) noexcept : string(value) {}
      value_t(boolean_t value) noexcept : boolean(value) {}
      value_t(function_t value) noexcept : function(value) {}
      value_t(userdata_t value) noexcept : userdata(value) {}
      ~value_t() {}
    };
  private:
    int     _type;
    value_t _value;
  public:
    int type() const { return _type; }
  public:
    LuaValue() : _value() { _type = Type::NIL; }
    LuaValue(table_t value) : _value(value) { _type = Type::TABLE; }
    LuaValue(number_t value) : _value(value) { _type = Type::NUMBER; }
    LuaValue(string_t value) : _value(value) { _type = Type::STRING; }
    LuaValue(boolean_t value) : _value(value) { _type = Type::BOOL; }
    LuaValue(function_t value) : _value(value) { _type = Type::FUNCTION; }
    LuaValue(userdata_t value) : _value(value) { _type = Type::USERDATA; }
    LuaValue(int type, userdata_t value) : _value(value) { _type = type; }
    LuaValue(const LuaValue& value)
    {
      _type = value._type;

      switch (_type)
      {
        case Type::NIL: break;
        case Type::BOOL: _value.boolean = value._value.boolean; break;
        case Type::NUMBER: _value.number = value._value.number; break;
        case Type::STRING: _value.string = value._value.string; break;
        case Type::FUNCTION: _value.function = value._value.function; break;
        default: _value.userdata = value._value.userdata; break;
      }
    }
    LuaValue(const char *value) : _value(std::string(value)) { _type = Type::STRING; }
  public:
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

      for (const auto &pair : _value.table)
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
        case Type::NUMBER: LUA->PushNumber(_value.number); break;
        case Type::STRING: LUA->PushString(_value.string.c_str()); break;
        case Type::BOOL: LUA->PushBool(_value.boolean); break;
        case Type::FUNCTION: LUA->PushCFunction(_value.function); break;
        default:
          LUA->PushUserdata(_value.userdata);
          break;
      }
    }
  private:
    void AssertType(int type) const
    {
      if (_type != type)
        throw std::runtime_error("Expected type '" + std::string("") + "' not type '" + std::string("") + "'");
    }
  public:
    inline LuaValue& operator= (const LuaValue& rhs)
    {
      if (*this == rhs)
        return *this;

      _type = rhs._type;

      switch (_type)
      {
        case Type::NIL: break;
        case Type::BOOL: _value.boolean = rhs._value.boolean; break;
        case Type::NUMBER: _value.number = rhs._value.number; break;
        case Type::STRING: _value.string = rhs._value.string; break;
        case Type::FUNCTION: _value.function = rhs._value.function; break;
        default: _value.userdata = rhs._value.userdata; break;
      }

      return *this;
    }

    inline bool operator< (const LuaValue& rhs) const
    {
      if (_type != rhs._type) return _type < rhs._type;

      switch (_type)
      {
        case Type::NIL: return false;
        case Type::BOOL: return _value.boolean < rhs._value.boolean;
        case Type::NUMBER: return _value.number < rhs._value.number;
        case Type::STRING: return _value.string < rhs._value.string;
        case Type::FUNCTION: return _value.function < rhs._value.function;
        default: return _value.userdata < rhs._value.userdata;
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
        case Type::BOOL: return _value.boolean == rhs._value.boolean;
        case Type::TABLE: return _value.table == rhs._value.table;
        case Type::NUMBER: return _value.number == rhs._value.number;
        case Type::STRING: return _value.string == rhs._value.string;
        case Type::FUNCTION: return _value.function == rhs._value.function;
        default: return _value.userdata == rhs._value.userdata;
      }
    }
    inline bool operator!=(const LuaValue& rhs) const { return !(*this == rhs); }
    inline LuaValue& operator[](LuaValue idx)
    {
      AssertType(Type::TABLE);
      return _value.table[idx];
    }

    explicit operator bool() const {
      AssertType(Type::BOOL);
      return _value.boolean;
    }
    explicit operator table_t() const {
      AssertType(Type::TABLE);
      return _value.table;
    }
    explicit operator number_t() const {
      AssertType(Type::NUMBER);
      return _value.number;
    }
    explicit operator string_t() const {
      AssertType(Type::STRING);
      return _value.string;
    }
    explicit operator const char*() const {
      AssertType(Type::STRING);
      return _value.string.c_str();
    }
    explicit operator function_t() const {
      AssertType(Type::FUNCTION);
      return _value.function;
    }
    explicit operator userdata_t() const {
      return _value.userdata;
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
      table_t table;
      int     table_ref;
      auto    table_value = LuaValue(table);
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

        table[key] = value;

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
          return LuaValue(LUA->GetString(position));
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