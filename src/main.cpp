#include <haywire/world.hpp>
#include <haywire/gui.hpp>

int main(int argc, char **argv)
{
    std::cerr << "Usage: ./haywire [data.toml]" << std::endl;
    std::cerr << "Space: toggle execution"      << std::endl;
    std::cerr << "Enter: step-by-step update"   << std::endl;

    haywire::window win;

    if(argc == 2)
    {
        win.load_toml(argv[1]);
    }
    while(win.update()) {}

    return 0;
}
