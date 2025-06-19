# ParaOS OneShotExecutor

OneShotExecutor is a component for asynchronous execution of delegates in a separate thread. It allows queuing delegates for subsequent execution.

## Features

- Execution of delegates in a separate thread
- Queue of delegates with configurable size
- Support for class methods without parameters and return value
- Thread-safe delegate queuing

## Important: Executor and Object Lifetime

### Executor Lifetime

OneShotExecutor must exist throughout the entire program runtime. There are two ways to guarantee this:

1. Declaration in global scope:
```cpp
// In global scope
paraos::OneShotExecutorAttributes global_attrs{
    {{"Global Worker", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kRealTime, nullptr}}
};
paraos::OneShotExecutor<10> global_executor{global_attrs};
```

2. Using `static` when declaring inside a function:
```cpp
void Example() {
    static paraos::OneShotExecutorAttributes attrs{
        {{"Static Worker", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime, nullptr}}
    };
    static paraos::OneShotExecutor<10> executor{attrs};
    // ...
}
```

### Delegate Object Lifetime

**IMPORTANT**: When calling `EnqueueDelegate()`, the delegate is queued and will be executed later in a separate thread. The user MUST ensure that the object whose method is registered in the delegate is not destroyed before the delegate execution. Otherwise, undefined behavior will occur.

Example of incorrect usage:
```cpp
void WrongUsage() {
    MyWorker* worker = new MyWorker();

    // WRONG: worker object might be deleted before delegate execution
    global_executor.EnqueueDelegate<MyWorker, &MyWorker::DoWork>(*worker);
    delete worker;  // Possible undefined behavior!
}
```

## Usage

### Basic Example

```cpp
#include "paraos_oneshot_executor.hpp"
#include <iostream>

// Example class whose methods will be executed through executor
class MyWorker {
public:
    // Constructor can accept initialization parameters
    MyWorker(const char* name = "default") : name_(name) {}

    // Method that will be called through executor
    // IMPORTANT: method must be void(void)
    void DoWork() {
        // Protect console output since method is called from another thread
        const paraos::CriticalSection critical;
        std::cout << "Worker '" << name_ << "' is processing task\n";

        // Here goes the actual work
        ProcessData();
    }

private:
    void ProcessData() {
        // Example of some useful work
        counter_++;
    }

    const char* name_;
    size_t counter_{0};
};

// Create executor in global scope
paraos::OneShotExecutorAttributes executor_attrs{
    {{"Worker Thread", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kRealTime, nullptr}}
};
paraos::OneShotExecutor<10> global_executor{executor_attrs};

// Create global worker object to guarantee its existence
static MyWorker global_worker{"global worker"};

void Example() {
    // Use global worker object
    global_executor.EnqueueDelegate<MyWorker, &MyWorker::DoWork>(global_worker);
}
```

### Using Pre-created Delegate

```cpp
// Global executor for second example
paraos::OneShotExecutorAttributes example2_attrs{
    {{"Worker Thread 2", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kRealTime, nullptr}}
};
paraos::OneShotExecutor<10> example2_executor{example2_attrs};

// Global worker for second example
static MyWorker example2_worker{"example2 worker"};

void Example2() {
    // Create delegate using global object
    auto delegate = paraos::executor_delegate_type::create<MyWorker, &MyWorker::DoWork>(example2_worker);

    // Add delegate to queue
    example2_executor.EnqueueDelegate(delegate);
}
```

### Class Usage Example

```cpp
class SafeUsageExample {
private:
    // Object exists throughout class lifetime
    MyWorker worker;
    bool has_pending_tasks{false};

public:
    void EnqueueWork() {
        has_pending_tasks = true;
        // worker exists as long as SafeUsageExample exists
        global_executor.EnqueueDelegate<MyWorker, &MyWorker::DoWork>(worker);
    }

    // Ensure object is not destroyed until all delegates complete
    ~SafeUsageExample() {
        if (has_pending_tasks) {
            // Wait for all delegates to complete before destruction
            // For example, through synchronization or waiting
            paraos::Thread::DelayMs(1000); // Example! Use proper synchronization in real code
        }
    }
};
```

## API

### Constructor

```cpp
explicit OneShotExecutor(const OneShotExecutorAttributes& attrs, bool thread_start_flag = true)
```

- `attrs`: Attributes for executor initialization
- `thread_start_flag`: Flag indicating whether to start thread immediately (default is true)

### Methods

#### EnqueueDelegate (for class methods)

```cpp
template <typename T, void (T::*Method)(void)>
auto EnqueueDelegate(T& instance)
```

- `T`: Class type
- `Method`: Pointer to class method (must be void(void))
- `instance`: Reference to class instance
- Returns: true if delegate was successfully added to queue

#### EnqueueDelegate (for pre-created delegates)

```cpp
auto EnqueueDelegate(executor_delegate_type delegate)
```

- `delegate`: Delegate to execute
- Returns: true if delegate was successfully added to queue

#### Finish

```cpp
void Finish()
```

Terminates the executor thread.

## Notes

1. Methods passed to executor must be without parameters and return value (void(void)).
2. Queue size is set via template parameter when creating OneShotExecutor.
3. When calling Finish(), an empty delegate is added to queue to unblock executor thread.
4. Executor automatically starts its thread upon creation (unless specified otherwise via thread_start_flag).
