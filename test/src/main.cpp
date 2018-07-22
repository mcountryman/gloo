#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaValue.h>
#include <vector>

int echo(lua_State *state) {
  int argc = LUA->Top();
  std::vector<GarrysMod::Lua::LuaValue> args;

  for (int i = 1; i <= argc; i++)
    args.push_back(GarrysMod::Lua::LuaValue::Pop(state, i));
  for (const auto &arg : args)
    arg.Push(state);

  return args.size();
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