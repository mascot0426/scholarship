# 测试服务器API端点说明

## 问题解答

### 为什么访问 `http://localhost:8080/api/sync` 显示 "Cannot GET /api/sync"？

**原因**：
1. **路径错误**：正确的端点是 `/api/activities/sync`，不是 `/api/sync`
2. **方法错误**：该端点只支持 **POST** 方法，不支持 **GET** 方法

## 正确的API端点

### 1. 同步活动信息（POST）

- **URL**: `POST http://localhost:8080/api/activities/sync`
- **方法**: POST（必须）
- **Content-Type**: `application/json`
- **功能**: 同步活动信息到校园平台

**不能在浏览器中直接访问**，因为浏览器默认使用GET方法。

### 2. 查看已同步的活动（GET）

- **URL**: `GET http://localhost:8080/api/synced-activities`
- **方法**: GET
- **功能**: 查看所有已同步的活动列表

**可以在浏览器中直接访问**！

## 测试方法

### 方法1：在浏览器中查看已同步的活动

直接在浏览器地址栏输入：
```
http://localhost:8080/api/synced-activities
```

应该看到类似这样的JSON响应：
```json
{
  "count": 2,
  "activities": [
    {
      "id": 2,
      "title": "123",
      "category": "11",
      "organizer": "admin",
      "synced_at": "2024-01-20T10:30:00.000Z"
    }
  ]
}
```

### 方法2：使用浏览器控制台测试POST请求

1. 打开浏览器（Chrome/Firefox）
2. 按 **F12** 打开开发者工具
3. 切换到 **Console（控制台）** 标签
4. 粘贴以下代码并回车：

```javascript
fetch('http://localhost:8080/api/activities/sync', {
    method: 'POST',
    headers: {
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({
        id: 999,
        title: "测试活动",
        description: "这是一个测试活动",
        category: "学术讲座",
        organizer: "测试发起人",
        start_time: "2024-02-01T10:00:00",
        end_time: "2024-02-01T12:00:00",
        max_participants: 100,
        location: "测试地点",
        status: 1
    })
})
.then(res => res.json())
.then(data => {
    console.log('✅ 响应:', data);
    if (data.success) {
        console.log('✅ 同步成功！');
    } else {
        console.log('❌ 同步失败:', data.message);
    }
})
.catch(error => {
    console.error('❌ 请求失败:', error);
});
```

### 方法3：使用curl命令（命令行）

**Windows PowerShell**:
```powershell
Invoke-RestMethod -Uri "http://localhost:8080/api/activities/sync" `
    -Method POST `
    -ContentType "application/json" `
    -Body '{"id":999,"title":"测试活动","category":"学术讲座","organizer":"测试发起人","start_time":"2024-02-01T10:00:00","end_time":"2024-02-01T12:00:00","max_participants":100,"location":"测试地点","status":1}'
```

**Linux/Mac**:
```bash
curl -X POST http://localhost:8080/api/activities/sync \
  -H "Content-Type: application/json" \
  -d '{
    "id": 999,
    "title": "测试活动",
    "description": "这是一个测试活动",
    "category": "学术讲座",
    "organizer": "测试发起人",
    "start_time": "2024-02-01T10:00:00",
    "end_time": "2024-02-01T12:00:00",
    "max_participants": 100,
    "location": "测试地点",
    "status": 1
  }'
```

### 方法4：使用Postman或其他API测试工具

1. 下载并安装 [Postman](https://www.postman.com/)
2. 创建新请求
3. 设置：
   - **Method**: POST
   - **URL**: `http://localhost:8080/api/activities/sync`
   - **Headers**: `Content-Type: application/json`
   - **Body**: 选择 "raw" 和 "JSON"，然后输入JSON数据

## 所有可用的API端点

### GET端点（可在浏览器中直接访问）

| 端点 | 说明 | 示例URL |
|------|------|---------|
| `/api/categories` | 获取活动类别 | http://localhost:8080/api/categories |
| `/api/announcements` | 获取公告 | http://localhost:8080/api/announcements |
| `/api/synced-activities` | 查看已同步活动 | http://localhost:8080/api/synced-activities |
| `/api/health` | 健康检查 | http://localhost:8080/api/health |
| `/` | API文档 | http://localhost:8080/ |

### POST端点（不能在浏览器中直接访问）

| 端点 | 说明 | 方法 |
|------|------|------|
| `/api/activities/sync` | 同步活动信息 | POST |

## 常见错误

### 错误1: "Cannot GET /api/sync"

**原因**: 
- 路径错误：应该是 `/api/activities/sync`
- 方法错误：应该使用POST，不是GET

**解决**: 使用正确的端点和POST方法

### 错误2: "Cannot GET /api/activities/sync"

**原因**: 在浏览器中直接访问POST端点

**解决**: 
- 使用浏览器控制台的fetch方法
- 或使用Postman等工具
- 或查看已同步活动：`/api/synced-activities`

### 错误3: "404 Not Found"

**原因**: 端点路径拼写错误

**解决**: 检查路径是否正确：
- ✅ `/api/activities/sync`（正确）
- ❌ `/api/sync`（错误）
- ❌ `/api/activity/sync`（错误）

## 快速测试清单

- [ ] 服务器正在运行（端口8080）
- [ ] 浏览器可以访问 `http://localhost:8080/api/health`
- [ ] 浏览器可以访问 `http://localhost:8080/api/synced-activities`
- [ ] 使用POST方法测试 `/api/activities/sync`

## 总结

- ✅ **可以在浏览器访问的端点**：所有GET端点
- ❌ **不能在浏览器直接访问的端点**：POST端点（需要使用工具或代码）

如果想快速查看同步结果，访问：
```
http://localhost:8080/api/synced-activities
```

