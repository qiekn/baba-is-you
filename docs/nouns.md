# Nouns 名词

> 数据参考：[Baba Is You Wiki - Category:Nouns](https://babaiswiki.fandom.com/wiki/Category:Nouns)

名词（Noun）是文字块的一类，每个名词对应场上的一种可渲染对象（Object）。
名词文字块的两个用途：

- `NOUN IS PROPERTY` ：给所有该名词实例附加一个属性（例：`BABA IS YOU`）。
- `NOUN IS NOUN` ：把场上所有前者实例替换成后者（例：`BABA IS ROCK`，所有 Baba 变成 Rock）。

文字块本身在规则里可用特殊名词 `TEXT` 指代。

---

## 分层：从 MVP 到全集

### 🟢 P1 MVP（只需这四个就能复刻「BABA IS YOU」首关）

| Text | 渲染对象 | 默认行为 | 说明 |
|------|---------|---------|------|
| `BABA` | 主角兔子 | 通常被赋 `YOU` | 最典型的玩家角色 |
| `WALL` | 砖墙 | 通常被赋 `STOP` | 阻挡通行，默认无属性 |
| `ROCK` | 石块 | 通常被赋 `PUSH` | 可推动的道具 |
| `FLAG` | 旗子 | 通常被赋 `WIN` | 通关目标 |

注意：**名词默认没有任何属性**。Wall 不会自动 Stop，必须场上有 `WALL IS STOP` 规则才成立。唯一例外是所有 **文字块** 默认带 `PUSH`（除非出现 `TEXT IS NOT PUSH`）。

### 🟡 P2 常见名词（扩展到 Solitary Island 前几关）

| Text | 渲染对象 | 常见角色 |
|------|---------|---------|
| `KEKE` | 第二角色（猫） | 双人控制或 NPC |
| `KEY` | 钥匙 | 通常 `PUSH`，配合 `DOOR IS OPEN, KEY IS SHUT`（或反之）触发消除 |
| `DOOR` | 门 | 通常 `STOP`，被 KEY 破除 |
| `WATER` | 水 | 通常 `SINK` |
| `LAVA` | 岩浆 | 通常 `HOT` |
| `SKULL` | 骷髅 | 通常 `DEFEAT` |
| `ICE` | 冰 | 通常 `MELT` 或 `PUSH`，按关卡而变 |
| `GRASS` | 草地 | 纯装饰，无默认规则 |
| `TILE` | 地砖 | 纯装饰 |
| `BOX` | 箱子 | 通常 `PUSH` |
| `TEXT` | 所有文字块（元名词） | 规则里写 `TEXT IS ...` 可给所有字块加属性 |

### 🔵 P3 进阶常见（完整通关需要）

角色类：`ME`、`ANNI`、`FOFO`、`IT`、`ROBOT`、`BELT`

小动物：`BUG`、`FISH`、`CRAB`、`BEE`、`BIRD`、`BAT`、`TURTLE`、`FROG`、`JELLY`、`DOG`、`CAT`、`GHOST`、`SNAIL`、`ROSE`

道具/物品：`PIPE`、`BOMB`、`ROPE`、`BOAT`、`BOOK`、`CAKE`、`CUP`、`ORB`、`STATUE`、`BRICK`、`STAR`、`SUN`、`MOON`、`CRYSTAL`、`FRUIT`、`TRUMPET`、`REED`、`LADDER`

地形：`HEDGE`、`CLOUD`、`LEAF`、`BRANCH`、`TREE`、`FLOWER`、`STUMP`、`FUNGUS`、`LILY`、`ALGA`、`SAND`、`DUST`、`DEADWOOD`、`SHRUBBERY`、`PLANK`、`RUBBLE`、`PILLAR`

特殊液/体：`BUBBLE`、`SEA`、`BOG`、`VOID`、`REVERIE`

### ⚫ P4 特殊名词（非具体对象）

这些名词不对应单一 sprite，而是起元数据/集合作用。P4 之前都可以先忽略。

| Text | 含义 |
|------|------|
| `ALL` | 当前关卡内所有名词（排除 `TEXT / EMPTY / LEVEL`）；一个名词只要在关卡中以 object 或 text 形式出现过就一直属于 ALL |
| `TEXT` | 所有文字块（包括自己） |
| `EMPTY` | 空格；`EMPTY IS ...` 会给所有空格应用属性；可用于 `X IS EMPTY` 强制删除 X |
| `LEVEL` | 关卡本身作为一个对象（在 World Map 上） |
| `IMAGE` | 关卡中预设的装饰图形 |
| `GROUP` / `GROUP2` / `GROUP3` | 自定义集合，用 `X IS GROUP` 把 X 加入集合，集合可被整体赋属性 |

---

## 实现建议

1. **数据驱动**：把名词列表、默认渲染 sprite、类别标签放在一个 JSON/TOML 文件里，运行时从资源包加载。不要写死在 C++ 枚举里——扩展时要改三处代码就输了。
2. **名词 ≠ 实体类型**：一个 `BABA` 实体（Object）和一个 `BABA` 文字块（Text）是两个不同的 entity，但共享同一个「noun id」。在 entt 里可以用两个 component（`ObjectNoun { NounId }` / `TextNoun { NounId }`）区分。
3. **图形帧**：原版每 150ms 切一次三帧动画，MVP 可先不做动画。
