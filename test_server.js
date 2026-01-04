/**
 * 校园活动管理系统 - 测试服务器 (Node.js版本)
 * 用于测试Qt应用程序的网络功能
 * 
 * 启动方法：
 *     node test_server.js
 * 
 * 服务器将在 http://localhost:8080 启动
 */

const express = require('express');
const cors = require('cors');

const app = express();
const PORT = 8080;

// 中间件配置
app.use(cors()); // 允许跨域请求
app.use(express.json()); // 解析JSON请求体

// 存储同步的活动（用于测试）
let syncedActivities = [];

// 获取活动类别列表
app.get('/api/categories', (req, res) => {
    const categories = [
        { name: "学术讲座" },
        { name: "文体活动" },
        { name: "社会实践" },
        { name: "志愿服务" },
        { name: "竞赛活动" },
        { name: "其他" }
    ];
    res.json(categories);
});

// 获取公告列表
app.get('/api/announcements', (req, res) => {
    const announcements = [
        {
            title: "欢迎使用活动管理系统",
            content: "系统已上线，欢迎使用！如有问题请联系管理员。",
            date: "2024-01-01"
        },
        {
            title: "活动报名提醒",
            content: "请及时关注活动信息，及时报名。热门活动名额有限，先到先得。",
            date: "2024-01-15"
        },
        {
            title: "签到功能说明",
            content: "活动开始后，学生可以在报名管理页面进行签到。管理员也可以为学生签到。",
            date: "2024-01-20"
        }
    ];
    res.json(announcements);
});

// 同步活动信息到校园平台
app.post('/api/activities/sync', (req, res) => {
    try {
        const data = req.body;
        
        if (!data) {
            return res.status(400).json({
                success: false,
                message: "请求数据为空"
            });
        }
        
        // 验证必要字段
        const requiredFields = ['id', 'title', 'category', 'organizer', 'start_time', 'end_time'];
        for (const field of requiredFields) {
            if (!data[field]) {
                return res.status(400).json({
                    success: false,
                    message: `缺少必要字段: ${field}`
                });
            }
        }
        
        // 保存同步的活动（用于测试）
        const activityInfo = {
            id: data.id,
            title: data.title,
            description: data.description || '',
            category: data.category,
            organizer: data.organizer,
            start_time: data.start_time,
            end_time: data.end_time,
            max_participants: data.max_participants || 0,
            location: data.location || '',
            status: data.status || 1,
            synced_at: new Date().toISOString()
        };
        
        syncedActivities.push(activityInfo);
        
        // 打印同步信息（用于调试）
        console.log(`[同步成功] 活动ID: ${activityInfo.id}, 标题: ${activityInfo.title}`);
        
        res.json({
            success: true,
            message: "活动同步成功",
            activity_id: activityInfo.id
        });
        
    } catch (error) {
        console.error(`[同步错误] ${error.message}`);
        res.status(500).json({
            success: false,
            message: `同步失败: ${error.message}`
        });
    }
});

// 获取已同步的活动列表（用于测试和调试）
app.get('/api/synced-activities', (req, res) => {
    res.json({
        count: syncedActivities.length,
        activities: syncedActivities
    });
});

// 健康检查端点
app.get('/api/health', (req, res) => {
    res.json({
        status: "ok",
        message: "服务器运行正常",
        timestamp: new Date().toISOString()
    });
});

// 根路径，返回API文档
app.get('/', (req, res) => {
    res.json({
        name: "校园活动管理系统 - 测试服务器",
        version: "1.0",
        endpoints: {
            "GET /api/categories": "获取活动类别列表",
            "GET /api/announcements": "获取公告列表",
            "POST /api/activities/sync": "同步活动信息",
            "GET /api/synced-activities": "获取已同步的活动（测试用）",
            "GET /api/health": "健康检查"
        },
        usage: "在Qt应用程序中配置 baseUrl = 'http://localhost:8080/api'"
    });
});

// 启动服务器
app.listen(PORT, () => {
    console.log("=".repeat(60));
    console.log("校园活动管理系统 - 测试服务器");
    console.log("=".repeat(60));
    console.log(`服务器地址: http://localhost:${PORT}`);
    console.log(`API基础路径: http://localhost:${PORT}/api`);
    console.log("=".repeat(60));
    console.log("\n可用端点:");
    console.log("  GET  /api/categories          - 获取活动类别");
    console.log("  GET  /api/announcements        - 获取公告");
    console.log("  POST /api/activities/sync      - 同步活动");
    console.log("  GET  /api/synced-activities    - 查看已同步活动");
    console.log("  GET  /api/health               - 健康检查");
    console.log("=".repeat(60));
    console.log("\n按 Ctrl+C 停止服务器\n");
});

