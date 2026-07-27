---
name: fakerinput-extra-info-limitations
description: FAKERINPUT_EXTRAINFO 宏无法稳定启用的原因集合
metadata:
  type: reference
  confidence: 0.95
  applications: 1
  category: fakerinput-architecture
  source: ep-2026-07-20-fakeinput-extra-info-review
---

# FAKERINPUT_EXTRAINFO 已知限制

FAKERINPUT_EXTRAINFO 编译宏（QKeyMapper.pro line 40，当前注释状态 `# DEFINES += FAKERINPUT_EXTRAINFO`）在代码层面完整，但存在无法根本解决的 Windows 输入栈底层限制。

## 不可修复的限制

1. **相同 VK Windows 去重**：物理键盘已按下某键时，FakerInput 再次发送同 VK 的 DOWN 事件，Windows 不生成新消息
2. **物理+FakerInput 同 VK 碰撞**：两者都是 extraInfo=0，FIFO 匹配无法区分来源
3. **ThreadPool clear 丢弃异步 KEY_UP**：序列中断时 `clearPressedVirtualKeysOfMappingKeys` 的异步 SendInputTask 可能被后续 ThreadPool::clear() 丢弃

## 已修复的限制

4. **FIFO 级联失配**（✅）：深度搜索 + 跳过丢失条目
5. **Stale 慢速排空**（✅）：while 循环一次性排空

## 参考

详细文档：`FAKERINPUT_EXTRAINFO_known_issues.md`
