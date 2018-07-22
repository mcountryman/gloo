#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaValue.h>
#include <GarrysMod/Lua/LuaObject.h>

using namespace GarrysMod::Lua;

int do_garbage(lua_State *state)
{
  // implicit cast to bool
  if (LuaValue::Pop(state, 1))
  {
    // implicit cast to map
    std::map<LuaValue, LuaValue> garbage_ops = LuaValue::Pop(state, 2);
  }
}

GMOD_MODULE_OPEN() {
  LUA->PushSpecial(SPECIAL_GLOB);
    LUA->PushCFunction(do_garbage);
    LUA->SetField(-2, "do_garbage");
  LUA->Pop();

  return 0;
}

GMOD_MODULE_CLOSE() {

  return 0;
}