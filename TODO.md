# TODO List for libcxxdes

1. Support priority resources. The infrastructure is already there, you only need to figure out a nice way to delegate the `.priority()` to `co_with (x) {  }`.
2. final_suspend() should rethrow an exception for tasks?
3. Make interrupts with a generic interrupt parameter. Idea: store the exception object with interrupts. Use this idea to implement preemptive resources.
4. Implement a container object.
5. Bring `simpy` examples in C++.
6. How to implement a real time environment? Do we even need a real time environment?
7. Write proper documentation.
