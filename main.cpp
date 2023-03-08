#include"server.h"

int main(int argc, char* argv[]) 
{
    Server server(8888, 8, 3, 60000, true, 1, 1024);
    server.start();
    return 0;
    
}