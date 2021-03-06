/**
 * Copyright (C) 2015-2018 Think Silicon S.A. (https://think-silicon.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public v3
 * License as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */

/**
 *  @file       shader.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Shader Functionality in GLOVE
 *
 *  @scope
 *
 *  A Shader is a user-defined program designed to run on some stage of a
 *  graphics processor. Its purpose is to execute one of the programmable
 *  stages of the rendering pipeline.
 *
 */

#include "shader.h"

Shader::Shader(const vulkanAPI::vkContext_t *vkContext)
: mVkContext(vkContext), mVkShaderModule(VK_NULL_HANDLE), mShaderCompiler(nullptr), mSource(nullptr),
  mSourceLength(0), mShaderType(SHADER_TYPE_INVALID), mShaderVersion(ESSL_VERSION_100), mCompiled(false)
{
    FUN_ENTRY(GL_LOG_TRACE);
}

Shader::~Shader()
{
    FUN_ENTRY(GL_LOG_TRACE);

    FreeSources();
    DestroyVkShader();
}

int
Shader::GetInfoLogLength(void) const
{
    FUN_ENTRY(GL_LOG_TRACE);

    return mShaderCompiler->GetShaderInfoLog(mShaderType, mShaderVersion) ? (int)strlen(mShaderCompiler->GetShaderInfoLog(mShaderType, mShaderVersion)) : 0;
}

void
Shader::FreeSources(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(mSource) {
        delete[] mSource;
        mSource = nullptr;
        mSourceLength = 0;
    }
}

void
Shader::SetShaderSource(GLsizei count, const GLchar *const *source, const GLint *length)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    FreeSources();
    mCompiled = false;

    if(!source || !count) {
        return;
    }

    uint32_t *sourceLengths = new uint32_t[count];
    mSourceLength = 0;

    /// Count total source length
    for(GLsizei i = 0; i < count; ++i) {
        /// All strings in source are considered to be null terminated
        if(length == nullptr) {
            /// strlen does not include null termination character
            sourceLengths[i] = strlen(source[i]);
        } else {
            if(length[i] < 0) {
                /// NULL terminated again
                sourceLengths[i] = strlen(source[i]);
            } else {
                sourceLengths[i] = length[i];
            }
        }

        mSourceLength += sourceLengths[i];
    }

    if(!mSourceLength) {
        if(sourceLengths) {
            delete[] sourceLengths;
        }
        return;
    }

    mSource = new char[mSourceLength + 1];

    /// Concatenate sources into 1 string and null terminate it
    uint32_t currentLength = 0;
    for(GLsizei i = 0; i < count; ++i) {
        if(sourceLengths[i]) {
            memcpy(static_cast<void *>(&mSource[currentLength]), source[i], sourceLengths[i]);
            currentLength += sourceLengths[i];
        }
    }
    assert(currentLength == mSourceLength);
    mSource[currentLength] = '\0';
    delete[] sourceLengths;

    if(GLOVE_SAVE_SHADER_SOURCES_TO_FILES) {
        mShaderCompiler->EnableSaveSourceToFiles();
    }

    if(GLOVE_DUMP_ORIGINAL_SHADER_SOURCE) {
        GlslPrintShaderSource(mShaderType, mShaderVersion, mSource);
    }
}

int
Shader::GetShaderSourceLength(void) const
{
    FUN_ENTRY(GL_LOG_DEBUG);

    return (mSourceLength > 0) ? mSourceLength+1 : 0;
}

char *
Shader::GetShaderSource(void) const
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(!mSource) {
        return nullptr;
    } else {
        char *source = new char[GetShaderSourceLength()];
        memcpy(source, mSource, GetShaderSourceLength());

        return source;
    }
}

char *
Shader::GetInfoLog(void) const
{
    FUN_ENTRY(GL_LOG_DEBUG);

    char *log = nullptr;

    if(mShaderCompiler) {
        uint32_t len = strlen(mShaderCompiler->GetShaderInfoLog(mShaderType, mShaderVersion)) + 1;
        log = new char[len];

        memcpy(log, mShaderCompiler->GetShaderInfoLog(mShaderType, mShaderVersion), len);

        /// NULL terminate it
        if(log[len - 1] != '\0') {
            log[len] = '\0';
        }
    }

    return log;
}

bool
Shader::CompileShader(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    mCompiled = mShaderCompiler->CompileShader(&mSource, mShaderType, mShaderVersion);

    return mCompiled;
}

void
Shader::DestroyVkShader(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

     if(mVkShaderModule != VK_NULL_HANDLE) {
         vkDestroyShaderModule(mVkContext->vkDevice, mVkShaderModule, nullptr);
         mVkShaderModule = VK_NULL_HANDLE;
     }
}

VkShaderModule
Shader::CreateVkShaderModule(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    assert(mShaderType == SHADER_TYPE_VERTEX || mShaderType == SHADER_TYPE_FRAGMENT);

    DestroyVkShader();

    if(!mSpv.size()) {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = nullptr;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = mSpv.size() * sizeof(uint32_t);
    moduleCreateInfo.pCode = mSpv.data();

    if(vkCreateShaderModule(mVkContext->vkDevice, &moduleCreateInfo, nullptr, &mVkShaderModule)) {
        return VK_NULL_HANDLE;
    }

    return mVkShaderModule;
}
