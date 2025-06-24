/*
 * KeyMatrix.cc
 *
 *  Created on: 2025年5月23日
 *      Author: Zhang Zhan
 */

#include "KeyMatrix.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstdlib> // 添加这个头文件以支持rand()函数
#include <stdint.h>
#include <cmath>

// 生成(0,1)之间的随机数
double KeyMatrix::RandomVariable() const
{ 
    // 生成(0,1)之间的随机数
    double random = static_cast<double>(rand()) / RAND_MAX ;
    return random;
}

// 默认构造函数
KeyMatrix::KeyMatrix() : m_networkSize(0), m_nodeId(0) {
}

// 构造函数,具体的初始化。
KeyMatrix::KeyMatrix(uint32_t networkSize, uint32_t nodeId) : m_networkSize(networkSize), m_nodeId(nodeId) {
  // 直接在构造函数中初始化矩阵
  m_matrix.resize(m_networkSize);
  for (uint32_t i = 0; i < m_networkSize; i++) {
    m_matrix[i].resize(m_networkSize);
    for (uint32_t j = 0; j < m_networkSize; j++) {
      m_matrix[i][j] = (i == j);
    } 
  }   
}

// 析构函数实现
KeyMatrix::~KeyMatrix() {
}

// 初始化矩阵
void KeyMatrix::InitializeMatrix(uint32_t networkSize, uint32_t nodeId) {
  m_networkSize = networkSize;
  m_nodeId = nodeId;
  m_matrix.resize(m_networkSize);
  for (uint32_t i = 0; i < m_networkSize; i++) {
    m_matrix[i].resize(m_networkSize);
    for (uint32_t j = 0; j < m_networkSize; j++) {
      m_matrix[i][j] = (i == j);
    } 
  }   
}

// 判断某节点是否拥有某密钥贡献
bool KeyMatrix::HasKeyContribution(uint32_t AnyNodeId_i, uint32_t AnyNodeId_j) const
{   
  return m_matrix[AnyNodeId_i][AnyNodeId_j];
}   

// 接收密钥贡献
void KeyMatrix::ReceiveKeyContribution(uint32_t contributorId)
{
    m_matrix[m_nodeId][contributorId] = true;
}

// 检查KeyMatrix是否全为1
bool KeyMatrix::IsFull1() const
{
  for (uint32_t i = 0; i < m_networkSize; i++) {
    for (uint32_t j = 0; j < m_networkSize; j++) {
      if (m_matrix[i][j] != true) {
        return false;   
      }
    }
  }
  return true;
}   

// 合并收到的矩阵到本地矩阵，ReceivedMatrix也是KeyMatrix
void KeyMatrix::MergeMatrix(const KeyMatrix& ReceivedMatrix)
{
  // 遍历接收到的矩阵的每一行
  for (uint32_t i = 0; i < m_networkSize; i++) {
    // 遍历接收到的矩阵的每一列
    for (uint32_t j = 0; j < m_networkSize; j++) {
      // 如果接收到的矩阵的元素为true，则合并到本地矩阵
      m_matrix[i][j] = m_matrix[i][j] || ReceivedMatrix.m_matrix[i][j];
    }
  }
}

// 计算补充率(CR)  
double KeyMatrix::CalculateCR(uint32_t NeighborId) const
{
  uint32_t diffCount = 0;  // 差集大小
  uint32_t unionCount = 0; // 并集大小

  // 计算差集大小 - 仅在本地节点中存在的密钥贡献, 邻居节点中不存在的密钥贡献
  for (uint32_t j = 0; j < m_networkSize; j++) {
    if (m_matrix[m_nodeId][j] && !m_matrix[NeighborId][j]) {
      diffCount++;
    }
    // 计算并集大小 - 至少在一个节点中存在的密钥贡献
    if (m_matrix[m_nodeId][j] || m_matrix[NeighborId][j]) {
      unionCount++;
    }
  }
  
  // 返回差集与并集的比值作为补充率
  return static_cast<double>(diffCount) / unionCount;
}

// 计算转发度(FD)   
double KeyMatrix::CalculateFD(uint32_t ContributorId) const
{
  uint32_t receivedCount = 0; // 已接收到贡献的节点数  
  // 计算已接收到该贡献者密钥的节点数量,即对应列中为1的节点数量
  for (uint32_t i = 0; i < m_networkSize; i++) {
    if (m_matrix[i][ContributorId]) {
      receivedCount++;
    }
  }
  // 返回已接收节点数与总节点数的比值作为转发度
  return static_cast<double>(receivedCount) / m_networkSize;
}

// 检查自己是否拥有所有密钥贡献
bool KeyMatrix::SelfIsFull1() const
{
  for (uint32_t i = 0; i < m_networkSize; i++) {
    if (!m_matrix[m_nodeId][i]) {
      return false;
    }
  }
  return true;
}

std::string KeyMatrix::GetForwardingContributions(uint32_t NeighborId) const
{
  // 初始化一个m_networkSize大小的bit串，每一位的0和1代表本次消息中是否拥有该密钥贡献
  std::string forwardingContributions(m_networkSize, '0');

  // 如果自己拥有所有密钥贡献，则全部转发
  if (SelfIsFull1()) {
    for (uint32_t i = 0; i < m_networkSize; i++) {
      forwardingContributions[i] = '1';
    }
  }  
  else {

  double cr = std::max(CalculateCR(NeighborId), 0.8);
  for (uint32_t j = 0; j < m_networkSize; j++) {
    if (m_matrix[m_nodeId][j] && !m_matrix[NeighborId][j] && RandomVariable() < cr) {
        forwardingContributions[j] = '1';
      }
    }
  }
  return forwardingContributions;
}


// 将矩阵转换为字符串
std::string KeyMatrix::MatrixToString() const {
  std::string matrixString;
  for (uint32_t i = 0; i < m_networkSize; i++) {
    for (uint32_t j = 0; j < m_networkSize; j++) {
      // 使用std::ostringstream替代std::to_string
      std::ostringstream ss;
      ss << (m_matrix[i][j] ? 1 : 0);
      matrixString += ss.str();
    }
  }
  return matrixString;
}

// 将字符串转换为矩阵
KeyMatrix KeyMatrix::StringToMatrix(const std::string& matrixString) const {
  KeyMatrix result;
  result.InitializeMatrix(m_networkSize, m_nodeId);
  
  for (uint32_t i = 0; i < m_networkSize; i++) {
    for (uint32_t j = 0; j < m_networkSize; j++) {
      if (matrixString[i * m_networkSize + j] == '1') {
        result.m_matrix[i][j] = true;
      } else {
        result.m_matrix[i][j] = false;
      }
    }
  }
  return result;
}
