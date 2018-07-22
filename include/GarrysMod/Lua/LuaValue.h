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

    union value_t
    {
      table_t   *table;
      number_t   number;
      string_t  *string;
      boolean_t  boolean;
      function_t function;
      userdata_t userdata;

      value_t() noexcept {}
      value_t(table_t value) noexcept {
        table = create<table_t>(value);
      }
      value_t(number_t value) noexcept : number(value) {}
      value_t(string_t value) noexcept {
        string = create<string_t>(value);
      }
      value_t(boolean_t value) noexcept : boolean(value) {}
      value_t(function_t value) noexcept : function(value) {}
      value_t(userdata_t value) noexcept : userdata(value) {}
      ~value_t() {
      
      }
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
    LuaValue(LuaValue &&value) { Copy(value); }
    ~LuaValue() {
      switch (_type) {
        case Type::TABLE:
        {
          std::allocator<table_t> allocator;
          std::allocator_traits<decltype(allocator)>::destroy(allocator, _value.table);
          std::allocator_traits<decltype(allocator)>::deallocate(allocator, _value.table, 1);
          break;
        }
        case Type::STRING:
        {
          std::allocator<string_t> allocator;
          std::allocator_traits<decltype(allocator)>::destroy(allocator, _value.string);
          std::allocator_traits<decltype(allocator)>::deallocate(allocator, _value.string, 1);
          break;
        }
      }
    }
  public:
    void Copy(LuaValue &that)
    {
      std::swap(_type, that._type);
      std::swap(_value, that._value);
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

      for (const auto &pair : *_value.table)
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
        case Type::STRING: LUA->PushString(_value.string->c_str()); break;
        case Type::BOOL: LUA->PushBool(_value.boolean); break;
        case Type::FUNCTION: LUA->PushCFunction(_value.function); break;
        default:
          LUA->PushUserdata(_value.userdata);
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
      std::swap(_type, rhs._type);
      std::swap(_value, rhs._value);

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
      assert_type(Type::TABLE);
      return _value.table->operator[](idx);
    }

    explicit operator bool() const {
      assert_type(Type::BOOL);
      return _value.boolean;
    }
    explicit operator table_t() const {
      assert_type(Type::TABLE);
      return *_value.table;
    }
    explicit operator number_t() const {
      assert_type(Type::NUMBER);
      return _value.number;
    }
    explicit operator string_t() const {
      assert_type(Type::STRING);
      return *_value.string;
    }
    explicit operator const char*() const {
      assert_type(Type::STRING);
      return _value.string->c_str();
    }
    explicit operator function_t() const {
      assert_type(Type::FUNCTION);
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

    template<typename T, typename... Args>
    static T* create(Args&& ...args) {
      using AllocatorType = std::allocator<T>;
      using AllocatorTraits = std::allocator_traits<AllocatorType>;

      AllocatorType alloc;

      auto deleter = [&](T *object) {
        AllocatorTraits::deallocate(alloc, object, 1);
      };
      std::unique_ptr<T, decltype(deleter)> object(AllocatorTraits::allocate(alloc, 1), deleter);
      AllocatorTraits::construct(alloc, object.get(), std::forward<Args>(args)...);
      assert(object != nullptr);
      return object.release();
    }
  }; // LuaValue

}} // GarrysMod::Lua

#endif//_GLOO_LUA_VALUE_H_