#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
校园活动管理系统 - 测试服务器
用于测试Qt应用程序的网络功能

启动方法：
    python test_server.py

服务器将在 http://localhost:8080 启动
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
from datetime import datetime
import json

app = Flask(__name__)
CORS(app)  # 允许跨域请求

# 存储同步的活动（用于测试）
synced_activities = []


@app.route('/api/categories', methods=['GET'])
def get_categories():
    """获取活动类别列表"""
    categories = [
        {"name": "学术讲座"},
        {"name": "文体活动"},
        {"name": "社会实践"},
        {"name": "志愿服务"},
        {"name": "竞赛活动"},
        {"name": "其他"}
    ]
    return jsonify(categories)


@app.route('/api/announcements', methods=['GET'])
def get_announcements():
    """获取公告列表"""
    announcements = [
        {
            "title": "欢迎使用活动管理系统",
            "content": "系统已上线，欢迎使用！如有问题请联系管理员。",
            "date": "2024-01-01"
        },
        {
            "title": "活动报名提醒",
            "content": "请及时关注活动信息，及时报名。热门活动名额有限，先到先得。",
            "date": "2024-01-15"
        },
        {
            "title": "签到功能说明",
            "content": "活动开始后，学生可以在报名管理页面进行签到。管理员也可以为学生签到。",
            "date": "2024-01-20"
        }
    ]
    return jsonify(announcements)


@app.route('/api/activities/sync', methods=['POST'])
def sync_activity():
    """同步活动信息到校园平台"""
    try:
        # 获取JSON数据
        data = request.get_json()
        
        if not data:
            return jsonify({
                "success": False,
                "message": "请求数据为空"
            }), 400
        
        # 验证必要字段
        required_fields = ['id', 'title', 'category', 'organizer', 'start_time', 'end_time']
        for field in required_fields:
            if field not in data:
                return jsonify({
                    "success": False,
                    "message": f"缺少必要字段: {field}"
                }), 400
        
        # 保存同步的活动（用于测试）
        activity_info = {
            "id": data.get('id'),
            "title": data.get('title'),
            "description": data.get('description', ''),
            "category": data.get('category'),
            "organizer": data.get('organizer'),
            "start_time": data.get('start_time'),
            "end_time": data.get('end_time'),
            "max_participants": data.get('max_participants', 0),
            "location": data.get('location', ''),
            "status": data.get('status', 1),
            "synced_at": datetime.now().isoformat()
        }
        
        synced_activities.append(activity_info)
        
        # 打印同步信息（用于调试）
        print(f"[同步成功] 活动ID: {activity_info['id']}, 标题: {activity_info['title']}")
        
        return jsonify({
            "success": True,
            "message": "活动同步成功",
            "activity_id": activity_info['id']
        })
        
    except Exception as e:
        print(f"[同步错误] {str(e)}")
        return jsonify({
            "success": False,
            "message": f"同步失败: {str(e)}"
        }), 500


@app.route('/api/synced-activities', methods=['GET'])
def get_synced_activities():
    """获取已同步的活动列表（用于测试和调试）"""
    return jsonify({
        "count": len(synced_activities),
        "activities": synced_activities
    })


@app.route('/api/health', methods=['GET'])
def health_check():
    """健康检查端点"""
    return jsonify({
        "status": "ok",
        "message": "服务器运行正常",
        "timestamp": datetime.now().isoformat()
    })


@app.route('/', methods=['GET'])
def index():
    """根路径，返回API文档"""
    return jsonify({
        "name": "校园活动管理系统 - 测试服务器",
        "version": "1.0",
        "endpoints": {
            "GET /api/categories": "获取活动类别列表",
            "GET /api/announcements": "获取公告列表",
            "POST /api/activities/sync": "同步活动信息",
            "GET /api/synced-activities": "获取已同步的活动（测试用）",
            "GET /api/health": "健康检查"
        },
        "usage": "在Qt应用程序中配置 baseUrl = 'http://localhost:8080/api'"
    })


if __name__ == '__main__':
    print("=" * 60)
    print("校园活动管理系统 - 测试服务器")
    print("=" * 60)
    print("服务器地址: http://localhost:8080")
    print("API基础路径: http://localhost:8080/api")
    print("=" * 60)
    print("\n可用端点:")
    print("  GET  /api/categories          - 获取活动类别")
    print("  GET  /api/announcements        - 获取公告")
    print("  POST /api/activities/sync      - 同步活动")
    print("  GET  /api/synced-activities    - 查看已同步活动")
    print("  GET  /api/health               - 健康检查")
    print("=" * 60)
    print("\n按 Ctrl+C 停止服务器\n")
    
    # 启动服务器
    app.run(
        host='0.0.0.0',  # 允许外部访问
        port=8080,
        debug=True,      # 调试模式
        threaded=True    # 多线程支持
    )

