#ifndef _GLOO_LUA_OBJECT_H_
#define _GLOO_LUA_OBJECT_H_

#include <tuple>
#include <string>
#include <optional>
#include "LuaValue.h"
#include "GarrysMod/Lua/Interface.h"

namespace GarrysMod {
namespace Lua{

  template<int type, class ChildObject>
  class LuaObject
  {
  private:
    enum class Member
    {
      Getter,
      Setter,
      Method,
    };
  private:
    std::map<std::string, std::tuple<member_t, CFunc>> _members;
    std::map<lua_State*, int>                          _references;
  public:
    virtual int destroy(lua_State *state) { return 0; }
  public:
    void AddGetter(std::string name, CFunc fn) { _members[name] = std::make_tuple(member_t::Getter, fn); }
    void AddSetter(std::string name, CFunc fn) { _members[name] = std::make_tuple(member_t::Setter, fn); }
    void AddMethod(std::string name, CFunc fn) { _members[name] = std::make_tuple(member_t::Method, fn); }

    /**
     * @brief push object to lua stack
     * @param state Lua state
     */
    int Push(lua_State *state)
    {
      registerObject(state);

      LUA->ReferencePush(_references[state]);
      return 1;
    }
  private:
    void registerObject(lua_State *state)
    {
      if (_references.find(state) == _references.end())
        return;

      LUA->CreateTable();
        LUA->SetField(-2, "__gc");
        LUA->SetField(-2, "__index");
        LUA->SetField(-2, "__newindex");
        LUA->SetField(-2, "__tostring");

      _references[state] = LUA->ReferenceCreate();
    }
  public:
    static ChildObject* Pop(lua_State *state, int position)
    {
      LUA->CheckType(position, type);
      return (ChildObject*)LUA->GetUserdata(position);
    }
  private:
    static int __gc(lua_State *state) {
      ChildObject *obj = Pop(state, 1);
      obj->destroy(state);
      delete obj;

      // TODO: Remove reference

      return 0;
    }
    static int __index(lua_State *state) {
      ChildObject *obj = Pop(state, 1);

      return 0;
    }
    static int __newindex(lua_State *state) {}
    static int __tostring(lua_State *state) {
      LUA->PushString("");
      return 1;
    }
  }; // LuaObject

}} // GarrysMod::Lua

#endif//_GLOO_LUA_OBJECT_H_