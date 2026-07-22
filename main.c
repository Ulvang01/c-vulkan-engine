#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>

int main(void) {
    if (!glfwInit()) {
        printf("GLFW init failed\n");
        return 1;
    }
    printf("GLFW initialized OK\n");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "BoogerAids", NULL, NULL);
    if (!window) {
        printf("Window creation failed\n");
        glfwTerminate();
        return 1;
    }
    printf("Window created OK\n");

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        printf("vkCreateInstance failed: %d\n", result);
        return 1;
    }
    printf("Vulkan instance created OK\n");

    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}