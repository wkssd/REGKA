/*
 * KeyMatrix.h
 *
 *  Created on: 2025年5月23日
 *      Author: Zhang Zhan
 */

#ifndef KEY_MATRIX_H
#define KEY_MATRIX_H

#include <vector>
#include <set>
#include <string>
#include <stdint.h>

class KeyMatrix
{
public:
  // 默认构造函数
  KeyMatrix();
  // 构造函数
  KeyMatrix(uint32_t networkSize, uint32_t nodeId);
  // 析构函数
  ~KeyMatrix();
  
  // 初始化矩阵
  void InitializeMatrix(uint32_t networkSize, uint32_t nodeId);

  // 判断某节点是否拥有某密钥贡献
  bool HasKeyContribution(uint32_t AnyNodeId_i, uint32_t AnyNodeId_j) const;

  // 接收来自另一个节点的密钥贡献
  void ReceiveKeyContribution(uint32_t contributorId);

  // 合并另一个节点的矩阵信息
  void MergeMatrix(const KeyMatrix& ReceivedMatrix);

  // 计算与另一个节点的补充率
  double CalculateCR(uint32_t NeighborId) const;
  // 计算某个密钥贡献的转发度
  double CalculateFD(uint32_t ContributorId) const;
  // 随机变量生成函数
  double RandomVariable() const;
  // 获取需要转发的密钥贡献集合
  std::string GetForwardingContributions(uint32_t NeighborId) const;

  bool IsFull1() const;
  // 检查自己是否拥有所有密钥贡献
  bool SelfIsFull1() const;
  // 将矩阵转换为字符串
  std::string MatrixToString() const;
  // 将字符串转换为矩阵
  KeyMatrix StringToMatrix(const std::string& matrixString) const;


private:
  std::vector<std::vector<bool> > m_matrix; ///< 密钥贡献矩阵,是一个二维数组,m_matrix[i][j]表示节点i是否拥有节点j的密钥贡献
  uint32_t m_networkSize;                  ///< 网络节点数量
  uint32_t m_nodeId;                       ///< 当前节点ID
};

#endif /* KEY_MATRIX_H */ 