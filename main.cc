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
#include "vulkan_texture.h"
#include "vulkan_sampler.h"
#include "vulkan_command_buffer.h"
#include "vulkan_shader.h"
#include "shaders/vk_kernel_sources.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader_s.h"
#include "shaders/gl_shader_sources.h"

void window_refresh_callback(GLFWwindow* window);
void processInput(GLFWwindow *window);
GLuint GetGLExternalTexture(const HANDLE mem_hnd, size_t mem_size, GLenum internal_format, int width, int height);

std::shared_ptr<VulkanContext> g_vk_context;
std::shared_ptr<VulkanTexture> g_vk_dst_tex;
std::shared_ptr<VulkanComputePipeline> g_vk_pipeline;
std::shared_ptr<VulkanCommandBuffer> g_vk_cmd_buf;
std::shared_ptr<VulkanSemaphore> g_vk_ext_signal_semaph;

std::unique_ptr<Shader> g_render2screen_shader;
GLuint g_gl_ext_wait_semaph;
GLuint g_gl_ext_tex1;
GLuint VBO, VAO, EBO;
std::map<GLuint, GLuint> g_gl_ext_texs;

int main(int argc, char** argv) {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int win_w = 1600;
    const int win_h = 900; 
    GLFWwindow *window = glfwCreateWindow(win_w, win_h, "VulkanGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowRefreshCallback(window, window_refresh_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Load vulkan
    g_vk_context = std::make_shared<VulkanContext>();
    CHECK_RETURN_IF(0 != g_vk_context->Init(), -1);
    // create vulkan shader
    auto vk_shader = std::make_shared<VulkanShader>(g_vk_context);
    CHECK_RETURN_IF(0 != vk_shader->CreateShader(vk_copy_kernel_comp, vk_copy_kernel_comp_len), -1);

    // create vk-gl shared semaphore
    g_vk_ext_signal_semaph = std::make_shared<VulkanSemaphore>(g_vk_context);
    HANDLE ext_vk_signal_semaph_hnd;
    CHECK_RETURN_IF(0 != g_vk_ext_signal_semaph->CreateWin32ExternalSemaphore(ext_vk_signal_semaph_hnd), -1);
    glGenSemaphoresEXT(1, &g_gl_ext_wait_semaph);
	glImportSemaphoreWin32HandleEXT(g_gl_ext_wait_semaph, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, ext_vk_signal_semaph_hnd);


    // load and create textures
    // -------------------------    
    // load image from file
    int image_w, image_h, image_cn;
    // stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *image_file_data = stbi_load("1.jpg", &image_w, &image_h, &image_cn, 4);
    CHECK_RETURN_IF(!image_file_data, -1);

    // create vulkan ext texture
    HANDLE src_mem_hnd;
    size_t src_mem_size;
    auto vk_src_tex = std::make_shared<VulkanTexture>(g_vk_context);
    int vk_ret = vk_src_tex->CreateWin32GlSharedTexture(VK_FORMAT_R8G8B8A8_UNORM,
                                                        image_w,
                                                        image_h,
                                                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                        VK_IMAGE_TILING_OPTIMAL,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                        VK_IMAGE_LAYOUT_UNDEFINED,
                                                        src_mem_hnd,
                                                        src_mem_size);
    CHECK_RETURN_IF(0 != vk_ret, vk_ret);
    // opengl gl_ext_tex0
    GLuint gl_ext_tex0 = GetGLExternalTexture(src_mem_hnd, src_mem_size, GL_RGBA8, image_w, image_h);
    CHECK_RETURN_IF(0 == gl_ext_tex0, -1);
    // upload image data
    glBindTexture(GL_TEXTURE_2D, gl_ext_tex0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_w, image_h, GL_RGBA, GL_UNSIGNED_BYTE, image_file_data);
    stbi_image_free(image_file_data);

    HANDLE dst_mem_hnd;
    size_t dst_mem_size;
    g_vk_dst_tex = std::make_shared<VulkanTexture>(g_vk_context);
    vk_ret = g_vk_dst_tex->CreateWin32GlSharedTexture(VK_FORMAT_R8G8B8A8_UNORM,
                                                      win_w,
                                                      win_h,
                                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                      VK_IMAGE_TILING_OPTIMAL,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                                      dst_mem_hnd,
                                                      dst_mem_size);
    CHECK_RETURN_IF(0 != vk_ret, vk_ret);
    // opengl g_gl_ext_tex1
    g_gl_ext_tex1 = GetGLExternalTexture(dst_mem_hnd, dst_mem_size, GL_RGBA8, win_w, win_h);
    CHECK_RETURN_IF(0 == g_gl_ext_tex1, -1);

    auto vk_sampler = std::make_shared<VulkanSampler>(g_vk_context);
    CHECK_RETURN_IF(0 != vk_sampler->CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE), -1);

    g_vk_pipeline = std::make_shared<VulkanComputePipeline>(g_vk_context);
    g_vk_pipeline->Bind(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, vk_src_tex->DescriptorInfo());
    g_vk_pipeline->Bind(1, VK_DESCRIPTOR_TYPE_SAMPLER, vk_sampler->DescriptorInfo());
    g_vk_pipeline->Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, g_vk_dst_tex->DescriptorInfo());
    CHECK_RETURN_IF(0 != g_vk_pipeline->CreatePipeline(vk_shader), -1);

    g_vk_cmd_buf = std::make_shared<VulkanCommandBuffer>(g_vk_context);
    CHECK_RETURN_IF(0 != g_vk_cmd_buf->Create(), -1);
    g_vk_cmd_buf->Begin();
    g_vk_cmd_buf->BindPipeline(g_vk_pipeline);
    g_vk_cmd_buf->Dispatch(win_w, win_h);
    g_vk_cmd_buf->End();
    // g_vk_cmd_buf->AddSignalSemaphore(g_vk_ext_signal_semaph);

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

    // build and compile our shader zprogram
    g_render2screen_shader = std::unique_ptr<Shader>(new Shader(render_image_vs, render_image_fs));
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    g_render2screen_shader->use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    // glUniform1i(glGetUniformLocation(g_render2screen_shader->ID, "texture1"), 0);
    g_render2screen_shader->setInt("texture1", 0);

    // render loop
    // -----------
    int loop_cnt = 0;
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        window_refresh_callback(window);

        // glfw: poll IO events (keys pressed/released, mouse moved etc.)
        // On some platforms(windows), a window move, resize or menu operation will cause event processing to block
        glfwPollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    for (auto ext_tex : g_gl_ext_texs) {
        glDeleteMemoryObjectsEXT(1, &(ext_tex.second));
        glDeleteTextures(1, &(ext_tex.first));
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();

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
void window_refresh_callback(GLFWwindow* window) {    
    int win_x;
    int win_y;
    int win_w;
    int win_h;
    glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetWindowPos(window, &win_x, &win_y);
    if ((win_w != g_vk_dst_tex->Width()) || (win_h != g_vk_dst_tex->Height())) {
        auto it = g_gl_ext_texs.find(g_gl_ext_tex1);
        if (it != g_gl_ext_texs.end()) {
            glDeleteMemoryObjectsEXT(1, &(it->second));
            glDeleteTextures(1, &(it->first));
            g_gl_ext_texs.erase(it);
        }

        HANDLE dst_mem_hnd;
        size_t dst_mem_size;
        g_vk_dst_tex = std::make_shared<VulkanTexture>(g_vk_context);
        int vk_ret = g_vk_dst_tex->CreateWin32GlSharedTexture(VK_FORMAT_R8G8B8A8_UNORM,
                                                            win_w,
                                                            win_h,
                                                            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                            VK_IMAGE_TILING_OPTIMAL,
                                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                                            dst_mem_hnd,
                                                            dst_mem_size);
        CHECK_RETURN_IF(0 != vk_ret, CHECK_VOID);
        g_gl_ext_tex1 = GetGLExternalTexture(dst_mem_hnd, dst_mem_size, GL_RGBA8, win_w, win_h);
        CHECK_RETURN_IF(0 == g_gl_ext_tex1, CHECK_VOID);

        g_vk_pipeline->Bind(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, g_vk_dst_tex->DescriptorInfo());
        g_vk_pipeline->UpdateBindings();

        g_vk_cmd_buf->Begin();
        g_vk_cmd_buf->BindPipeline(g_vk_pipeline);
        g_vk_cmd_buf->Dispatch(win_w, win_h);
        g_vk_cmd_buf->End();
    }

    g_vk_cmd_buf->Submit();
    // g_vk_cmd_buf->WaitComplete();

    // render
    // ------
    glViewport(0, 0, win_w, win_h);

    GLenum layout = GL_LAYOUT_TRANSFER_SRC_EXT;
    glWaitSemaphoreEXT(g_gl_ext_wait_semaph, 0, nullptr, 1, &g_gl_ext_tex1, &layout);

    // render container
    g_render2screen_shader->use();
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl_ext_tex1);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
}

GLuint GetGLExternalTexture(const HANDLE mem_hnd, size_t mem_size, GLenum internal_format, int width, int height) {
    GLuint gl_tex = 0;
    GLuint mem = 0;
    GLenum gl_err = GL_NO_ERROR;
    do {
        glCreateMemoryObjectsEXT(1, &mem);
        if (!mem) {
            std::cout << __FUNCTION__ << ": glCreateMemoryObjectsEXT failed:" << glGetError();
            break;
        };

		glImportMemoryWin32HandleEXT(mem, mem_size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, mem_hnd);
        gl_err = glGetError();
        if (gl_err != GL_NO_ERROR) {
            std::cout << __FUNCTION__ << ": glImportMemoryWin32HandleEXT failed:" << gl_err;
            break;
        }
        glCreateTextures(GL_TEXTURE_2D, 1, &gl_tex);
        if (!gl_tex) {
            std::cout << __FUNCTION__ << ": glCreateTextures failed:" << glGetError();
            break;
        }

        glTextureStorageMem2DEXT(gl_tex, 1, internal_format, width, height, mem, 0);
        gl_err = glGetError();
        if (gl_err != GL_NO_ERROR) {
            std::cout << __FUNCTION__ << ": glTextureStorageMem2DEXT failed:" << gl_err;
            glDeleteTextures(1, &gl_tex);
            gl_tex = 0;
            break;
        }
    } while(0);

    if ((0 == gl_tex) && (0 != mem)) {
        glDeleteMemoryObjectsEXT(1, &mem);
        mem = 0;
    }

    if (0 != gl_tex) {
        g_gl_ext_texs[gl_tex] = mem;
    }
    
    glBindTexture(GL_TEXTURE_2D, gl_tex);
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return gl_tex;
}