#ifndef _GLOO_LUA_EVENT_H_
#define _GLOO_LUA_EVENT_H_

#include <set>
#include <deque>
#include <tuple>
#include <mutex>
#include <sstream>
#include <functional>
#include "LuaValue.h"
#include "LuaObject.h"
#include "GarrysMod/Lua/Interface.h"

namespace GarrysMod {
namespace Lua {

  class ILuaEvent
  {
  public:
    virtual void Think(lua_State*) = 0;
  }; // ILuaEvent

  class LuaEventManager
  {
  private:
    std::set<std::weak_ptr<ILuaEvent>> _events;
    bool _registered;
  public:
    /**
     * @brief create lua think hook using LuaEventManager::think as callback
     * @param state - lua state
     */
    void Register(lua_State *state)
    {
      if (_registered)
        return;

      std::stringstream id;
      id << "LuaEventManager_";
      id << static_cast<const void*>(this);

      LUA->PushSpecial(SPECIAL_GLOB);
        LUA->GetField(-1, "hook");
          LUA->GetField(-1, "Add");
            LUA->PushString("Think");
            LUA->PushString(id.str().c_str());
            LUA->PushCFunction(think);
            LUA->Call(3, 0);

      _registered = true;
    }

    /**
     * @brief register ILuaEvent
     */
    void RegisterEvent(std::weak_ptr<ILuaEvent> event)
    {
      _events.insert(event);
    }
  private:
    static int think(lua_State *state)
    {
      auto manager = Current(state);
      auto events = manager._events;

      for (auto iter = events.begin(); iter != events.end();)
      {
        if (auto event = iter->lock())
        {
          event->Think(state);
          ++iter;
          continue;
        }

        iter = events.erase(iter);
      }

      return 0;
    }
  public:
    static LuaEventManager& Current(lua_State *state)
    {
      static std::map<lua_State*, LuaEventManager> _managers;
      return _managers[state];
    }
  }; // LuaEventManager

  class LuaEvent :
    public LuaObject<240, LuaEvent>,
    public ILuaEvent
  {
  public:
    typedef std::shared_ptr<LuaEvent> shared_t;
  private:
    std::vector<std::tuple<CFunc, bool>> _listeners;
    std::mutex _listeners_mtx;
    std::deque<std::vector<LuaValue>> _emit_queue;
    std::mutex _emit_queue_mtx;
  public:
    LuaEvent() : LuaObject()
    {
      AddMethod("on", on);
      AddMethod("once", once);
      AddMethod("removeAll", removeAll);
    }
  public:
    void AddListener(CFunc fn, bool once = false)
    {
      std::unique_lock<std::mutex> _lock(_listeners_mtx);
      _listeners.push_back(
        std::make_tuple(fn, once)
      );
    }

    void RemoveListeners()
    {
      std::unique_lock<std::mutex> _lock(_listeners_mtx);
      _listeners.clear();
    }

    template<typename... Args>
    void Emit(Args&& ...args)
    {
      std::unique_lock<std::mutex> _lock(_emit_queue_mtx);
      std::vector<LuaValue> lua_args = {LuaValue(args)...};

      _emit_queue.push_back(lua_args);
    }
  public:
    void Think(lua_State *state) override
    {
      std::unique_lock<std::mutex> _lock(_emit_queue_mtx);

      while (!_emit_queue.empty())
      {
        auto args = _emit_queue.front();
        _emit_queue.pop_front();

        std::unique_lock<std::mutex> _lock(_listeners_mtx);
        for (auto it = _listeners.begin(); it != _listeners.end();)
        {
          auto fn = std::get<0>(*it);
          auto once = std::get<1>(*it);

          LUA->PushCFunction(fn);
          for (auto &arg : args)
            arg.Push(state);
          LUA->Call(args.size(), 0);

          if (once)
            it = _listeners.erase(it);
          else
            ++it;
        }
      }
    }
  private:
    static int get_listeners_count(lua_State *state)
    {
      auto obj = Pop(state, 1);
      std::unique_lock<std::mutex> _lock(obj->_listeners_mtx);

      return LuaValue::Push(state, obj->_listeners.size());
    }

    static int on(lua_State *state)
    {
      LUA->CheckType(2, Type::FUNCTION);

      auto obj = Pop(state, 1);
      auto fn = LUA->GetCFunction(2);

      obj->AddListener(fn, false);

      LuaEventManager::Current(state).Register(state);
      LuaEventManager::Current(state).RegisterEvent(std::static_pointer_cast<ILuaEvent>(obj));

      return 0;
    }

    static int once(lua_State *state)
    {
      LUA->CheckType(2, Type::FUNCTION);

      auto obj = Pop(state, 1);
      auto fn = LUA->GetCFunction(2);

      obj->AddListener(fn, false);

      LuaEventManager::Current(state).Register(state);
      LuaEventManager::Current(state).RegisterEvent(std::static_pointer_cast<ILuaEvent>(obj));

      return 0;
    }

    static int removeAll(lua_State *state)
    {
      auto obj = Pop(state, 1);
      obj->RemoveListeners();

      return 0;
    }
  }; // LuaEvent


  bool operator< (
    const std::weak_ptr<ILuaEvent> &lhs,
    const std::weak_ptr<ILuaEvent> &rhs
  )
  {
    auto lptr = lhs.lock();
    auto rptr = rhs.lock();

    if (!rptr) return true;
    if (!lptr) return false;

    return lptr < rptr;
  }

  bool operator==(
    const std::weak_ptr<ILuaEvent> &lhs,
    const std::weak_ptr<ILuaEvent> &rhs
  )
  {
    auto lptr = lhs.lock();
    auto rptr = rhs.lock();

    if (!rptr) return false;
    if (!lptr) return true;

    return lptr == rptr;
  }

}} // GarrysMod::Lua

#endif//_GLOO_LUA_EVENT_H_