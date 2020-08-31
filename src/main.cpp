#include <haywire/world.hpp>
#include <haywire/gui.hpp>
#include <extlib/wad/wad/default_archiver.hpp>

int main(int argc, char **argv)
{
    std::cerr << "Usage: ./haywire [data.toml]" << std::endl;
    std::cerr << "Space: toggle execution"      << std::endl;
    std::cerr << "Enter: step-by-step update"   << std::endl;

    haywire::window win;

    if(argc == 2)
    {
        const std::string fname(argv[1]);
        if(fname.substr(fname.size() - 5) == ".toml")
        {
            win.load_toml(argv[1]);
        }
        else if(fname.substr(fname.size() - 4) == ".msg")
        {
            wad::read_archiver src(fname);
            if(!wad::load(src, win))
            {
                return 1;
            }
        }
    }
    while(win.update()) {}

    return 0;
}
