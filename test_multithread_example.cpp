/**
 * 多线程测试示例程序
 * 
 * 这是一个简单的测试程序，演示如何测试 ConflictChecker 和 ExportThread
 * 可以直接在Qt项目中运行，或作为参考实现完整的单元测试
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QDateTime>
#include <QTime>
#include "database.h"
#include "conflictchecker.h"
#include "exportthread.h"

class TestWindow : public QMainWindow
{
    Q_OBJECT

public:
    TestWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , database(new Database(this))
        , conflictChecker(nullptr)
        , exportThread(nullptr)
    {
        setWindowTitle("多线程测试工具");
        setMinimumSize(800, 600);
        
        // 初始化数据库
        if (!database->initializeDatabase()) {
            QMessageBox::critical(this, "错误", "数据库初始化失败！");
            return;
        }
        
        setupUI();
        setupTestData();
    }

private slots:
    void testConflictChecker()
    {
        logOutput("=== 开始测试 ConflictChecker ===");
        
        // 创建测试学生和活动
        QString studentId = "test_student_001";
        
        // 创建活动1
        QDateTime startTime1 = QDateTime::currentDateTime().addDays(1);
        QDateTime endTime1 = startTime1.addSecs(7200); // 2小时后
        
        int activityId1 = database->createActivity(
            "测试活动A",                    // title
            "用于测试的活动A",              // description
            "学术",                         // category
            "测试发起人",                   // organizer
            startTime1,                     // startTime
            endTime1,                       // endTime
            50,                             // maxParticipants
            "测试地点"                      // location
        );
        logOutput(QString("创建活动1，ID: %1").arg(activityId1));
        
        // 批准活动1（需要批准后才能报名）
        database->updateActivityStatus(activityId1, ActivityStatus::Approved);
        logOutput("活动1已批准");
        
        // 学生报名活动1
        database->registerActivity(activityId1, studentId, "测试学生");
        logOutput("学生已报名活动1");
        
        // 创建活动2（与活动1时间冲突）
        QDateTime startTime2 = startTime1.addSecs(3600); // 1小时后开始
        QDateTime endTime2 = endTime1.addSecs(3600);     // 1小时后结束
        
        int activityId2 = database->createActivity(
            "测试活动B",                    // title
            "与活动A时间冲突",              // description
            "体育",                         // category
            "测试发起人",                   // organizer
            startTime2,                     // startTime
            endTime2,                       // endTime
            50,                             // maxParticipants
            "测试地点"                      // location
        );
        logOutput(QString("创建活动2，ID: %1（与活动1冲突）").arg(activityId2));
        
        // 批准活动2（冲突检查只检查已批准的活动）
        database->updateActivityStatus(activityId2, ActivityStatus::Approved);
        logOutput("活动2已批准");
        
        // 测试1：检查冲突（应该有冲突）
        logOutput("\n--- 测试1：检查时间冲突 ---");
        conflictChecker = new ConflictChecker(database, this);
        
        connect(conflictChecker, &ConflictChecker::conflictDetected, this, 
                [this](const QList<QHash<QString, QVariant>> &conflicts) {
            logOutput(QString("检测到 %1 个冲突活动：").arg(conflicts.size()));
            for (const auto &conflict : conflicts) {
                logOutput(QString("  - %1 (%2 - %3)")
                    .arg(conflict["title"].toString())
                    .arg(conflict["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm"))
                    .arg(conflict["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
            }
        });
        
        connect(conflictChecker, &ConflictChecker::checkCompleted, this,
                [this](bool hasConflict) {
            logOutput(QString("冲突检查完成，结果：%1").arg(hasConflict ? "有冲突" : "无冲突"));
            logOutput("UI响应性测试：检查过程中可以点击其他按钮，UI应该保持响应");
        });
        
        conflictChecker->setStudentId(studentId);
        conflictChecker->setActivityTime(activityId2, startTime2, endTime2);
        
        logOutput("启动冲突检查线程...");
        QTime timer;
        timer.start();
        conflictChecker->start();
        
        // 验证线程启动不阻塞
        int elapsed = timer.elapsed();
        logOutput(QString("线程启动耗时：%1 ms（应该 < 100ms）").arg(elapsed));
        
        if (elapsed < 100) {
            logOutput("✓ UI响应性测试通过");
        } else {
            logOutput("✗ UI响应性测试失败");
        }
        
        // 测试2：检查无冲突情况
        logOutput("\n--- 测试2：检查无冲突情况 ---");
        ConflictChecker *checker2 = new ConflictChecker(database, this);
        
        connect(checker2, &ConflictChecker::checkCompleted, this,
                [this](bool hasConflict) {
            if (!hasConflict) {
                logOutput("✓ 无冲突检测正确");
            } else {
                logOutput("✗ 无冲突检测失败");
            }
        });
        
        // 创建一个不冲突的活动
        QDateTime noConflictStart = QDateTime::currentDateTime().addDays(2);
        QDateTime noConflictEnd = noConflictStart.addSecs(7200);
        
        checker2->setStudentId(studentId);
        checker2->setActivityTime(999, noConflictStart, noConflictEnd);
        checker2->start();
        
        logOutput("=== ConflictChecker 测试完成 ===\n");
    }
    
    void testExportThread()
    {
        logOutput("=== 开始测试 ExportThread ===");
        
        // 准备测试数据
        QList<QHash<QString, QVariant>> statistics;
        for (int i = 0; i < 50; ++i) {
            QHash<QString, QVariant> stat;
            stat["id"] = i + 1;
            stat["title"] = QString("测试活动%1").arg(i + 1);
            stat["category"] = "测试类别";
            stat["organizer"] = "测试发起人";
            stat["current_participants"] = 10 + i;
            stat["max_participants"] = 50;
            stat["status"] = 1;
            statistics.append(stat);
        }
        
        logOutput(QString("准备导出 %1 条统计数据").arg(statistics.size()));
        
        // 选择保存位置
        QString filename = QFileDialog::getSaveFileName(this, "保存测试导出文件",
            "test_export.csv", "CSV Files (*.csv)");
        
        if (filename.isEmpty()) {
            logOutput("用户取消了文件选择");
            return;
        }
        
        logOutput(QString("导出文件：%1").arg(filename));
        
        // 创建导出线程
        exportThread = new ExportThread(this);
        
        // 连接进度信号
        connect(exportThread, &ExportThread::exportProgress, this,
                [this](int percentage) {
            progressBar->setValue(percentage);
            logOutput(QString("导出进度：%1%").arg(percentage));
        });
        
        // 连接完成信号
        connect(exportThread, &ExportThread::exportFinished, this,
                [this, filename](bool success, const QString &message) {
            if (success) {
                logOutput(QString("✓ 导出成功：%1").arg(message));
                logOutput(QString("文件已保存到：%1").arg(filename));
                
                // 验证文件存在
                if (QFile::exists(filename)) {
                    QFileInfo fileInfo(filename);
                    logOutput(QString("文件大小：%1 字节").arg(fileInfo.size()));
                    logOutput("✓ 文件验证通过");
                } else {
                    logOutput("✗ 文件验证失败：文件不存在");
                }
            } else {
                logOutput(QString("✗ 导出失败：%1").arg(message));
            }
            logOutput("=== ExportThread 测试完成 ===\n");
        });
        
        // 连接错误信号
        connect(exportThread, &ExportThread::exportError, this,
                [this](const QString &error) {
            logOutput(QString("✗ 导出错误：%1").arg(error));
        });
        
        // 设置导出参数
        exportThread->setExportType("statistics");
        exportThread->setFilename(filename);
        exportThread->setStatisticsData(statistics);
        
        // 测试UI响应性
        logOutput("启动导出线程...");
        QTime timer;
        timer.start();
        exportThread->start();
        
        int elapsed = timer.elapsed();
        logOutput(QString("线程启动耗时：%1 ms（应该 < 100ms）").arg(elapsed));
        
        if (elapsed < 100) {
            logOutput("✓ UI响应性测试通过");
            logOutput("提示：在导出过程中，可以点击其他按钮测试UI响应性");
        } else {
            logOutput("✗ UI响应性测试失败");
        }
    }
    
    void testUIResponsiveness()
    {
        static int clickCount = 0;
        clickCount++;
        logOutput(QString("UI响应性测试按钮被点击（第 %1 次）").arg(clickCount));
        logOutput("如果能看到这条消息，说明UI在后台操作时仍然保持响应");
    }

private:
    void setupUI()
    {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        
        // 标题
        QLabel *titleLabel = new QLabel("多线程功能测试工具", this);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
        layout->addWidget(titleLabel);
        
        // 按钮区域
        QPushButton *testConflictBtn = new QPushButton("测试 ConflictChecker", this);
        connect(testConflictBtn, &QPushButton::clicked, this, &TestWindow::testConflictChecker);
        layout->addWidget(testConflictBtn);
        
        QPushButton *testExportBtn = new QPushButton("测试 ExportThread", this);
        connect(testExportBtn, &QPushButton::clicked, this, &TestWindow::testExportThread);
        layout->addWidget(testExportBtn);
        
        QPushButton *testUIBtn = new QPushButton("测试UI响应性（在后台操作时点击）", this);
        connect(testUIBtn, &QPushButton::clicked, this, &TestWindow::testUIResponsiveness);
        layout->addWidget(testUIBtn);
        
        // 进度条
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        layout->addWidget(progressBar);
        
        // 日志输出
        logTextEdit = new QTextEdit(this);
        logTextEdit->setReadOnly(true);
        layout->addWidget(logTextEdit);
        
        logOutput("测试工具已就绪");
        logOutput("请点击上方按钮开始测试");
    }
    
    void setupTestData()
    {
        // 可以在这里准备一些测试数据
        logOutput("数据库已初始化");
    }
    
    void logOutput(const QString &message)
    {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        logTextEdit->append(QString("[%1] %2").arg(timestamp).arg(message));
    }
    
    Database *database;
    ConflictChecker *conflictChecker;
    ExportThread *exportThread;
    QTextEdit *logTextEdit;
    QProgressBar *progressBar;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    TestWindow window;
    window.show();
    
    return app.exec();
}

#include "test_multithread_example.moc"

