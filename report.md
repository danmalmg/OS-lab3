# Data Structures
In the file src/thread/thread.h we declared the following attribute to the thread data structure: ‘int16_t ticks_blocked’, with the aim to track how many timer ticks a thread remains blocked. This was done in order to manage thread sleep durations efficiently and letting the blocked thread itself store the information about how long it is blocked.

# Algorithms
In the function timer_sleep(), we create a pointer to the current thread called t.
In order to block a thread, interrupts need to be temporarily disabled. We accomplish this in the timer_sleep() function by utilizing intr_disable(), which disables interrupts and saves the previous interrupt state as a variable old_level. Then, the thread can be blocked using thread_block(). After this, interrupts are enabled by passing old_level to intr_set_level().
The blocked thread stores information about how long it is blocked by setting t->ticks_blocked = ticks. In this case, t->ticks_blocked is the number of blocked ticks that are stored in the threads struct and ticks is the number of ticks to be blocked.
The function update_blocked_ticks() was added in order to activate sleeping threads. The function thread_foreach() takes this function as an argument in order to apply it to every thread. This is done in the timer_interrupt() function which is called once per timer tick. Each time update_blocked_ticks() is called, the status of the current thread is checked. If the thread is blocked, it decrements the number of ticks blocked in the threads struct. The function will be executed until there are no remaining blocked ticks, meaning the thread is ready to wake up. The thread is then woken by using thread_unblock().

# Synchronization
In order to allow synchronization, modifications to the ‘timer_sleep()’ within the file ‘src/devices/timer.c’ were made. Interrupts are disabled using ‘intr_disable()’ before modifying shared data structures. This is done in order to prevent other threads from interrupting and potentially causing race conditions, i.e., when two or more threads can access shared data, and they try to change it at the same time.
After the wanted operations are completed, interrupts are restored to their previous state using intr_set_level(old_level) which ensures that the system is able to handle interrupts normally again.
Since timer_interrupt() is called once per timer tick, which in turn calls the thread_foreach() function that will iterate over all threads and apply the update_blocked_threads() function, we can ensure that all threads are checked each timer tick and updated if needed. Meaning no thread with the same amount of blocked_ticks as another will wake up before that other thread.

# Time and space complexity
## Time complexity:
‘timer_sleep()’
complexity: O(1), because this function performs a constant amount of work by evaluating a condition, then updating the fields within a single thread, and then blocking the thread.

‘thread_foreach(update_blocked_ticks(), NULL)’
complexity: O(n), where n is the number of threads, because the function iterates over all threads and decreases their attribute ‘ticks_blocked’ and potentially unblocks them.

‘timer_interrupt()’
complexity: O(n), due to that this function iterates over all threads to update their blocked status.

## Space complexity:
‘timer_sleep()’
complexity: O(1), this function uses a constant amount of extra space for local variables.

‘update_blocked_ticks()’
complexity: O(1), constant amount of extra space for local variables, e.g., ‘ticks_blocked’.

‘timer_interrupt()’
complexity: O(1), constant space for the global ticks counter.
