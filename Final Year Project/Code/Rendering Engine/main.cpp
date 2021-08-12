#include "VulkanApplication.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

int main() 
{
    VulkanApplication app;

    try 
    {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}