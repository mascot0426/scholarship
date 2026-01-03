#include "csvexporter.h"
#include "database.h"
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QTextCodec>
#include <QDateTime>
#include <QDebug>

CsvExporter::CsvExporter(QObject *parent)
    : QObject(parent)
{
}

QString CsvExporter::escapeCsvField(const QString &field)
{
    // 如果字段包含逗号、引号或换行符，需要用引号括起来，并转义引号
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace("\"", "\"\""); // 转义引号
        return "\"" + escaped + "\"";
    }
    return field;
}

bool CsvExporter::exportRegistrations(const QString &filename, 
                                      const QList<QHash<QString, QVariant>> &registrations,
                                      const QHash<QString, QVariant> &activity)
{
    QFile file(filename);
    // 使用二进制模式打开
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filename;
        return false;
    }
    
    // 获取UTF-8编码器
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    if (!utf8Codec) {
        qDebug() << "UTF-8 codec not available";
        file.close();
        return false;
    }
    
    // 写入UTF-8 BOM
    QByteArray bom;
    bom.append(static_cast<char>(0xEF));
    bom.append(static_cast<char>(0xBB));
    bom.append(static_cast<char>(0xBF));
    file.write(bom);
    
    // 构建CSV内容
    QString content;
    
    // 写入活动信息
    content += "活动信息\n";
    content += "活动标题," + escapeCsvField(activity["title"].toString()) + "\n";
    content += "活动类别," + escapeCsvField(activity["category"].toString()) + "\n";
    content += "发起人," + escapeCsvField(activity["organizer"].toString()) + "\n";
    content += "开始时间," + activity["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm") + "\n";
    content += "结束时间," + activity["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm") + "\n";
    content += "地点," + escapeCsvField(activity["location"].toString()) + "\n";
    content += "最大人数," + activity["max_participants"].toString() + "\n";
    content += "当前人数," + activity["current_participants"].toString() + "\n";
    content += "\n";
    
    // 写入报名列表
    content += "报名名单\n";
    content += "序号,学号,姓名,报名时间,状态\n";
    
    for (int i = 0; i < registrations.size(); ++i) {
        const auto &reg = registrations[i];
        
        QString statusText;
        RegistrationStatus status = static_cast<RegistrationStatus>(reg["status"].toInt());
        switch (status) {
            case RegistrationStatus::Registered: statusText = "已报名"; break;
            case RegistrationStatus::Cancelled: statusText = "已取消"; break;
            case RegistrationStatus::Waitlisted: statusText = "候补"; break;
            case RegistrationStatus::Confirmed: statusText = "已确认"; break;
        }
        
        content += QString::number(i + 1) + ","
            + escapeCsvField(reg["student_id"].toString()) + ","
            + escapeCsvField(reg["student_name"].toString()) + ","
            + reg["registered_at"].toDateTime().toString("yyyy-MM-dd hh:mm") + ","
            + escapeCsvField(statusText) + "\n";
    }
    
    // 将内容转换为UTF-8字节并写入文件
    QByteArray utf8Data = utf8Codec->fromUnicode(content);
    file.write(utf8Data);
    
    file.close();
    return true;
}

bool CsvExporter::exportStatistics(const QString &filename,
                                   const QList<QHash<QString, QVariant>> &statistics)
{
    QFile file(filename);
    // 使用二进制模式打开
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filename;
        return false;
    }
    
    // 获取UTF-8编码器
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    if (!utf8Codec) {
        qDebug() << "UTF-8 codec not available";
        file.close();
        return false;
    }
    
    // 写入UTF-8 BOM
    QByteArray bom;
    bom.append(static_cast<char>(0xEF));
    bom.append(static_cast<char>(0xBB));
    bom.append(static_cast<char>(0xBF));
    file.write(bom);
    
    // 构建CSV内容
    QString content;
    
    // 写入表头
    content += "活动ID,活动标题,类别,发起人,开始时间,结束时间,最大人数,当前人数,候补人数,状态\n";
    
    for (const auto &stat : statistics) {
        QString statusText;
        ActivityStatus status = static_cast<ActivityStatus>(stat["status"].toInt());
        switch (status) {
            case ActivityStatus::Pending: statusText = "待审批"; break;
            case ActivityStatus::Approved: statusText = "已批准"; break;
            case ActivityStatus::Rejected: statusText = "已拒绝"; break;
            case ActivityStatus::Ongoing: statusText = "进行中"; break;
            case ActivityStatus::Finished: statusText = "已结束"; break;
        }
        
        content += stat["id"].toString() + ","
            + escapeCsvField(stat["title"].toString()) + ","
            + escapeCsvField(stat["category"].toString()) + ","
            + escapeCsvField(stat["organizer"].toString()) + ","
            + stat["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm") + ","
            + stat["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm") + ","
            + stat["max_participants"].toString() + ","
            + stat["current_participants"].toString() + ","
            + stat["waitlist_count"].toString() + ","
            + escapeCsvField(statusText) + "\n";
    }
    
    // 将内容转换为UTF-8字节并写入文件
    QByteArray utf8Data = utf8Codec->fromUnicode(content);
    file.write(utf8Data);
    
    file.close();
    return true;
}

