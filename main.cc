#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include "vulkan_context.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader_s.h"
#include "shaders/gl_shader_sources.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main(int argc, char** argv) {

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "VulkanGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader zprogram
    // ------------------------------------
    Shader render_image_shader(render_image_vs, render_image_fs);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // load and create a texture 
    // -------------------------
    unsigned int texture1;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1); 
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int image_w, image_h, image_cn;
    // stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *image_file_data = stbi_load("1.jpg", &image_w, &image_h, &image_cn, 4);
    if (image_file_data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_w, image_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_file_data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(image_file_data);
    glfwSetWindowSize(window, image_w, image_h);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    render_image_shader.use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    // glUniform1i(glGetUniformLocation(render_image_shader.ID, "texture1"), 0);
    // or set it via the texture class
    render_image_shader.setInt("texture1", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        // render container
        render_image_shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

/*
    VkResult vkRes;

    uint32_t instanceApiVersion;
    // Get the max. supported Vulkan Version if vkEnumerateInstanceVersion is available (loader version 1.1 and up)
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion) {
        vkEnumerateInstanceVersion(&instanceApiVersion);
    } else {
        instanceApiVersion = VK_API_VERSION_1_0;
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanDemo";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "VulkanDemo";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = instanceApiVersion;

    // Create Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> instanceEnabledExtensions = {};

    // Get instance extensions
    std::vector<VkExtensionProperties> instanceExtensions;
    do {
        uint32_t extCount;
        vkRes = vkEnumerateInstanceExtensionProperties(NULL, &extCount, NULL);
        if (vkRes != VK_SUCCESS) {
            break;
        }
        std::vector<VkExtensionProperties> extensions(extCount);
        vkRes = vkEnumerateInstanceExtensionProperties(NULL, &extCount, &extensions.front());
        instanceExtensions.insert(instanceExtensions.end(), extensions.begin(), extensions.end());
    } while (vkRes == VK_INCOMPLETE);

    // Check support for new property and feature queries
    bool deviceProperties2Available = false;
    for (auto& ext : instanceExtensions) {
        if (strcmp(ext.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
            deviceProperties2Available = true;
            instanceEnabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            continue;
        }
#define ENABLE_INSTANCE_EXT(EXT_NAME) \
if (0 == strcmp(ext.extensionName, EXT_NAME)) { \
    instanceEnabledExtensions.push_back(EXT_NAME); \
    continue; \
}

        ENABLE_INSTANCE_EXT(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME)
        ENABLE_INSTANCE_EXT(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME)

#undef ENABLE_INSTANCE_EXT
    }

    instanceCreateInfo.ppEnabledExtensionNames = instanceEnabledExtensions.data();
    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceEnabledExtensions.size();

    // Create vulkan Instance
    VkInstance instance;
    vkRes = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (vkRes != VK_SUCCESS)
    {
        // QString error;
        if (vkRes == VK_ERROR_INCOMPATIBLE_DRIVER)
        {
            printf("No compatible Vulkan driver found!\nThis version requires a Vulkan driver that is compatible with at least Vulkan 1.1");
        }
        else
        {
            printf("Could not create Vulkan instance!\nError: %s", vulkanResources::resultString(vkRes).c_str());
        }
        return false;
    }

    for (auto& ext : instanceEnabledExtensions) {
        printf("InstanceEnabledExtensions: %s\n", ext);
    }

    if (deviceProperties2Available) {
        pfnGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR"));
        if (!pfnGetPhysicalDeviceFeatures2KHR) {
            deviceProperties2Available = false;
            printf("Could not get function pointer for vkGetPhysicalDeviceFeatures2KHR (even though extension is enabled!)\nNew features and properties won't be displayed!\n");
        }
        pfnGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
        if (!pfnGetPhysicalDeviceProperties2KHR) {
            deviceProperties2Available = false;
            printf("Could not get function pointer for vkGetPhysicalDeviceProperties2KHR (even though extension is enabled!)\nNew features and properties won't be displayed!\n");
        }
    }

    pfnGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));

    uint32_t numGPUs;
    // Enumerate devices
    vkRes = vkEnumeratePhysicalDevices(instance, &numGPUs, NULL);
    if (vkRes != VK_SUCCESS)
    {
        printf("Could not enumerate device count!\n");
        return -1;
    }
    std::vector<VkPhysicalDevice> vulkanDevices;
    vulkanDevices.resize(numGPUs);

    vkRes = vkEnumeratePhysicalDevices(instance, &numGPUs, &vulkanDevices.front());
    if (vkRes != VK_SUCCESS)
    {
        printf("Could not enumerate physical devices!\n");
        return -1;
    }

    std::vector<VkDevice> vk_devices; 
    for (auto physi_device : vulkanDevices) {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(physi_device, &props);
        printf("Device \"%s\"\n", props.deviceName);

        // extensions
        std::vector<VkExtensionProperties> extensions;
        do {
            uint32_t extCount;
            vkRes = vkEnumerateDeviceExtensionProperties(physi_device, NULL, &extCount, NULL);
            assert(!vkRes);
            std::vector<VkExtensionProperties> exts(extCount);
            vkRes = vkEnumerateDeviceExtensionProperties(physi_device, NULL, &extCount, &exts.front());
            for (auto& ext : exts)
                extensions.push_back(ext);
        } while (vkRes == VK_INCOMPLETE);
        assert(!vkRes);

        // queue
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physi_device, &queueFamilyCount, NULL);
        assert(queueFamilyCount > 0);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physi_device, &queueFamilyCount, &queueFamilyProperties.front());

        VkPhysicalDeviceMemoryProperties mem_props = {};
        vkGetPhysicalDeviceMemoryProperties(physi_device, &mem_props);

        float queuePriorities[1] = { 1.0f };
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
        {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = i;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = queuePriorities;
                break;
            }
        }

        std::vector<const char *> deviceEnabledExtensions;
        for (auto& ext : extensions) {
#define ENABLE_DEVICE_EXT(EXT_NAME) \
if (0 == strcmp(ext.extensionName, EXT_NAME)) { \
    deviceEnabledExtensions.push_back(EXT_NAME); \
    continue; \
}

            ENABLE_DEVICE_EXT(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME)
            ENABLE_DEVICE_EXT(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)

#undef ENABLE_DEVICE_EXT
        }

        // Init device

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pQueueCreateInfos = &queueCreateInfo;
        info.queueCreateInfoCount = 1;
        info.enabledLayerCount = 0;
        if (deviceEnabledExtensions.size() > 0) {
            info.enabledExtensionCount = (uint32_t)deviceEnabledExtensions.size();
            info.ppEnabledExtensionNames = deviceEnabledExtensions.data();
        };

        VkDevice vk_device;
        vkRes = vkCreateDevice(physi_device, &info, nullptr, &vk_device);
        if (vkRes != VK_SUCCESS)
        {
            printf("Could not create a Vulkan device!\nError: %s\n", vulkanResources::resultString(vkRes).c_str());
            exit(EXIT_FAILURE);
        }

        for (auto& ext : deviceEnabledExtensions) {
            printf("DeviceEnabledExtensions: %s\n", ext);
        }
        vk_devices.push_back(vk_device);
    }

    for (auto dev : vk_devices) {
        vkDestroyDevice(dev, nullptr);
    }

    vkDestroyInstance(instance, nullptr);
*/

    printf("main exit.\n");

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}