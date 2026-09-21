---
name: header-default-argument-namespace-qualification
description: 头文件函数默认实参中使用常量时必须显式指定命名空间限定符（如 QKeyMapperConstants::）
metadata:
  type: pitfall
---

## Header Default Argument Namespace Qualification

在 `qkeymapper.h` 等头文件中声明成员函数时，若为参数提供默认值且该默认值为常量（定义在 `qkeymapper_constants.h` 中）：

**必须使用完全限定名，如 `QKeyMapperConstants::POPUP_MESSAGE_DISPLAY_TIME_DEFAULT`，切勿直接写无限定的名称。**

### 产生原因

- 绝大多数 `.cpp` 文件（如 `qkeymapper.cpp`）在文件顶部写了 `using namespace QKeyMapperConstants;`，因此在实现函数体中直接使用未限定的常量名可以编译通过；
- 但在头文件（`*.h`）中，为了避免污染全局命名空间，绝对不会在文件作用域引入 `using namespace QKeyMapperConstants;`；
- 当头文件声明了带默认实参的函数时：
  ```cpp
  // 错误：头文件作用域找不到无前缀的常量名
  void showInformationPopup(const QString &message, const QRect &targetRect = QRect(), int displayDuration = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT);

  // 正确：必须显式带上命名空间前缀
  void showInformationPopup(const QString &message, const QRect &targetRect = QRect(), int displayDuration = QKeyMapperConstants::POPUP_MESSAGE_DISPLAY_TIME_DEFAULT);
  ```

### 规则与规范

1. **头文件内一律显式限定**：凡是在 `*.h` 中引用的任何枚举、宏定义常量、结构体、命名空间常量，默认实参或类型声明均需带有完整的 `QKeyMapperConstants::` 前缀；
2. **代码检查清单**：每次修改头文件函数声明并添加默认值时，立即检查默认值是否依赖于命名空间常量。
