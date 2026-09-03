
#include "Renderer.h"

int main() 
{
    Renderer renderer(800, 600, 10, "../scenes/scene02.xml");
    renderer.Run();

    return 0;
}
