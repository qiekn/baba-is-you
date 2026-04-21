#include "game_layer.h"

#include <array>
#include <cstring>
#include <string>

#include <imgui.h>
#include <raylib.h>

#include "board.h"
#include "components.h"

namespace {

constexpr float kAnimFps = 6.0f;  // cycle 1->2->3 every ~0.5s
constexpr const char* kDefaultLevel = "assets/levels/starter.json";
constexpr const char* kSpritesDir = "assets/sprites";
constexpr const char* kLevelsDir = "assets/levels";

// Draw a sprite texture fitted to a board cell, tinted with `color`.
void DrawSpriteInCell(const Texture2D& tex, Rectangle cell, Color color) {
  if (tex.id == 0) return;
  const Rectangle src = {0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
  DrawTexturePro(tex, src, cell, {0.0f, 0.0f}, 0.0f, color);
}

// 4-bit neighbor mask: right=1, up=2, left=4, down=8. Matches the variant
// number encoded in Baba Is You sprite filenames (wall_<mask>_<frame>.png).
int ComputeTileMask(const entt::registry& registry, ObjectId id, int x, int y) {
  auto has_same = [&](int nx, int ny) {
    if (nx < 0 || nx >= board::kCols || ny < 0 || ny >= board::kRows) return false;
    for (auto [e, cell, kind] : registry.view<const Cell, const Kind>().each()) {
      if (cell.x == nx && cell.y == ny && kind.id == id) return true;
    }
    return false;
  };
  int mask = 0;
  if (has_same(x + 1, y)) mask |= 1;
  if (has_same(x, y - 1)) mask |= 2;
  if (has_same(x - 1, y)) mask |= 4;
  if (has_same(x, y + 1)) mask |= 8;
  return mask;
}

}  // namespace

GameLayer::GameLayer() : Layer("GameLayer") {}

void GameLayer::OnAttach() {
  sprites_.LoadAll(kSpritesDir);
  LoadLevelFromPath(kDefaultLevel);
}

void GameLayer::OnDetach() {
  sprites_.Unload();
  registry_.clear();
}

void GameLayer::OnUpdate(float dt) {
  // Animation frame cycle (always on, independent of turn).
  anim_timer_ += dt;
  const float period = 1.0f / kAnimFps;
  while (anim_timer_ >= period) {
    anim_timer_ -= period;
    current_frame_ = (current_frame_ % 3) + 1;
  }

  // Recompute rules every frame so the editor sees live feedback.
  RecomputeRules();

  // Input: turn-based movement + undo.
  if (ImGui::GetIO().WantCaptureKeyboard) return;

  if (IsKeyPressed(KEY_R)) {
    reset_requested_ = true;
    return;
  }

  if (edit_mode_) {
    HandleEditorMouse();
    return;
  }

  if (IsKeyPressed(KEY_Z)) {
    Snapshot snap;
    if (undo_.Pop(snap)) {
      RestoreSnapshot(registry_, snap);
      won_ = false;
    }
    return;
  }

  struct KeyDir {
    int key;
    Direction dir;
  };
  static constexpr std::array<KeyDir, 8> kKeys = {{
      {KEY_RIGHT, Direction::Right},
      {KEY_D, Direction::Right},
      {KEY_UP, Direction::Up},
      {KEY_W, Direction::Up},
      {KEY_LEFT, Direction::Left},
      {KEY_A, Direction::Left},
      {KEY_DOWN, Direction::Down},
      {KEY_S, Direction::Down},
  }};

  // Scan keys in priority order; first one held wins so switching directions
  // feels responsive.
  std::optional<Direction> cur_dir;
  for (const auto& kd : kKeys) {
    if (IsKeyDown(kd.key)) {
      cur_dir = kd.dir;
      break;
    }
  }

  constexpr float kRepeatDelay = 0.22f;
  constexpr float kRepeatInterval = 0.08f;

  if (cur_dir != held_dir_) {
    held_dir_ = cur_dir;
    hold_time_ = 0.0f;
    first_repeat_done_ = false;
    if (cur_dir) Step(*cur_dir);
  } else if (cur_dir) {
    hold_time_ += dt;
    const float threshold = first_repeat_done_ ? kRepeatInterval : kRepeatDelay;
    if (hold_time_ >= threshold) {
      Step(*cur_dir);
      hold_time_ = 0.0f;
      first_repeat_done_ = true;
    }
  }
}

void GameLayer::OnRender() {
  auto draw_kind = [&](const Cell& cell, const Kind& kind) {
    const int variant = IsAutoTiled(kind.id) ? ComputeTileMask(registry_, kind.id, cell.x, cell.y) : 0;
    const auto& tex = sprites_.Get(kind.id, current_frame_, variant);
    DrawSpriteInCell(tex, board::CellRect(cell.x, cell.y), sprites_.TintFor(kind.id));
  };

  // Three draw layers match the original's ordering.
  // 1) Floor decorations — grass/flower/tile sit below everything.
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    if (LayerOf(kind.id) == DrawLayer::Floor) draw_kind(cell, kind);
  }
  // 2a) Text blocks.
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    const auto& tex = sprites_.Get(text.id, current_frame_);
    DrawSpriteInCell(tex, board::CellRect(cell.x, cell.y), sprites_.TintFor(text.id));
  }
  // 2b) Gameplay objects on top of text.
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    if (LayerOf(kind.id) == DrawLayer::Object) draw_kind(cell, kind);
  }
  // 3) Float decorations — cloud/star drawn on top of everything.
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    if (LayerOf(kind.id) == DrawLayer::Float) draw_kind(cell, kind);
  }

  // Edit-mode affordances: brush preview on hovered cell.
  if (edit_mode_ && !ImGui::GetIO().WantCaptureMouse) {
    if (auto cell = board::ScreenToCell(GetMousePosition())) {
      const auto [col, row] = *cell;
      const Rectangle rect = board::CellRect(col, row);
      DrawRectangleLinesEx(rect, 2.0f, YELLOW);
      if (std::holds_alternative<ObjectId>(brush_)) {
        const auto id = std::get<ObjectId>(brush_);
        Color tint = sprites_.TintFor(id);
        tint.a = 128;
        DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
      } else {
        const auto id = std::get<TextId>(brush_);
        Color tint = sprites_.TintFor(id);
        tint.a = 128;
        DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
      }
    }
  }

  if (won_) {
    const char* msg = "YOU WIN!";
    const int size = 48;
    const int w = MeasureText(msg, size);
    DrawText(msg, (GetScreenWidth() - w) / 2, 40, size, GOLD);
  }
}

void GameLayer::OnImGuiRender() {
  if (reset_requested_) {
    ImGui::OpenPopup("Reset level?");
    reset_requested_ = false;
  }

  const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Reset level?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Revert the board to the level's starting state?");
    ImGui::Text("All moves will be lost.");
    ImGui::Separator();
    if (ImGui::Button("Reset", ImVec2(120, 0))) {
      ResetToInitial();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)) || IsKeyPressed(KEY_ESCAPE)) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  DrawScenePanel();
  DrawRulesPanel();
  DrawEditorPanel();
}

// ---------------------------------------------------------------------------
// Level / registry
// ---------------------------------------------------------------------------

void GameLayer::LoadLevelFromPath(const std::filesystem::path& path) {
  Level loaded;
  if (!LoadLevelFromJson(path, loaded)) {
    TraceLog(LOG_WARNING, "Failed to load level: %s", path.string().c_str());
    return;
  }
  level_ = std::move(loaded);
  initial_level_ = level_;
  BuildRegistryFromLevel();
  undo_.Clear();
  won_ = false;
  RecomputeRules();
}

void GameLayer::ResetToInitial() {
  level_ = initial_level_;
  BuildRegistryFromLevel();
  undo_.Clear();
  won_ = false;
  RecomputeRules();
}

void GameLayer::SaveLevelToPath(const std::filesystem::path& path) {
  Level current = ExtractLevelFromRegistry();
  if (!SaveLevelToJson(path, current)) {
    TraceLog(LOG_WARNING, "Failed to save level: %s", path.string().c_str());
  }
}

void GameLayer::BuildRegistryFromLevel() {
  ClearRegistry();
  for (const auto& tile : level_.tiles) {
    if (tile.IsObject()) {
      SpawnObject(std::get<ObjectId>(tile.kind), tile.x, tile.y);
    } else {
      SpawnText(std::get<TextId>(tile.kind), tile.x, tile.y);
    }
  }
}

Level GameLayer::ExtractLevelFromRegistry() const {
  Level lvl;
  lvl.cols = board::kCols;
  lvl.rows = board::kRows;
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    LevelTile tile;
    tile.x = cell.x;
    tile.y = cell.y;
    tile.kind = kind.id;
    lvl.tiles.push_back(tile);
  }
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    LevelTile tile;
    tile.x = cell.x;
    tile.y = cell.y;
    tile.kind = text.id;
    lvl.tiles.push_back(tile);
  }
  return lvl;
}

void GameLayer::ClearRegistry() { registry_.clear(); }

void GameLayer::SpawnObject(ObjectId id, int x, int y) {
  auto e = registry_.create();
  registry_.emplace<Cell>(e, x, y);
  registry_.emplace<Kind>(e, id);
  registry_.emplace<Facing>(e);
  registry_.emplace<AnimFrame>(e);
}

void GameLayer::SpawnText(TextId id, int x, int y) {
  auto e = registry_.create();
  registry_.emplace<Cell>(e, x, y);
  registry_.emplace<TextBlock>(e, id);
  registry_.emplace<Facing>(e);
  registry_.emplace<AnimFrame>(e);
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

void GameLayer::RecomputeRules() {
  RuleBoard rb(board::kCols, board::kRows);
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    rb.Set(cell.x, cell.y, text.id);
  }
  rules_ = ParseRules(rb);
  ApplyRules(registry_, rules_);
}

// ---------------------------------------------------------------------------
// Turn step
// ---------------------------------------------------------------------------

std::pair<int, int> GameLayer::Delta(Direction dir) {
  switch (dir) {
    case Direction::Right:
      return {1, 0};
    case Direction::Up:
      return {0, -1};
    case Direction::Left:
      return {-1, 0};
    case Direction::Down:
      return {0, 1};
  }
  return {0, 0};
}

bool GameLayer::CanEnter(int x, int y, int dx, int dy) {
  if (x < 0 || x >= board::kCols || y < 0 || y >= board::kRows) return false;
  bool any_push = false;
  for (auto [e, cell] : registry_.view<const Cell>().each()) {
    if (cell.x != x || cell.y != y) continue;
    if (registry_.all_of<IsStop>(e)) return false;
    if (registry_.all_of<IsPush>(e)) any_push = true;
  }
  if (any_push) return CanEnter(x + dx, y + dy, dx, dy);
  return true;
}

void GameLayer::PushChain(int x, int y, int dx, int dy) {
  std::vector<entt::entity> pushers;
  for (auto [e, cell] : registry_.view<Cell, const IsPush>().each()) {
    if (cell.x == x && cell.y == y) pushers.push_back(e);
  }
  if (pushers.empty()) return;
  PushChain(x + dx, y + dy, dx, dy);
  for (auto e : pushers) {
    auto& c = registry_.get<Cell>(e);
    c.x = x + dx;
    c.y = y + dy;
  }
}

bool GameLayer::TryMove(entt::entity who, Direction dir) {
  const auto& c = registry_.get<const Cell>(who);
  const auto [dx, dy] = Delta(dir);
  const int tx = c.x + dx;
  const int ty = c.y + dy;
  if (!CanEnter(tx, ty, dx, dy)) return false;
  PushChain(tx, ty, dx, dy);
  auto& mut = registry_.get<Cell>(who);
  mut.x = tx;
  mut.y = ty;
  if (auto* facing = registry_.try_get<Facing>(who)) facing->dir = dir;
  return true;
}

void GameLayer::Step(Direction dir) {
  if (won_) return;

  undo_.Push(CaptureSnapshot(registry_));

  std::vector<entt::entity> yous;
  for (auto e : registry_.view<const IsYou>()) yous.push_back(e);
  bool any_moved = false;
  for (auto e : yous) {
    if (TryMove(e, dir)) any_moved = true;
  }

  if (!any_moved) {
    // Discard the no-op snapshot to avoid spamming undo history.
    Snapshot discard;
    undo_.Pop(discard);
  }

  RecomputeRules();
  RunWinDefeat();
}

void GameLayer::RunWinDefeat() {
  // Collect YOU entity cells.
  std::vector<std::pair<int, int>> you_cells;
  for (auto [e, cell] : registry_.view<const Cell, const IsYou>().each()) {
    you_cells.emplace_back(cell.x, cell.y);
  }

  // WIN: any YOU sharing a cell with a WIN entity.
  for (auto [e, cell] : registry_.view<const Cell, const IsWin>().each()) {
    for (const auto& yc : you_cells) {
      if (yc.first == cell.x && yc.second == cell.y) {
        won_ = true;
        return;
      }
    }
  }

  // DEFEAT: destroy YOU entities sharing a cell with a DEFEAT entity.
  std::vector<entt::entity> doomed;
  auto defeat_view = registry_.view<const Cell, const IsDefeat>();
  auto you_view = registry_.view<const Cell, const IsYou>();
  for (auto [ye, yc] : you_view.each()) {
    for (auto [de, dc] : defeat_view.each()) {
      if (yc.x == dc.x && yc.y == dc.y) {
        doomed.push_back(ye);
        break;
      }
    }
  }
  for (auto e : doomed) registry_.destroy(e);
}

// ---------------------------------------------------------------------------
// ImGui panels
// ---------------------------------------------------------------------------

const char* GameLayer::PrettyName(ObjectId id) {
  switch (id) {
    case ObjectId::Baba:
      return "Baba";
    case ObjectId::Flag:
      return "Flag";
    case ObjectId::Wall:
      return "Wall";
    case ObjectId::Rock:
      return "Rock";
    case ObjectId::Grass:
      return "Grass";
    case ObjectId::Flower:
      return "Flower";
    case ObjectId::Tile:
      return "Tile";
    case ObjectId::Cloud:
      return "Cloud";
    case ObjectId::Star:
      return "Star";
    case ObjectId::kCount:
      break;
  }
  return "?";
}

const char* GameLayer::PrettyName(TextId id) {
  switch (id) {
    case TextId::Is:
      return "IS";
    case TextId::And:
      return "AND";
    case TextId::Not:
      return "NOT";
    case TextId::Baba:
      return "BABA";
    case TextId::Flag:
      return "FLAG";
    case TextId::Wall:
      return "WALL";
    case TextId::Rock:
      return "ROCK";
    case TextId::You:
      return "YOU";
    case TextId::Win:
      return "WIN";
    case TextId::Stop:
      return "STOP";
    case TextId::Push:
      return "PUSH";
    case TextId::Move:
      return "MOVE";
    case TextId::Defeat:
      return "DEFEAT";
    case TextId::kCount:
      break;
  }
  return "?";
}

void GameLayer::DrawScenePanel() {
  if (!ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  int object_count = 0;
  for ([[maybe_unused]] auto [e, k] : registry_.view<const Kind>().each()) ++object_count;
  int text_count = 0;
  for ([[maybe_unused]] auto [e, t] : registry_.view<const TextBlock>().each()) ++text_count;

  ImGui::Text("Entities: %d objects, %d text", object_count, text_count);
  ImGui::Separator();

  if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
      const bool you = registry_.all_of<IsYou>(e);
      const bool push = registry_.all_of<IsPush>(e);
      const bool stop = registry_.all_of<IsStop>(e);
      const bool win = registry_.all_of<IsWin>(e);
      ImGui::Text("%s @ (%d,%d)%s%s%s%s", PrettyName(kind.id), cell.x, cell.y, you ? " [YOU]" : "",
                  push ? " [PUSH]" : "", stop ? " [STOP]" : "", win ? " [WIN]" : "");
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Text", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
      ImGui::Text("%s @ (%d,%d)", PrettyName(text.id), cell.x, cell.y);
    }
    ImGui::TreePop();
  }

  ImGui::End();
}

void GameLayer::DrawRulesPanel() {
  if (!ImGui::Begin("Rules", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }
  if (rules_.empty()) {
    ImGui::TextDisabled("No active rules. Place text blocks to form NOUN IS PROPERTY.");
  } else {
    for (const auto& rule : rules_) {
      ImGui::Text("%s . IS . %s", PrettyName(rule.subject), PrettyName(rule.predicate));
    }
  }
  ImGui::Separator();
  ImGui::Text("Undo history: %zu", undo_.Size());
  ImGui::End();
}

void GameLayer::DrawEditorPanel() {
  if (!ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  ImGui::Checkbox("Edit Mode", &edit_mode_);
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    reset_requested_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    ClearRegistry();
    undo_.Clear();
    won_ = false;
  }

  ImGui::Separator();
  ImGui::Text("Brush");
  ImGui::TextDisabled("Left click: place   Right click: erase");

  constexpr float kBtn = 44.0f;
  const ImVec2 btn_size{kBtn, kBtn};

  // Object row
  for (int i = 0; i < kObjectCount; ++i) {
    if (i > 0) ImGui::SameLine();
    const auto id = static_cast<ObjectId>(i);
    const bool selected = std::holds_alternative<ObjectId>(brush_) && std::get<ObjectId>(brush_) == id;
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
    if (ImGui::Button(PrettyName(id), btn_size)) brush_ = id;
    if (selected) ImGui::PopStyleColor();
  }

  // Text rows (6 per row)
  for (int i = 0; i < kTextCount; ++i) {
    if (i % 6 != 0) ImGui::SameLine();
    const auto id = static_cast<TextId>(i);
    const bool selected = std::holds_alternative<TextId>(brush_) && std::get<TextId>(brush_) == id;
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
    if (ImGui::Button(PrettyName(id), btn_size)) brush_ = id;
    if (selected) ImGui::PopStyleColor();
  }

  ImGui::Separator();
  ImGui::InputText("File", save_name_, sizeof(save_name_));
  if (ImGui::Button("Load")) {
    LoadLevelFromPath(std::filesystem::path{kLevelsDir} / save_name_);
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    SaveLevelToPath(std::filesystem::path{kLevelsDir} / save_name_);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload Starter")) {
    LoadLevelFromPath(kDefaultLevel);
  }

  ImGui::End();
}

// ---------------------------------------------------------------------------
// Editor mouse
// ---------------------------------------------------------------------------

void GameLayer::HandleEditorMouse() {
  if (ImGui::GetIO().WantCaptureMouse) return;
  auto cell = board::ScreenToCell(GetMousePosition());
  if (!cell) return;
  const auto [col, row] = *cell;

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    PlaceBrushAt(col, row);
  } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    EraseAt(col, row);
  }
}

void GameLayer::PlaceBrushAt(int col, int row) {
  // Don't duplicate the exact same tile.
  if (std::holds_alternative<ObjectId>(brush_)) {
    const ObjectId id = std::get<ObjectId>(brush_);
    for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
      if (cell.x == col && cell.y == row && kind.id == id) return;
    }
    SpawnObject(id, col, row);
  } else {
    const TextId id = std::get<TextId>(brush_);
    for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
      if (cell.x == col && cell.y == row && text.id == id) return;
    }
    SpawnText(id, col, row);
  }
  undo_.Clear();
}

void GameLayer::EraseAt(int col, int row) {
  std::vector<entt::entity> doomed;
  for (auto [e, cell] : registry_.view<const Cell>().each()) {
    if (cell.x == col && cell.y == row) doomed.push_back(e);
  }
  for (auto e : doomed) registry_.destroy(e);
  undo_.Clear();
}
