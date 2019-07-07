//
//  VulkanModel.hpp
//  VulkanGfx
//
//  Created by Michael Mackie on 9/2/18.
//  Copyright © 2018 Michael Mackie. All rights reserved.
//

#ifndef VulkanModel_hpp
#define VulkanModel_hpp

#include "VulkanCommon.hpp"
#include "IModel.hpp"

class VulkanModel : public IModel
{
    typedef std::vector<PositionColorVertex> VertexList;
    typedef std::vector<uint32_t> IndexList;
public:
    VulkanModel(const char* aModelFile);
    ~VulkanModel();
    
    virtual bool Load();
    
    void Draw(VkCommandBuffer& aCmdBuffer);

private:
    bool CreateModelFromFile(VertexList& outVertecies, IndexList& outIndices);
    bool CreateVertexBuffer(const VertexList& outVertecies);
    bool CreateIndexBuffer(const IndexList& outIndices);

    uint32_t                            m_ModelIndexCount;
    VkBuffer                            m_ModelVertexBuffer;
    VkBuffer                            m_ModelIndexBuffer;
    VkDeviceMemory                      m_ModelVertexBufferMemory;
    VkDeviceMemory                      m_ModelIndexBufferMemory;
};

#endif /* VulkanModel_hpp */
