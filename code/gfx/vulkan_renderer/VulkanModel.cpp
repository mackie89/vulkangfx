//
//  VulkanModel.cpp
//  VulkanGfx
//
//  Created by Michael Mackie on 9/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#include "VulkanModel.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanUtils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "obj_loader.h"
#undef TINYOBJLOADER_IMPLEMENTATION

VulkanModel::VulkanModel(const char* aModelFile)
: IModel(aModelFile)
{
}

VulkanModel::~VulkanModel()
{
    VulkanRenderer* renderer = VulkanRenderer::GetInstance();
    VkDevice& aDevice = renderer->GetLogicalDevice();
    
    vkDestroyBuffer(aDevice, m_ModelIndexBuffer, nullptr);
    vkFreeMemory(aDevice, m_ModelIndexBufferMemory, nullptr);
    
    vkDestroyBuffer(aDevice, m_ModelVertexBuffer, nullptr);
    vkFreeMemory(aDevice, m_ModelVertexBufferMemory, nullptr);
}

bool VulkanModel::Load()
{
    VertexList modelVertices;
    IndexList modelIndices;
    
    bool loaded = CreateModelFromFile(modelVertices, modelIndices);
        
    if(loaded)
        loaded &= CreateVertexBuffer(modelVertices);
    
    if(loaded)
        loaded &= CreateIndexBuffer(modelIndices);
    
    return loaded;
}

void VulkanModel::Draw(VkCommandBuffer& aCmdBuffer)
{
    VkBuffer vertexBuffers[] = {m_ModelVertexBuffer};
    VkDeviceSize offsets[] = {0};
    
    vkCmdBindVertexBuffers(aCmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(aCmdBuffer, m_ModelIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(aCmdBuffer, m_ModelIndexCount, 1, 0, 0, 0);
}

bool VulkanModel::CreateModelFromFile(VertexList& outVertecies, IndexList& outIndices)
{
    SCOPE_FUNCTION_MILLI();
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_ModelFile.c_str()))
        return false;
    
    std::unordered_map<PositionColorVertex, uint32_t> uniqueVertices = {};
    
    for (const tinyobj::shape_t& shape : shapes)
    {
        for (const tinyobj::index_t& currentIndex : shape.mesh.indices)
        {
            PositionColorVertex vertex = {};
            
            const int vertIndex = 3 * currentIndex.vertex_index;
            const int uvIndex = 2 * currentIndex.texcoord_index;
            
            vertex.m_Pos =
            {
                attrib.vertices[vertIndex],
                attrib.vertices[vertIndex + 1],
                attrib.vertices[vertIndex + 2]
            };
            
            //obj expect bottom-left / vulkan expects top-left
            vertex.m_UV =
            {
                attrib.texcoords[uvIndex],
                1.0f - attrib.texcoords[uvIndex + 1]
            };
            
            vertex.m_Color = {1.0f, 1.0f, 1.0f};
            
            uint32_t index = 0;
            std::unordered_map<PositionColorVertex, uint32_t>::const_iterator vertItr = uniqueVertices.find(vertex);
            
            if (vertItr == uniqueVertices.end())
            {
                index = static_cast<uint32_t>(outVertecies.size());
                uniqueVertices[vertex] = index;
                outVertecies.push_back(vertex);
            }
            else
            {
                index = vertItr->second;
            }
            
            outIndices.push_back(index);
        }
    }
    
    return true;
}

bool VulkanModel::CreateVertexBuffer(const VertexList& outVertecies)
{
    VulkanRenderer* renderer = VulkanRenderer::GetInstance();
    VkDevice& aDevice = renderer->GetLogicalDevice();
    
    const VkDeviceSize size = sizeof(PositionColorVertex) * outVertecies.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!VulkanUtils::CreateBuffer(size, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }
    
    void* data;
    vkMapMemory(aDevice, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, outVertecies.data(), size);
    vkUnmapMemory(aDevice, stagingBufferMemory);
    
    // create device local buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if(!VulkanUtils::CreateBuffer(size, usage, properties, m_ModelVertexBuffer, m_ModelVertexBufferMemory))
            return false;
    }
    
    VulkanUtils::CopyBuffer(stagingBuffer, m_ModelVertexBuffer, size);
    
    vkDestroyBuffer(aDevice, stagingBuffer, nullptr);
    vkFreeMemory(aDevice, stagingBufferMemory, nullptr);
    
    return true;
}

bool VulkanModel::CreateIndexBuffer(const IndexList& outIndices)
{
    VulkanRenderer* renderer = VulkanRenderer::GetInstance();
    VkDevice& aDevice = renderer->GetLogicalDevice();
    
    const VkDeviceSize bufferSize = sizeof(outIndices[0]) * outIndices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // create staging buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        if(!VulkanUtils::CreateBuffer(bufferSize, usage, properties, stagingBuffer, stagingBufferMemory))
            return false;
    }
    
    void* data;
    vkMapMemory(aDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, outIndices.data(), (size_t)bufferSize);
    vkUnmapMemory(aDevice, stagingBufferMemory);
    
    // create device local buffer
    {
        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        if(!VulkanUtils::CreateBuffer(bufferSize, usage, properties, m_ModelIndexBuffer, m_ModelIndexBufferMemory))
            return false;
    }
    
    VulkanUtils::CopyBuffer(stagingBuffer, m_ModelIndexBuffer, bufferSize);
    
    vkDestroyBuffer(aDevice, stagingBuffer, nullptr);
    vkFreeMemory(aDevice, stagingBufferMemory, nullptr);
    
    m_ModelIndexCount = static_cast<uint32_t>(outIndices.size());
    
    return true;
}
