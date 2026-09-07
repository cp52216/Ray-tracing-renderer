
#include "Renderer.h"

int main() 
{
    Renderer renderer(800, 600, 2, "../scenes/scene03.xml");
    renderer.Run();

    return 0;
}
