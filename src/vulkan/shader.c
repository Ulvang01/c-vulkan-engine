#include "shader.h"
#include "core/log.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Reads an entire binary file into a newly allocated buffer.
 *
 * Caller owns the returned buffer and must free it.
 *
 * @param filepath Path to the file.
 * @param size     Out-parameter, receives the file size in bytes.
 * @return Pointer to the file contents, or NULL on failure.
 */
static char *readFile(const char *filepath, size_t *size)
{
    // "rb" matters on Windows, without it the CRT translates line endings
    // and corrupts binary data
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s", filepath);
        return NULL;
    }

    // Seek to the end to learn the size, then rewind, this is the C
    // equivalent of C++'s std::ios::ate trick
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0)
    {
        LOG_ERROR("File is empty or unreadable: %s", filepath);
        fclose(file);
        return NULL;
    }

    char *buffer = malloc(fileSize);
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);

    if (bytesRead != (size_t)fileSize)
    {
        LOG_ERROR("Short read on %s (%zu of %ld bytes)", filepath, bytesRead, fileSize);
        free(buffer);
        return NULL;
    }

    *size = (size_t)fileSize;
    return buffer;
}

int createShaderModule(VkDevice device, const char *filepath, VkShaderModule *module)
{
    size_t codeSize = 0;
    char *code = readFile(filepath, &codeSize);
    if (!code)
    {
        return 1;
    }

    VkShaderModuleCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    // SPIR-V is a stream of 32-bit words, so Vulkan wants a uint32_t*.
    // malloc's return is guaranteed suitably aligned for this, so the
    // cast is safe here, it would not be if this pointed into the middle
    // of some larger buffer.
    createInfo.pCode = (const uint32_t *)code;

    VkResult result = vkCreateShaderModule(device, &createInfo, NULL, module);
    free(code);

    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create shader module from %s: %d", filepath, result);
        return 1;
    }

    LOG_DEBUG("Loaded shader module: %s (%zu bytes)", filepath, codeSize);
    return 0;
}