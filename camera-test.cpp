#include <signal.h>
#include "onvifpipe.h"

int ter = 0;
void sigint_handler(int arg)
{
	ter = 1;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigint_handler);

    ONVIFPipe pipe("admin", "123456", "uuid:3fa1fe68-b915-4053-a3e1-7405a57fc132");
    pipe.ONVIFDetectDevice(10);

    return 0;
}
