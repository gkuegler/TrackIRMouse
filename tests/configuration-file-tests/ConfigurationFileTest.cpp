#include "../../Config.h"

int main()
{
    // Load Settings
    auto config = config::Config();

    try
    {
        config.LoadSettings();
    }
    catch (std::runtime_error e)
    {
        printf("Load Settings Failed. See TOML error above.");
    }
    catch (const std::exception& ex)
    {
        printf("%s", ex.what());
    }
    catch (...)
    {
        printf("exception has been uncocked");
    }

    return 0;
}