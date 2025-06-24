#include "SimulationDatabase.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>  
#include <errno.h>  
#include <unistd.h> 
// 数据库路径
std::string dbPath = "/home/z/ns-allinone-3.25/ns-3.25/scratch/REGKA-Ours/DB/";

SimulationDatabase::SimulationDatabase(const std::string& dbFile)
  : m_db(NULL)
{
  // 如果dbFile已经包含完整路径，直接使用
  if (dbFile.find('/') != std::string::npos) {
    m_dbFilename = dbFile;
  } else {
    // 否则，将文件名添加到dbPath
    m_dbFilename = dbPath + dbFile;
  }

  // 确保数据库目录存在
  struct stat st;
  if (stat(dbPath.c_str(), &st) != 0) {
    // 创建目录，权限为可读可写可执行
    if (mkdir(dbPath.c_str(), 0777) != 0) {
      std::cerr << "无法创建数据库目录: " << dbPath << " 错误: " << strerror(errno) << std::endl;
      return;
    }
    std::cerr << "创建数据库目录: " << dbPath << std::endl;
  }

  // 检查目录权限
  if (access(dbPath.c_str(), W_OK) != 0) {
    std::cerr << "数据库目录没有写入权限: " << dbPath << " 错误: " << strerror(errno) << std::endl;
    return;
  }

  std::cerr << "尝试创建数据库文件: " << m_dbFilename << std::endl;

  // 打开或创建数据库
  int rc = sqlite3_open(m_dbFilename.c_str(), &m_db);
  if (rc) {
    std::cerr << "SQLite错误代码: " << rc << std::endl;
    std::cerr << "SQLite错误信息: " << sqlite3_errmsg(m_db) << std::endl;
    
    // 如果数据库文件不存在，则创建数据库文件
    if (rc == SQLITE_CANTOPEN) {
      std::cerr << "数据库文件不存在，尝试创建数据库文件" << std::endl;
      
      // 确保父目录存在
      std::string::size_type pos = m_dbFilename.find_last_of('/');
      if (pos != std::string::npos) {
        std::string dir = m_dbFilename.substr(0, pos);
        if (stat(dir.c_str(), &st) != 0) {
          if (mkdir(dir.c_str(), 0777) != 0) {
            std::cerr << "无法创建数据库文件目录: " << dir << " 错误: " << strerror(errno) << std::endl;
            return;
          }
        }
      }

      // 再次尝试打开数据库
      rc = sqlite3_open(m_dbFilename.c_str(), &m_db);
      if (rc) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(m_db) << std::endl;
        std::cerr << "错误代码: " << rc << std::endl;
        std::cerr << "系统错误: " << strerror(errno) << std::endl;
        sqlite3_close(m_db);
        m_db = NULL;
        return;
      }
    }
  }

  if (m_db) {
    std::cerr << "成功打开数据库文件: " << m_dbFilename << std::endl;
  }
}

SimulationDatabase::~SimulationDatabase()
{
  if (m_db) {
    sqlite3_close(m_db);
    m_db = NULL;
  }
}

// 获取当前时间戳（月-日-时分秒）
std::string SimulationDatabase::GetCurrentTimestamp() const
{
  std::time_t now = std::time(NULL);
  std::tm* timeinfo = std::localtime(&now);
  
  std::ostringstream oss;
  oss << std::setfill('0') 
      << std::setw(2) << timeinfo->tm_mon + 1 << "-"
      << std::setw(2) << timeinfo->tm_mday << "-"
      << std::setw(2) << timeinfo->tm_hour
      << std::setw(2) << timeinfo->tm_min
      << std::setw(2) << timeinfo->tm_sec;
  
  return oss.str();
}

bool SimulationDatabase::InitializeDatabase()
{
  if (!m_db) {
    std::cerr << "数据库未打开" << std::endl;
    return false;
  }
  
  char* errMsg = NULL;
  std::string sql = 
    "CREATE TABLE IF NOT EXISTS SimulationResults ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  timestamp TEXT,"
    "  areaLength REAL,"
    "  areaWidth REAL,"
    "  areaHeight REAL,"
    "  numNodes INTEGER,"
    "  linkQuality TEXT,"
    "  simulationRun INTEGER,"
    "  completionTime REAL,"
    "  totalSentPackets INTEGER,"
    "  totalReceivedPackets INTEGER,"
    "  overheadRatio REAL,"
    "  successRate REAL"
    ");";
  
  int rc = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errMsg);
  
  if (rc != SQLITE_OK) {
    std::cerr << "SQL错误: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }
  
  return true;
}

bool SimulationDatabase::RecordSimulationResult(
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
  double successRate)
{
  if (!m_db) {
    std::cerr << "数据库未打开" << std::endl;
    return false;
  }
  
  // 获取当前时间戳
  std::string timestamp = GetCurrentTimestamp();
  
  // 开始事务
  char* errMsg = NULL;
  if (sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, &errMsg) != SQLITE_OK) {
    std::cerr << "开始事务失败: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }
  
  // 插入模拟结果
  std::stringstream ss;
  ss << "INSERT INTO SimulationResults "
     << "(timestamp, areaLength, areaWidth, areaHeight, numNodes, linkQuality, simulationRun, completionTime, totalSentPackets, totalReceivedPackets, overheadRatio, successRate) "
     << "VALUES ('" << timestamp << "', " 
     << areaLength << ", " << areaWidth << ", " << areaHeight << ", "
     << numNodes << ", '" << linkQuality << "', " << simulationRun << ", " 
     << completionTime << ", " << totalSentPackets << ", " << totalReceivedPackets << ", "
     << overheadRatio << ", " << successRate << ");";
  
  std::string sql = ss.str();
  int rc = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errMsg);
  
  if (rc != SQLITE_OK) {
    std::cerr << "插入模拟结果失败: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    sqlite3_exec(m_db, "ROLLBACK", NULL, NULL, NULL);
    return false;
  }
  
  // 提交事务
  if (sqlite3_exec(m_db, "COMMIT", NULL, NULL, &errMsg) != SQLITE_OK) {
    std::cerr << "提交事务失败: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    sqlite3_exec(m_db, "ROLLBACK", NULL, NULL, NULL);
    return false;
  }
  
  return true;
} 