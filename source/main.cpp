
#include "Renderer.h"

int main() 
{
    Renderer renderer(800, 600, 10, 100, "../scenes/scene04.xml");
    renderer.Run();

    return 0;
}
