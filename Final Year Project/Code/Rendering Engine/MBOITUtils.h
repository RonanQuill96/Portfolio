#pragma once

#include <glm/glm.hpp>

struct MBOITPushConstants
{
    float lnDepthMin;
    float lnDepthMax;
    float pad;
    float pad1;

    glm::ivec3 clusterCounts;
    int clusterSizeX;

    float zNear;
    float zFar;
    float scale;
    float bias;
};

struct MBOITTransmittanceConstants
{
    float moment_bias;
    float overestimationx;
    float screenWidth;
    float screenHeight;
};

struct MBOITTransmittanceClusteredConstants
{
    float moment_bias;
    float overestimation;
    float screenWidth;
    float screenHeight;

    glm::ivec3 clusterCounts;
    int clusterSizeX;

    float zNear;
    float zFar;
    float scale;
    float bias;

    bool enableEnviromentLight;
};

struct MBOITPerFrameInfo
{
    glm::vec3 cameraPosition;
    float pad;

    float lnDepthMin;
    float lnDepthMax;
    float tilesNumX;
    float tilesNumY;
};