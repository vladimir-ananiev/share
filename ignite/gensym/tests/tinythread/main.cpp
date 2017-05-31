#include <stdio.h>
#include "tinythread.h"

void thread_proc(void* p)
{
    puts("Thread ENTER");

    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(2550));
    
    puts("Thread LEAVE");
}

int main(int argc, char* argv[])
{
    tthread::thread thread(thread_proc, NULL);

    // Let thread start
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));

    if (!thread.join(2500))
    {
        puts("Thread is timed out and will be killed");
        thread.kill();
    }
    else
    {
        puts("Thread is completed in time");
    }

    puts("Checking for crash...");
    //puts("Press ENTER to exit");
    //getchar();
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
    puts("No crash. GOOD.");

    return 0;
}


