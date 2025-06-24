#ifndef SIMULATION_DATABASE_H
#define SIMULATION_DATABASE_H

#include <string>
#include <sqlite3.h>
#include "ns3/core-module.h"

using namespace ns3;

class SimulationDatabase
{
public:
  // 构造函数
  SimulationDatabase(const std::string& dbFile);
  // 析构函数
  ~SimulationDatabase();
  
  // 初始化数据库表结构
  bool InitializeDatabase();
  
  // 记录模拟结果
  bool RecordSimulationResult(
    double areaLength,
    double areaWidth,
    double areaHeight,
    uint32_t numNodes,
    const std::string& linkQuality,
    uint32_t simulationRun,
    double completionTime,
    uint32_t totalSentPackets,
    uint32_t totalReceivedPackets,
    double overheadRatio,
    double successRate
  );
  
private:
  // 获取当前时间戳（月-日-时分秒）
  std::string GetCurrentTimestamp() const;
  
  sqlite3* m_db;               // SQLite数据库连接
  std::string m_dbFilename;    // 数据库文件名
};

#endif /* SIMULATION_DATABASE_H */ 