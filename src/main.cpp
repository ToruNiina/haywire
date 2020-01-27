#include <haywire/world.hpp>
#include <haywire/gui.hpp>

int main()
{
    std::cerr << "Usage:"                     << std::endl;
    std::cerr << "Space: toggle execution"    << std::endl;
    std::cerr << "Enter: step-by-step update" << std::endl;

    haywire::window win;
    while(win.update()) {}

    return 0;
}
