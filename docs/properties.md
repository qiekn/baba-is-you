# Properties 属性

> 数据参考：[Baba Is You Wiki - Category:Properties](https://babaiswiki.fandom.com/wiki/Category:Properties)

属性（Property）是形容词类文字块，通过 `NOUN IS PROPERTY` 规则赋予对象某种行为/状态。
一个对象可以同时拥有多个属性；属性之间有明确的 **执行优先级** 与 **层级**（见底部）。

---

## 🟢 P1 MVP 四件套

| Text | 语义 | 实作要点 |
|------|------|---------|
| `YOU` | 玩家控制。按下方向键时，所有 `YOU` 对象尝试朝该方向移动一格 | 同时可以有多个 YOU（多选控制）；所有 YOU 一起移动 |
| `WIN` | 胜利条件。某个 `YOU` 与某个 `WIN` 处于同一格时，关卡胜利 | 在移动结算之后、下一回合开始前检查 |
| `STOP` | 阻挡移动。任何对象都无法穿过 `STOP` 的格子 | 注意 STOP 不能被推动；要同时可推，需要加 PUSH，但两者冲突以 STOP 占优 |
| `PUSH` | 可被推动。当移动方向上有该对象时，一并沿方向推开 | 递归链式：一次可以推一整列 PUSH；推到 STOP / 边界失败则整条链都不动 |

**隐式 PUSH**：所有文字块默认带 PUSH，除非有 `TEXT IS NOT PUSH` 规则主动移除。

---

## 🟡 P2 互动/销毁类

| Text | 语义 |
|------|------|
| `DEFEAT` | 任何 `YOU` 碰到 `DEFEAT` 都会被摧毁（DEFEAT 本身不消失）|
| `SINK` | 任何物体踏入 `SINK` 格时，双方都被摧毁 |
| `HOT` | 与 `MELT` 同格时，销毁 `MELT`；自身不消失 |
| `MELT` | 与 `HOT` 同格时自身被销毁 |
| `WEAK` | 任何推动/移动下只要碰到东西就自毁 |
| `SHUT` | 与 `OPEN` 同格时双方被销毁（常见于 KEY/DOOR） |
| `OPEN` | 与 `SHUT` 同格时双方被销毁 |

---

## 🔵 P3 移动/位移类

| Text | 语义 |
|------|------|
| `MOVE` | 每回合自己朝当前朝向移动一格；撞墙反向 |
| `SHIFT` | 推动站在自己身上的对象朝 SHIFT 的朝向移动（传送带） |
| `FALL` | 每回合自动向下移动一格 |
| `PULL` | 被相反方向推动时，一并拉动身后的 PULL 链 |
| `SWAP` | 与进入自己格的物体交换位置 |
| `TELE` | 进入 `TELE` 格的对象被随机传送到同关卡另一个 `TELE` 格 |
| `STILL` | 抵消所有主动移动属性，对象固定不动 |

---

## ⚫ P4 状态/层级类

| Text | 语义 |
|------|------|
| `FLOAT` | 漂浮层；只能与其他 FLOAT 交互（可越过 DEFEAT/HOT/MELT/OPEN/SHUT/SINK/WIN） |
| `WORD` | 把一个 Object 当作 Text 对待（可以参与规则构建） |
| `PHANTOM` | 幻影状态（视觉/规则特殊） |
| `HIDE` | 隐形（渲染效果） |
| `LOCKED` | 被锁定，不可被推动 |
| `POWER` | 关卡按钮/钥匙（与 World Map 相关） |
| `MORE` | 每回合在空地生成一个自己的拷贝 |
| `BEST` | 最佳结果标记（元关卡） |
| `REVERSE` | 翻转朝向 |
| `REVERT` | 回到上一状态 |

---

## 🟠 结果类（关卡层）

| Text | 语义 |
|------|------|
| `WIN` | （见 P1） |
| `END` | 触发 End 结局（剧情用） |
| `DONE` | 对象已完成（任务标记） |
| `BONUS` | 可收集的奖励 |

---

## 🔴 控制变体

| Text | 语义 |
|------|------|
| `YOU2` | 第二控制组，输入方向自动反向 |
| `SELECT` | 在 World Map 上标识可选关卡 |

---

## 🧊 表情/冷门（一般不影响 MVP）

`SAFE` `SCARY` `SAD` `HAPPY` `SLEEP` `SLIP` `TURN` `NUDGE` `HOLD` `PET` `PARTY` `WONDER`

原版完整 Editor Palette 属性列表（按字母序）：
`DEFEAT`、`DONE`、`END`、`FALL`、`FLOAT`、`HAPPY`、`HIDE`、`HOLD`、`HOT`、`LOCKED`、`MELT`、`MORE`、`MOVE`、`NUDGE`、`OPEN`、`PARTY`、`PET`、`PHANTOM`、`POWER`、`PULL`、`PUSH`、`REVERSE`、`REVERT`、`SAD`、`SAFE`、`SCARY`、`SELECT`、`SHIFT`、`SHUT`、`SINK`、`SLEEP`、`SLIP`、`STILL`、`STOP`、`SWAP`、`TELE`、`TURN`、`WEAK`、`WIN`、`WONDER`、`WORD`、`YOU`、`YOU2`

---

## 交互优先级

同格多个销毁属性同时触发时的结算顺序：

```
SINK  >  WEAK  >  HOT/MELT  >  DEFEAT
```

其它关键关系：

- `YOU` 优先于 `MOVE`：既 YOU 又 MOVE 的对象按玩家输入移动，忽略 MOVE 自走。
- `STOP` 优先于 `PUSH`：同时 STOP+PUSH 的对象不可推。
- `PUSH` / `PULL` 在 `DEFEAT` 结算前生效，可用于把 YOU 推离 DEFEAT 格。
- `FLOAT` 会把自己排除在大多数地面层销毁类（DEFEAT/HOT/MELT/OPEN/SHUT/SINK/WIN）之外，除非对方也是 FLOAT。

---

## 实现建议（数据驱动）

1. 每个属性定义为一个 `enum class Tag`，同时在 JSON 里有一份元数据（名字、描述、分类、优先级）。
2. 每回合规则解析后，清空所有对象的 Tag component，再按当前规则重新打上标签——原版就是无状态重算。
3. 把「销毁结算」实作成一个按固定顺序跑的 pass 序列：`SinkPass → WeakPass → HotMeltPass → DefeatPass`，每个 pass 内只做一件事。不要想着用一个大 switch 处理所有互动。
4. MVP 实现顺序推荐：
   - 先 `YOU` + `STOP` + `PUSH`（走/撞/推）
   - 再 `WIN`（判胜）
   - 再 `IS NOUN` 变形（证明规则系统可扩展）
   - 再加 `DEFEAT`、`SINK` 验证 pass 架构
   - 最后才碰 `FLOAT`、条件操作符这类元机制
