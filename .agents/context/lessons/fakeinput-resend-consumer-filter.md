---
name: fakeinput-resend-consumer-filter
description: FakerInput 驱动的 extraInfo=0 限制应在消费端过滤而非 Hook 端追踪
metadata:
  type: pattern
  confidence: 0.95
  applications: 2
  category: fakerinput-architecture
  source: ep-2026-07-20-fakeinput-resend-fix
  related_memories:
    - fakerinput-extra-info-limitations
---

# FakerInput 污染数据应在消费端过滤

## Pattern

FakerInput 通过内核驱动发送按键时 `dwExtraInfo=0`，Hook 无法区分虚拟按键和物理按键。**不应在 Hook 层面尝试追踪或标记 FakerInput 按键**（FAKERINPUT_EXTRAINFO 机制已被证明不可靠），而应在消费 `pressedRealKeysList` 数据的位置（如 `resendRealKeyCodeOnStop`）进行过滤。

## Problem

FakerInput 发送的按键在 Hook 中被当作真实物理按键加入 `pressedRealKeysList`，导致 `resendRealKeyCodeOnStop` 错误地重发这些合成按键。

## Solution

在消费端利用已有的 `SendMappingKeyMethod` 字段判断：当行使用 `SENDMAPPINGKEY_METHOD_FAKERINPUT` 时，其 `Pure_MappingKeys` 不应参与 resend 检查。

```cpp
// 在 resendRealKeyCodeOnStop 中
if (KeyMappingDataList_ForResend->at(rowindex).SendMappingKeyMethod != SENDMAPPINGKEY_METHOD_FAKERINPUT) {
    keyListToCheck = KeyMappingDataList_ForResend->at(rowindex).Pure_MappingKeys;
}
```

## Why This Works

- `Pure_MappingKeys` 是合成按键，永远不应出现在 `pressedRealKeysList`（物理按键列表）中
- 如果出现，必定是 extraInfo=0 导致的污染数据，应被忽略
- `Pure_OriginalKeys` 不受影响（它们是用户物理按下的原始键）
- 零新状态、零跨线程同步、零性能影响

## When to Use

任何需要消费 `pressedRealKeysList` 或 `pressedRealKeysListRemoveMultiInput` 并与映射键交叉检查的场景，都应该意识到 FakerInput 行会污染这些列表。

## Confidence

0.95 — 已验证在 FAKERINPUT_EXTRAINFO ON/OFF 两种配置下均正确工作。
