# sib::monad

sib::monad is a library that provides monadic extensions, with the ability to extend to any type
that behaves like a monad - no matter what the syntax.

## namespace sib

### template\<typename R, typename... Args\> class shared_task\<R(Args...)\>

###### **Header:** sib/shared_task.h

shared_task is to std::packaged_task what std::shared_future is to std::future. i.e. it exposes the same essential functionality,
but through a const interface, and the task is copyable.  Furthermore, copies of a shared_task share a common
state.

#### Construction, assignment and destruction

##### ~shared_task() noexcept
Destructor

##### shared_task(shared_task&& rhs) noexcept
Move constructor.

_rhs_ is left in a valid state.

_rhs_ may still share internal state with the constructed shared_task.


##### shared_task(shared_task const& rhs) noexcept
Copy constructor.

Shared tasks that are copies of each other share the same internal state.

##### shared_task(packaged_task\<R(Args...)\> task)
implicit construction from a packaged_task.

It is **Undefined Behaviour** to pass a task that has had either its call operator invoked or its future fetched.

##### template<typename Callable> explicit shared_task(Callable&& callable)
explicit construction from an arbitrary callable.

Invoking _callable_ with _Args..._ must return a type convertible to _R_.

##### shared_task& operator=(shared_task&& rhs) noexcept
Move assignment operator.

_rhs_ is left in a valid state.

_rhs_ may still share internal state with the constructed shared_task.

##### shared_task& operator=(shared_task const& rhs) noexcept
Copy assignment operator.

Shared tasks that are copies of each other share the same internal state.

#### Members

##### void operator(Args... args) const
Call operator.

Calling a shared_task will invoke the underlyinging packaged task on the first call.
Subsequent calls to the same shared_task, or other shared_tasks that share the same internal state will reuse the same value from before - **even if different arguments are supplied**.

In all cases, the call operator will not return until the future returned by get_future is ready.

##### shared_future<R> get_future() const
Getter.

Returns a reference to the internal std::shared_future.  This is created from the initial packaged_task used in construction.
shared_tasks that share the same internal state will return references to the same future.

## namespace sib::monad

### monad
### optional
### function
### task