# gloo
A OOP helper library intended to aid the creation of modern C++17 binary modules in Garry's Mod.

## Getting Started
#### Download
```shell
git submodule add https://github.com/mcountryman/gloo vendor/gloo
git submodule update --init --recursive
```
#### Premake 5
```lua
project "..."
  include ".../gloo"
```
## Header Overview
```cpp
#include <GarrysMod/Lua/LuaValue.h>
  // class LuaValue
#include <GarrysMod/Lua/LuaObject.h>
  // class LuaObject
#include <GarrysMod/Lua/LuaEvent.h>
  // class LuaEventEmitter
  // class ILuaEventEmitter
  // class LuaEventEmitterManager
```
## Examples
Check out `test/src/gloo_test.cpp` for an example that goes over 99% of the features of this library.
### LuaValue
You can create a LuaValue two ways both shown below.  The Make method creates an emtpy LuaValue with the supplied type, and the Pop method creates a LuaValue by popping the data from the Lua stack. 

```cpp
using namespace GarrysMod::Lua;

// Make will default create the object however, default the stored value to 
// something sensible.  For Types::BOOL gloo defaults the value to false.
LuaValue empty = LuaValue::Make(Types::BOOL);

// Pop method requires a lua state and a position to pop from.
LuaValue value = LuaValue::Pop(state, 1);
```

With this newly created LuaValue object we can use this directly in our C++ library or we can pass it back to the Lua stack.
```cpp
using namespace GarrysMod::Lua;

LuaValue value = ...;

// Push to lua stack
value.Push(state);

// Validate type
if (value.type() == Type::BOOL)
{
  // Automatically cast to boolean
  bool boolean = value;

  method_with_one_boolean_parameter(value);
}
```

It is important to note that when invoking the cast operator for a LuaValue then [assert](https://en.cppreference.com/w/cpp/error/assert) method is used to ensure the underlying lua type is correctly associated with the requesting cast type.

### LuaObject
With the LuaObject base we can create OOP lua objects.
```cpp
using namespace GarrysMod::Lua;

class Object :
  // The number in the first parameter of the template for LuaObject is used as
  // and type identifier for lua.  The second is our defined object.
  public LuaObject<123, Object>
{
private:
  int _field;
public:
  Object() : LuaObject() // The ctor call to LuaObject is used to define the
                         // default metamethods __gc, __index, __newindex, and
                         // __tostring.  It is very important that these are 
                         // defined and used.
  {
    AddGetter("name_of_field", getter_method); // Add getter method
    AddSetter("name_of_field", setter_method); // Add setter method
    AddMethod("do_thing", do_thing);
  }
public:
  void DoThing() { ... }
private: // Personal preference for myself is to define lua callbacks as static
         // to not de-organize C++ class members.

  static int getter_method()
  {
    // We can pop the instance to our object by using the Pop method show below
    auto obj = Pop(state, 1);

    // From here we can expose our private field value to lua 
    return LuaValue::Push(state, obj->_field);
  }

  static int setter_method()
  {
    // Guess what we want to do here?
    auto obj = Pop(state, 1); // :0

    // Now we use our handy LuaValue magic to grab a value to store from Lua
    auto value = LuaValue::Pop(state, 2);

    // And ensure it is the correct type
    if (value.type() != Type::NUMBER)
      LUA->ArgError(2, "Expected number!");

    // Finally assign it to the _field field
    obj->_field = value;

    return 0;
  }

  static int do_thing(lua_State *state)
  {
    // Fairly self-explanitory
    Pop(state, 1)->DoThing();

    return 0;
  }
};
```

### LuaEventEmitter
Now that we are all the way down here we can discuss the fun stuff!  The LuaEventEmitter is a base class to be used similarly to the LuaObject class however, it comes with some pretty usefull abilities.

For one our LuaObject will automatically have the following methods exposed to Lua
```typescript
obj:on(event: String, callback: Function)
obj:once(event: String, callback: Function)
obj:add_listener(event: String, callback: Function, delete_after_invokation: Boolean)
obj:remove_listeners()
```

Using these methods we call add Lua callbacks to be invoked when the Think hook is called.
```cpp
class Object : public LuaEventEmitter<?, Object>
{
public:
  void AsyncWork()
  {
    Emit("event_name", "any", "valid", {{ "lua", "value" }}, 69, LuaValue::Make(Type::NIL));
  }
};
```

The `Think` hook is added and removed behind the scenes via the `LuaEventEmitterManager` object.  Hooking is done when a listener is created and removal is done when there are zero active `LuaEventEmitter` objects in the `LuaEventEmitter`.  Registration of a `LuaEventEmitter` is again, done when a listener is created.

Several potentially obscure things to note; data passed to the `Emit` method will not be dequeued until a valid listener is present during a `Think` event.  The `Think` method in `LuaEventEmitter` is configured by default (via `max_events_per_tick`) to only dequeue 100 events per call.  This can be changed by invoking the `max_events_per_tick` method with an integer value as the first parameter as shown below.

```cpp
class Object : public LuaEventEmitter<?, Object>
{
  Object() : LuaEventEmitter() // As stated for LuaObject it is important to call
                            // the parent constructor here if we want access to
                            // the fields defined by LuaObject and now LuaEventEmitter
  {
    // Increase the maximum number of events to be dequeued per tick to 1000.
    max_events_per_tick(1000);
  }
};
```