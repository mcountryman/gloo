#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaValue.h>
#include <GarrysMod/Lua/LuaObject.h>

class TestObject
  : public GarrysMod::Lua::LuaObject<699, TestObject>
{

};

GMOD_MODULE_OPEN() {


  return 0;
}

GMOD_MODULE_CLOSE() {

  return 0;
}