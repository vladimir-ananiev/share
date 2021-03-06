#include <stdio.h>
#include "tinythread.h"

void thread_proc(void* p)
{
    puts("Thread ENTER");

    tthread::thread::make_cancel_safe();
    
    //tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10000));
    while (1)tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1));
    
    puts("Thread LEAVE");
}

int main(int argc, char* argv[])
{
    tthread::thread thread(thread_proc, NULL);

    // Let thread start
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));

    if (!thread.join(2500))
    {
        puts("Thread is timed out and will be cancelled");
        thread.cancel();
//         tthread::thread kill(kill_proc, &thread);
//         kill.join();
    }
    else
    {
        puts("Thread is completed in time");
    }

    puts("Checking for crash...");
    puts("Press ENTER to exit");
    getchar();
//     tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
    puts("No crash. GOOD.");

    return 0;
}


