# RogueWorker

RogueWorker is a **multithreading** library with no other dependancies than STL. It allows you to execute functions accross multiple threads.

## Usage

The function to execute with RogueWorker need to have this signature : `void(uint32_t, uint32_t)`

The first argument is the **executing thread's id** and the second is the **number of threads** currently working on the function.

When a function to execute is sent to RogueWorker, a `WorkGroup` object is returned.
In order to wait for the funtion to end, `waitExecutionDone` has to be called on the `WorkGroup`. This also allows you to run two groups on different functions **asynchronously**.

```cpp
uint32_t thread_count(4);
WorkGroup group_1 = RogueWorker.execute(function_1, 2);
WorkGroup group_2 = RogueWorker.execute(function_2, 2);

// function_1 and function_2 are being processed at the same time

group_1.waitExecutionDone();
group_2.waitExecutionDone();
```

## Example

### 1. Include header
```cpp
#include "RogueWorker.hpp"
```

### 2. Create concurrent function
```cpp
#include <vector>

const size_t vec_size(100000000);

std::vector<float> v1(vec_size);
std::vector<float> v2(vec_size);
std::vector<float> v3(vec_size);

void job(uint32_t worker_id, uint32_t worker_count)
{
    // Number of values for which the thread is responsible
    const uint32_t step = vec_size / worker_count;
    // First value for the thread
    const uint32_t start_index = worker_id * step;
    // Last value
    const uint32_t end_index = (worker_id < worker_count - 1) ? start_index + step : vec_size - 1;
    
    // The actuat loop
    for (uint32_t i(start_index); i < end_index; ++i)
    {
        vec3[i] = vec1[i] + vec2[i]
    }
}
```

### 3. Create RogueWorker and execute job
```cpp
// Create a RogueWorker object with 16 thread
const uint32_t thread_count(16);
Rowk::RogueWorker RogueWorker(thread_count);
Rowk::WorkGroup group = RogueWorker.execute(job, thread_count);
```

### Putting all together
```cpp
#include "RogueWorker.hpp"
#include <vector>

const size_t vec_size(100000000);

std::vector<float> v1(vec_size);
std::vector<float> v2(vec_size);
std::vector<float> v3(vec_size);

void job(uint32_t worker_id, uint32_t worker_count)
{
    // Number of values for which the thread is responsible
    const uint32_t step = vec_size / worker_count;
    // First value for the thread
    const uint32_t start_index = worker_id * step;
    // Last value
    const uint32_t end_index = (worker_id == worker_count - 1) ? start_index + step : vec_size - 1;
  
    // The actuat loop
    for (uint32_t i(start_index); i < end_index; ++i)
    {
        v3[i] = v1[i] + v2[i]
    }
}


int main()
{
    // Create a RogueWorker object with 16 thread
    const uint32_t thread_count(16);
    Rowk::RogueWorker RogueWorker(thread_count);
    // Start parallel job
    Rowk::WorkGroup group = RogueWorker.execute(job);
    // Wait for the job to terminate
    group.waitExecutionDone();
  
    return 0;
}
```

It is also possible to use a lambda instead of a pointer to a function
```c++
Rowk::WorkGroup group = RogueWorker.execute([&](uint32_t thread_id, uint32_t group_size){
    /* 
        The code here
    */
});
```

## Performance

1D naive mean filter on **100.000.000** values with a kernel width of **64** on an *i9-9900K*

|Thread count|Execution time|Speed Up|
|------------|--------------|--------|
|1           |3.1 seconds   |x1      |
|16          |0.28 seconds  |x11     |
