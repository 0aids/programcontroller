# Fake input manager structure

The main goal of the fake input manager is to be able to send non-blocking
movement requests to a program. Since mouse movement is just directly adding on,
for interpolating large mouse movements to look human-like, we'll need to
somehow be able to send events to some event manager without blocking.

This is where the child thread comes in. The child thread and the parent input
manager thread will communicate via some sort of pipe (Either linux or shared
memory). The child will read from the pipe, and sends it to a binary heap
priority queue, ordered by timestamp of when it must be sent by. The messages
will contain information about what the movement request is, and the time stamp
that it must be sent at.

Every loop of the child thread, it will:

1. Check the pipe for more input
2. If there are any more requests, add them to the priority queue
3. Check if the priority queue has anything at the current timestamp
4. If the priority queue has something at this timestamp (1 or more), process
   all relevant requests and remove them from the queue.
5. Repeat.

~~Maybe it'll be a good idea to add some sort of sleeping or blocking io? Probably
i'll poll the pipe with a set amount of timeout (1ms), which should be
sufficient sleeping time?~~

There's an option for a lock-free bounded queue. We'll use this queue to send
information to the thread, where the thread can process them.
