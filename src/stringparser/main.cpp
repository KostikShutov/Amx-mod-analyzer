#include <iostream>

using namespace std;

extern "C" int execute(int argc,char *argv[]);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: stringparser [input.amx]" << endl;
        return 1;
    }
    execute(argc, argv);
    return 0;
}