# COM3610 Programming Assignment 2 Threading and Synchronization

Multithreaded Web Server and Client

### Azriel Bachrach & Yaakov Baker

## How Work was Divided

We started of working together on Part 1. We researched threads, condition variables, semaphores, and mutexes and got the server to work using pop-up threads instead of processes. We came up with the thread logic and flow together and converted the pop-up threads to a thread pool. Azriel created the structures we would need and the logic to deal with setting the values and accessing them in the threads as well as making things thread local.

For part 2, Yaakov created the Priority Queue that functioned as the buffer and then we worked together to modify it based on changes to how the threads worked, like changing the elements put into the buffer to a struct containing certain information instead of the connections themselves. 

At the same time, Azriel worked on Part 3, having the server keep track of usage statistics and then reporting them as entity headers.

We worked together on Part 4 to make the client multi-threaded. We created the main function logic and flow together and how it creates the necessary thread(s). The Concur and FIFO threads logic and flow were done together. Azriel figured out where to put thread barriers and Yaakov figured out where to put the semaphore logic.


## Design Overview

#### The Server

We used a producer consumer model for the main server thread and the worker threads that handle the connection. When the main server thread receives a connection, we put the connection into a buffer. We use a mutex to be sure that no other thread is accessing the buffer at the same time and a condition variable to wait for there to be space in the buffer.

The worker threads use a condition variable to wait until there is something in the buffer. When there is something in the buffer, the thread handles the connection. It takes the connection data, reads from the connection, figures out what file to send, and then sends the file.

#### Scheduling Policies

To deal with different scheduling policies, we used a priority queue as our buffer. The priority of a request in the buffer is determined by the chosen scheduling algorithm and the type of file that was requested. With the regular FIFO semantics the priority queue would prioritize those that came first by giving everything a priority of 1 and since equal values being put into the buffer would go after those already in the buffer we would get FIFO queue semantics. with HPHC and HPIC we gave the higher priority format the priorty of 2 and the lower priority format a value of 1 and within these priorities the FIFO semantics as defined above were used. A newer image file would go after an already existant image file. 

#### The Client

When the client is started it creates the specified number of threads from the command line. If the client is run in concurrent mode (CONCUR), all threads send and receive a request simultaneously before waiting at a thread barrier for all threads to catch up and then repeating.

If the client is run in FIFO mode, a semaphore is used to insure that only one thread makes a request at a time, then all threads wait at a thread barrier until all responses are received.

## Complete Specification

If no scheduling algorithm is specified, the server defaults to FIFO. The number of threads defaults to 7 if unspecified and the buffer size defaults to 13.

## Known Bugs or Problems

All required functionality seems to work properly

## Testing

We tested that the server was able to handle requests properly and return the correct statistics using a combination of the original client, Google Chrome, and [Postman](https://www.postman.com/).

To test FIFO, HPSC, and HPDC policies, we put ``sleep(10);`` in *server.c* before the it starts accepting requests. Then after starting the server, but before the 10 seconds are up, we quickly send a number of requests for different data types. we can then compare the arrival and dispatch times to see the order that requests were scheduled in.

