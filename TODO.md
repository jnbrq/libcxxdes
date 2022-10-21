# TODO List for libcxxdes

1. Support priority resources. The infrastructure is already there, you only need to figure out a nice way to delegate the `.priority()` to `co_with (x) {  }`.
2. Make interrupts with a generic interrupt parameter. Idea: store the exception object with interrupts. Use this idea to implement preemptive resources.
3. Implement a container object.
4. Bring `simpy` examples in C++.
5. How to implement a real time environment? Do we even need a real time environment?
6. Write proper documentation.
