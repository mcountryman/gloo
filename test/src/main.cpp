#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaValue.h>

int echo(lua_State *state) {
  int argc = LUA->Top();

  for (int i = 1; i <= argc; i++) {
    auto value = GarrysMod::Lua::LuaValue::Pop(state, i);

    LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
      LUA->GetField(-1, "print");
      value.Push(state);
      LUA->Call(1, 0);
    LUA->Pop();
  }

  return 0;
}

int test_push(lua_State *state) {
  return 0;
}

GMOD_MODULE_OPEN() {
  auto table = GarrysMod::Lua::LuaValue::Make(GarrysMod::Lua::Type::TABLE);

  //(*table)[table] = table;


  LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
    LUA->CreateTable();
      LUA->PushCFunction(echo);
      LUA->SetField(-2, "echo");

      LUA->PushCFunction(test_push);
      LUA->SetField(-2, "test_push");
    LUA->SetField(-2, "gloo");
  LUA->Pop();

  return 0;
}

GMOD_MODULE_CLOSE() {

  return 0;
}