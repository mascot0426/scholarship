# 数据库E-R图（Mermaid格式）

## 实体关系图

```mermaid
erDiagram
    users ||--o{ activities : "创建"
    users ||--o{ registrations : "报名"
    users ||--o{ waitlist : "候补"
    activities ||--o{ registrations : "有报名"
    activities ||--o{ waitlist : "有候补"
    
    users {
        TEXT student_id PK "学号(主键)"
        TEXT password "密码(SHA256)"
        INTEGER role "角色:0=管理员,1=发起人,2=学生"
        TEXT name "姓名"
        DATETIME created_at "创建时间"
    }
    
    activities {
        INTEGER id PK "活动ID(自增主键)"
        TEXT title "活动标题"
        TEXT description "活动描述"
        TEXT category "活动类别"
        TEXT organizer FK "发起人学号(外键)"
        DATETIME start_time "开始时间"
        DATETIME end_time "结束时间"
        INTEGER max_participants "最大参与人数"
        INTEGER current_participants "当前参与人数"
        TEXT location "活动地点"
        INTEGER status "状态:0=待审批,1=已批准,2=已拒绝,3=进行中,4=已结束"
        DATETIME created_at "创建时间"
        DATETIME approved_at "审批时间"
        TEXT approved_by "审批人学号"
        TEXT checkin_code "签到码"
    }
    
    registrations {
        INTEGER id PK "报名记录ID(自增主键)"
        INTEGER activity_id FK "活动ID(外键)"
        TEXT student_id FK "学生学号(外键)"
        TEXT student_name "学生姓名"
        INTEGER status "报名状态:0=已报名,1=已取消,2=候补,3=已确认"
        DATETIME registered_at "报名时间"
        DATETIME checkin_time "签到时间"
    }
    
    waitlist {
        INTEGER id PK "候补记录ID(自增主键)"
        INTEGER activity_id FK "活动ID(外键)"
        TEXT student_id FK "学生学号(外键)"
        TEXT student_name "学生姓名"
        DATETIME added_at "加入候补时间"
    }
```

## 关系说明

1. **users → activities** (1:N)
   - 一个用户可以创建多个活动
   - 关系字段：activities.organizer → users.student_id

2. **activities → registrations** (1:N)
   - 一个活动可以有多个报名记录
   - 关系字段：registrations.activity_id → activities.id
   - 约束：ON DELETE CASCADE（删除活动时自动删除报名记录）

3. **users → registrations** (1:N)
   - 一个用户可以报名多个活动
   - 关系字段：registrations.student_id → users.student_id
   - 约束：UNIQUE(activity_id, student_id)（一个学生只能报名一次同一活动）

4. **activities → waitlist** (1:N)
   - 一个活动可以有多个候补记录
   - 关系字段：waitlist.activity_id → activities.id
   - 约束：ON DELETE CASCADE

5. **users → waitlist** (1:N)
   - 一个用户可以在多个活动的候补列表中
   - 关系字段：waitlist.student_id → users.student_id
   - 约束：UNIQUE(activity_id, student_id)

## 索引说明

- **users表**：主键索引 student_id（自动创建）
- **activities表**：
  - 主键索引：id（自动创建）
  - 普通索引：status（idx_activities_status），用于快速查询特定状态的活动
- **registrations表**：
  - 主键索引：id（自动创建）
  - 普通索引：activity_id（idx_registrations_activity）
  - 普通索引：student_id（idx_registrations_student）
- **waitlist表**：
  - 主键索引：id（自动创建）
  - 普通索引：activity_id（idx_waitlist_activity）
