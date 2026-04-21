# Operators 操作符

> 数据参考：[Baba Is You Wiki - Order of Operations](https://babaiswiki.fandom.com/wiki/Order_of_Operations)、[AND](https://babaiswiki.fandom.com/wiki/AND)、[Esolang - Baba Is You](https://esolangs.org/wiki/Baba_Is_You)

操作符（Operator）是规则里的连接词，把 Noun 和 Property / Noun 组合成完整规则。

**规则扫描方式**：游戏每回合重新扫描整张棋盘，横向（左→右）和纵向（上→下）两遍；匹配到合法的 `SUBJECT OP COMPLEMENT` 序列就激活规则。无效组合不生效、也不报错。

---

## 分层

### 🟢 P1 MVP

| Text | 规则形式 | 语义 |
|------|---------|------|
| `IS` | `SUBJECT IS PROPERTY` | 赋属性（例：`BABA IS YOU`）|
| `IS` | `SUBJECT IS NOUN` | 变形：扫描时把 Subject 全部替换成 Noun（例：`BABA IS ROCK`） |

只支持 `IS` 一个操作符已经能做出原版前 5 关。

### 🟡 P2 扩展

| Text | 规则形式 | 语义 |
|------|---------|------|
| `AND` | `A AND B IS P` / `A IS P AND Q` | 逻辑合取，展开成两条独立规则；主语、属性两侧都能用 |
| `NOT` | `A IS NOT P` / `NOT A IS P` | 否定；作用在属性上表示「取消」该属性；作用在主语上表示「除了 A 以外的所有 Noun」 |
| `HAS` | `A HAS B` | 当 A 被摧毁时，在原位置生成一个 B（例：`BABA HAS KEY` 被打败时掉钥匙） |

实作要点：
- `AND` 展开发生在语法层，不是语义层。`BABA AND KEKE IS YOU` 等价于 `BABA IS YOU` + `KEKE IS YOU` 两条规则。
- `NOT` 的作用域只覆盖紧邻的那一个词，不能连续否定（`NOT NOT YOU` 不合法）。
- `HAS` 的 B 可以是任意 Noun，包括 TEXT（`BABA HAS TEXT` 被打败时掉出「BABA」文字块）。

### 🔵 P3 进阶

| Text | 规则形式 | 语义 |
|------|---------|------|
| `MAKE` | `A MAKE B` | A 所在的每一格每回合生成一个 B（持续产出，不是被击毁触发） |
| `ON` | `A ON B IS P` | 条件：只有当 A 与 B 重叠时，A 才获得属性 P |
| `NEAR` | `A NEAR B IS P` | 条件：A 与 B 相邻（八邻域或四邻域，查原版实现）时 A 获得 P |
| `FACING` | `A FACING B IS P` | A 朝向 B 时 A 获得 P |
| `WITHOUT` | `A WITHOUT B IS P` | 当关卡中不存在 B 时 A 获得 P |
| `LONELY` | `A LONELY IS P` | 当 A 周围（相邻格）没有任何其它对象时 A 获得 P |
| `FEELING` | `A FEELING P IS Q` | A 当前具备属性 P 时，再获得属性 Q（属性触发条件） |

条件操作符都是「短语修饰主语」的形式，可以和 `AND`/`NOT` 嵌套：
- `BABA ON WATER IS SINK`
- `NOT BABA IS YOU`（除 BABA 外所有都是 YOU）
- `BABA AND KEKE ON GRASS IS YOU`

---

## 规则解析顺序 & 优先级

原版游戏每帧（实际是每回合）按固定步骤处理：

1. **收集字块**：扫描场上所有文字块位置。
2. **模式匹配**：横向（←→）、纵向（↑↓）各扫一遍，匹配 `SUBJECT [COND-PHRASE] OP COMPLEMENT`，支持 `AND` 展开。
3. **规则去重/合并**：同一条规则多次出现只生效一次。
4. **条件评估**：对带 `ON/NEAR/FACING/WITHOUT/LONELY/FEELING` 的规则，按当前棋盘状态评估是否成立。
5. **属性应用**：按对象顺序（对象在内部列表中的顺序）将通过的规则的属性广播到对应 Subject 所有实例。
6. **玩家输入 + 移动阶段**：YOU 的对象按玩家输入尝试移动，触发 PUSH / PULL / SWAP 等交互。
7. **转换阶段**：执行 `A IS B`（变形）与 `HAS` / `MAKE` 的生成/消解。
8. **胜负判定**：YOU 与 WIN 同格则胜利；所有 YOU 消失则失败。

**属性执行优先级**（同格多属性时）：`SINK` > `WEAK` > `HOT`/`MELT` > `DEFEAT`。`PUSH` 和 `PULL` 可以在 `DEFEAT` 结算前把玩家推开，从而反向利用。

---

## 实现建议（给 MVP）

- 先只实现 `IS`，一个 `Rule { Subject, Property }` 的小结构体就够用。
- 规则解析每回合重来一次，别尝试「增量更新」——原版就是全量重扫，代码最简单也最正确。
- 用 entt 的方式：规则应用后，给每个 Object 实体加/减对应的属性 component（如 `Tag::You`、`Tag::Push`）。每回合清空再重建，保证无状态残留。
- `AND` 可以作为一种预处理：把 `A AND B IS P` 拆成两条 `Rule`，后续一视同仁。
