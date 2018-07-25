#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaValue.h>
#include <GarrysMod/Lua/LuaObject.h>
#include <GarrysMod/Lua/LuaEvent.h>

#include <chrono>
#include <thread>

using namespace GarrysMod::Lua;

class TestObject
  : public LuaEventEmitter<239, TestObject>
{
  private:
    bool        _run;
    std::string _member;
    std::thread _thread;
  public:
    std::string name() override { return "TestObject"; }
  public:
    TestObject() : LuaEventEmitter()
    {
      _run = true;
      _thread = std::thread(&TestObject::thread_work, this);

      AddGetter("member", get_member);
      AddSetter("member", set_member);
    }
    ~TestObject()
    {
      _run = false;
      _thread.join();
    }
  public:
    void thread_work() 
    {
      while (_run)
      {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);

        Emit("event", "anything", "you", "want", "here", 69.99);
      }
    }

    static int get_member(lua_State *state)
    {
      auto obj = Pop(state, 1);
      
      return LuaValue::Push(state, obj->_member);
    }
    static int set_member(lua_State *state)
    {
      auto obj = Pop(state, 1);
      auto value = LuaValue::Pop(state, 2);

      if (value.type() == Type::STRING)
      {
        obj->_member = value;
      }

      return 0;
    }
}; // TestObject

int make_test_obj(lua_State *state)
{
  return TestObject::Make()->Push(state);
}

GMOD_MODULE_OPEN() {
  LUA->PushSpecial(SPECIAL_GLOB);
    LUA->CreateTable();
      LUA->PushCFunction(make_test_obj);
      LUA->SetField(-2, "make_test");
    LUA->SetField(-2, "gloo");
  LUA->Pop();

  return 0;
}

GMOD_MODULE_CLOSE() {

  return 0;
}