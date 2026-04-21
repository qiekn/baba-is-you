#include "game_layer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
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
constexpr const char* kImportedDir = "assets/imported";
constexpr const char* kWorldFile = "assets/worlds/tutorial.json";
constexpr const char* kProgressFile = "progress.json";

struct MusicTrack {
  const char* label;
  const char* path;
};

constexpr MusicTrack kTracks[] = {
    {"Map", "assets/music/map.ogg"},
    {"Menu", "assets/music/menu.ogg"},
    {"Baba", "assets/music/baba.ogg"},
    {"Cave", "assets/music/cave.ogg"},
    {"Garden", "assets/music/garden.ogg"},
    {"Forest", "assets/music/forest.ogg"},
};
constexpr int kTrackCount = static_cast<int>(sizeof(kTracks) / sizeof(kTracks[0]));

struct LevelEntry {
  const char* label;
  const char* file;
};

constexpr LevelEntry kBuiltinLevels[] = {
    {"Starter", "starter.json"},
    {"01 Intro", "01-intro.json"},
    {"02 Walls", "02-walls.json"},
    {"03 Push", "03-push.json"},
    {"04 Break", "04-break.json"},
};
constexpr int kBuiltinLevelCount = static_cast<int>(sizeof(kBuiltinLevels) / sizeof(kBuiltinLevels[0]));

// Draw a sprite texture fitted to a board cell, tinted with `color`.
void DrawSpriteInCell(const Texture2D& tex, Rectangle cell, Color color) {
  if (tex.id == 0) return;
  const Rectangle src = {0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
  DrawTexturePro(tex, src, cell, {0.0f, 0.0f}, 0.0f, color);
}

// Tool/sub-mode icons drawn procedurally on top of a regular ImGui::Button.
// Each toolbar button uses one of these so we don't ship any extra art.
enum class ToolGlyph : std::uint8_t {
  Pen,
  Line,
  RectOutline,
  RectFilled,
  Select,
  Bucket,
  Eraser,
};

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

std::optional<Color> GameLayer::LevelBackground() const {
  if (level_.bg_r < 0) return std::nullopt;
  return Color{static_cast<unsigned char>(level_.bg_r),
               static_cast<unsigned char>(level_.bg_g),
               static_cast<unsigned char>(level_.bg_b), 255};
}

std::optional<Color> GameLayer::LevelEdge() const {
  if (level_.edge_r < 0) return std::nullopt;
  return Color{static_cast<unsigned char>(level_.edge_r),
               static_cast<unsigned char>(level_.edge_g),
               static_cast<unsigned char>(level_.edge_b), 255};
}

void GameLayer::OnAttach() {
  sprites_.LoadAll(kSpritesDir);
  LoadWorld(kWorldFile, world_);
  LoadProgress(kProgressFile, progress_);
  // Scan the imported-level directory (may not exist in a fresh checkout).
  std::error_code ec;
  if (std::filesystem::is_directory(kImportedDir, ec)) {
    for (const auto& entry : std::filesystem::directory_iterator(kImportedDir, ec)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        imported_stems_.push_back(entry.path().stem().string());
      }
    }
    // Natural sort: leading digits compared numerically, then the rest
    // alphabetically. Keeps 1, 2, 10, 100 in order instead of 1, 10, 100, 2.
    auto split = [](const std::string& s) -> std::pair<long long, std::string> {
      std::size_t i = 0;
      while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
      long long n = i > 0 ? std::stoll(s.substr(0, i)) : -1;
      return {n, s.substr(i)};
    };
    std::sort(imported_stems_.begin(), imported_stems_.end(),
              [&](const std::string& a, const std::string& b) {
                return split(a) < split(b);
              });
  }
  LoadLevelFromPath(kDefaultLevel);
  LoadTrack(track_index_);
}

void GameLayer::OnDetach() {
  sprites_.Unload();
  registry_.clear();
  UnloadTrack();
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

  UpdateParticles(dt);

  if (music_loaded_) {
    SetMusicVolume(music_, muted_ ? 0.0f : volume_);
    UpdateMusicStream(music_);
  }

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

  if (cur_dir != held_dir_) {
    held_dir_ = cur_dir;
    hold_time_ = 0.0f;
    first_repeat_done_ = false;
    if (cur_dir) Step(*cur_dir);
  } else if (cur_dir) {
    hold_time_ += dt;
    const float threshold = first_repeat_done_ ? repeat_interval_ : repeat_delay_;
    if (hold_time_ >= threshold) {
      Step(*cur_dir);
      hold_time_ = 0.0f;
      first_repeat_done_ = true;
    }
  }
}

void GameLayer::OnRender() {
  auto draw_kind = [&](entt::entity e, const Cell& cell, const Kind& kind) {
    int variant = 0;
    if (IsAutoTiled(kind.id)) {
      variant = ComputeTileMask(registry_, kind.id, cell.x, cell.y);
    } else if (IsDirectional(kind.id)) {
      const Direction dir = registry_.try_get<Facing>(e) ? registry_.get<Facing>(e).dir : Direction::Right;
      variant = DirectionToVariant(dir);
    }
    const auto& tex = sprites_.Get(kind.id, current_frame_, variant);
    DrawSpriteInCell(tex, board::CellRect(cell.x, cell.y), sprites_.TintFor(kind.id));
  };

  // Gather everything with its engine layer number so we can draw strictly
  // bottom-to-top. Values come from values.lua — e.g. tile=4 (bottom),
  // wall=14, flag=17, baba=18, text=20.
  struct Draw {
    int layer;
    entt::entity e;
    int x, y;
    int kind;  // 0 = object, 1 = text
  };
  std::vector<Draw> draws;
  draws.reserve(registry_.storage<Cell>().size());
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    draws.push_back({LayerOf(kind.id), e, cell.x, cell.y, 0});
  }
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    draws.push_back({LayerOf(text.id), e, cell.x, cell.y, 1});
  }
  std::sort(draws.begin(), draws.end(),
            [](const Draw& a, const Draw& b) { return a.layer < b.layer; });
  for (const Draw& d : draws) {
    if (d.kind == 0) {
      const auto& kind = registry_.get<const Kind>(d.e);
      draw_kind(d.e, registry_.get<const Cell>(d.e), kind);
    } else {
      const auto& text = registry_.get<const TextBlock>(d.e);
      const auto& tex = sprites_.Get(text.id, current_frame_);
      DrawSpriteInCell(tex, board::CellRect(d.x, d.y), sprites_.TintFor(text.id));
    }
  }
  // 4) Particles (sparkles for IsWin entities).
  DrawParticles();

  if (edit_mode_ && !ImGui::GetIO().WantCaptureMouse) {
    DrawEditorOverlay();
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
  DrawWorldPanel();
  DrawSettingsPanel();
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
  board::kCols = level_.cols;
  board::kRows = level_.rows;
  current_level_id_ = path.stem().string();
  win_handled_ = false;
  BuildRegistryFromLevel();
  undo_.Clear();
  won_ = false;
  RecomputeRules();
}

void GameLayer::ResetToInitial() {
  level_ = initial_level_;
  win_handled_ = false;
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
        if (!won_) {
          won_ = true;
          if (!win_handled_ && !current_level_id_.empty()) {
            MarkLevelCompleted(current_level_id_);
            win_handled_ = true;
          }
        }
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
// Particles
// ---------------------------------------------------------------------------

void GameLayer::UpdateParticles(float dt) {
  constexpr float kEmitPeriod = 0.07f;
  constexpr float kLifetime = 0.8f;
  constexpr float kMaxSize = 10.0f;

  for (auto& p : particles_) p.life -= dt;
  std::erase_if(particles_, [](const Particle& p) { return p.life <= 0.0f; });

  // Collect IsWin cells; skip if none so the editor stays quiet.
  std::vector<std::pair<int, int>> win_cells;
  for (auto [e, cell] : registry_.view<const Cell, const IsWin>().each()) {
    win_cells.emplace_back(cell.x, cell.y);
  }

  particle_emit_timer_ += dt;
  while (particle_emit_timer_ >= kEmitPeriod) {
    particle_emit_timer_ -= kEmitPeriod;
    if (win_cells.empty()) continue;
    const auto& wc = win_cells[GetRandomValue(0, static_cast<int>(win_cells.size()) - 1)];
    const Rectangle rect = board::CellRect(wc.first, wc.second);
    Particle p;
    p.pos = {
        rect.x + static_cast<float>(GetRandomValue(0, static_cast<int>(rect.width))),
        rect.y + static_cast<float>(GetRandomValue(0, static_cast<int>(rect.height))),
    };
    p.max_size = kMaxSize + static_cast<float>(GetRandomValue(-2, 4));
    p.life = p.max_life = kLifetime + 0.01f * GetRandomValue(-20, 20);
    p.rot_deg = static_cast<float>(GetRandomValue(0, 359));
    particles_.push_back(p);
  }
}

void GameLayer::DrawParticles() const {
  for (const auto& p : particles_) {
    const float age = 1.0f - (p.life / p.max_life);               // 0 -> 1
    const float pulse = 1.0f - std::abs(age - 0.5f) * 2.0f;        // 0 -> 1 -> 0
    const float size = p.max_size * pulse;
    if (size < 1.0f) continue;
    const float thick = std::max(1.5f, size * 0.22f);
    const unsigned char alpha = static_cast<unsigned char>(255.0f * (p.life / p.max_life));
    const Color color = {255, 240, 150, alpha};

    const Rectangle horiz = {p.pos.x, p.pos.y, size, thick};
    const Rectangle vert = {p.pos.x, p.pos.y, thick, size};
    DrawRectanglePro(horiz, {size * 0.5f, thick * 0.5f}, p.rot_deg, color);
    DrawRectanglePro(vert, {thick * 0.5f, size * 0.5f}, p.rot_deg, color);
  }
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

void GameLayer::LoadTrack(int index) {
  if (index < 0 || index >= kTrackCount) return;
  UnloadTrack();
  if (!IsAudioDeviceReady()) return;
  music_ = LoadMusicStream(kTracks[index].path);
  if (music_.stream.buffer == nullptr) return;
  music_.looping = true;
  music_loaded_ = true;
  track_index_ = index;
  SetMusicVolume(music_, muted_ ? 0.0f : volume_);
  PlayMusicStream(music_);
}

void GameLayer::UnloadTrack() {
  if (!music_loaded_) return;
  StopMusicStream(music_);
  UnloadMusicStream(music_);
  music_loaded_ = false;
}

// ---------------------------------------------------------------------------
// ImGui panels
// ---------------------------------------------------------------------------

const char* GameLayer::PrettyName(ObjectId id) {
  return InfoOf(id).name.data();
}

const char* GameLayer::PrettyName(TextId id) {
  return InfoOf(id).short_name.data();
}

void GameLayer::DrawScenePanel() {
  if (!ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  if (!level_.name.empty()) {
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "%s", level_.name.c_str());
    ImGui::Separator();
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
    DiscardClipboard();
    won_ = false;
  }

  ImGui::Separator();
  DrawToolbar();
  ImGui::Separator();
  DrawPalette();

  ImGui::Separator();
  ImGui::InputText("File", save_name_, sizeof(save_name_));
  if (ImGui::Button("Load")) {
    LoadLevelFromPath(std::filesystem::path{kLevelsDir} / save_name_);
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    SaveLevelToPath(std::filesystem::path{kLevelsDir} / save_name_);
  }

  ImGui::End();
}

namespace {

// Renders a tiny glyph centered in `rect`. Uses the window's draw list so we
// can stamp vector icons on top of a regular ImGui::Button frame.
void DrawToolGlyph(ImDrawList* dl, const ImVec2& center, float size, ImU32 col,
                   ToolGlyph glyph) {
  const float h = size * 0.5f;
  switch (glyph) {
    case ToolGlyph::Pen: {
      // A slanted pencil: body + tip.
      ImVec2 a{center.x - h * 0.7f, center.y + h * 0.7f};
      ImVec2 b{center.x + h * 0.3f, center.y - h * 0.3f};
      dl->AddLine(a, b, col, 2.2f);
      dl->AddLine({center.x + h * 0.3f, center.y - h * 0.3f},
                  {center.x + h * 0.7f, center.y - h * 0.7f}, col, 2.2f);
      break;
    }
    case ToolGlyph::Line: {
      dl->AddLine({center.x - h * 0.8f, center.y + h * 0.8f},
                  {center.x + h * 0.8f, center.y - h * 0.8f}, col, 2.0f);
      break;
    }
    case ToolGlyph::RectOutline: {
      dl->AddRect({center.x - h * 0.8f, center.y - h * 0.8f},
                  {center.x + h * 0.8f, center.y + h * 0.8f}, col, 0.0f, 0, 2.0f);
      break;
    }
    case ToolGlyph::RectFilled: {
      dl->AddRectFilled({center.x - h * 0.8f, center.y - h * 0.8f},
                        {center.x + h * 0.8f, center.y + h * 0.8f}, col);
      break;
    }
    case ToolGlyph::Select: {
      // Dashed rectangle: draw four gap-separated segments per side.
      const float x0 = center.x - h * 0.8f;
      const float y0 = center.y - h * 0.8f;
      const float x1 = center.x + h * 0.8f;
      const float y1 = center.y + h * 0.8f;
      const float step = (x1 - x0) / 5.0f;
      for (int i = 0; i < 5; i += 2) {
        dl->AddLine({x0 + step * i, y0}, {x0 + step * (i + 1), y0}, col, 1.6f);
        dl->AddLine({x0 + step * i, y1}, {x0 + step * (i + 1), y1}, col, 1.6f);
        dl->AddLine({x0, y0 + step * i}, {x0, y0 + step * (i + 1)}, col, 1.6f);
        dl->AddLine({x1, y0 + step * i}, {x1, y0 + step * (i + 1)}, col, 1.6f);
      }
      break;
    }
    case ToolGlyph::Bucket: {
      // Triangle bucket silhouette + drop.
      ImVec2 t0{center.x - h * 0.8f, center.y - h * 0.2f};
      ImVec2 t1{center.x + h * 0.8f, center.y - h * 0.2f};
      ImVec2 t2{center.x, center.y + h * 0.7f};
      dl->AddTriangle(t0, t1, t2, col, 1.6f);
      dl->AddCircleFilled({center.x + h * 0.7f, center.y + h * 0.6f}, h * 0.15f, col);
      break;
    }
    case ToolGlyph::Eraser: {
      // Rotated square with a diagonal line showing worn corner.
      ImVec2 pts[4] = {
          {center.x - h * 0.8f, center.y},
          {center.x, center.y - h * 0.8f},
          {center.x + h * 0.8f, center.y},
          {center.x, center.y + h * 0.8f},
      };
      dl->AddPolyline(pts, 4, col, ImDrawFlags_Closed, 1.8f);
      dl->AddLine({center.x - h * 0.4f, center.y - h * 0.4f},
                  {center.x + h * 0.4f, center.y + h * 0.4f}, col, 1.6f);
      break;
    }
  }
}

bool ToolIconButton(const char* id, ToolGlyph glyph, bool selected,
                    const char* tooltip, float size) {
  if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
  const bool clicked = ImGui::Button(id, {size, size});
  if (selected) ImGui::PopStyleColor();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 p0 = ImGui::GetItemRectMin();
  const ImVec2 p1 = ImGui::GetItemRectMax();
  const ImVec2 c{(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
  DrawToolGlyph(dl, c, size, ImGui::GetColorU32(ImGuiCol_Text), glyph);
  if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
  return clicked;
}

}  // namespace

void GameLayer::DrawToolbar() {
  ImGui::TextUnformatted("Tool");
  constexpr float kBtn = 32.0f;

  struct ToolEntry {
    Tool tool;
    ToolGlyph glyph;
    const char* id;
    const char* tip;
  };
  const ToolEntry entries[] = {
      {Tool::Brush, ToolGlyph::Pen, "##t_brush", "Brush (pen)"},
      {Tool::Line, ToolGlyph::Line, "##t_line", "Line"},
      {Tool::RectOutline, ToolGlyph::RectOutline, "##t_rect", "Rectangle outline"},
      {Tool::RectFilled, ToolGlyph::RectFilled, "##t_rectf", "Filled rectangle"},
      {Tool::Select, ToolGlyph::Select, "##t_sel", "Select / cut / paste"},
      {Tool::Bucket, ToolGlyph::Bucket, "##t_fill", "Paint bucket (flood)"},
      {Tool::Eraser, ToolGlyph::Eraser, "##t_erase", "Eraser"},
  };
  for (std::size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
    if (i > 0) ImGui::SameLine();
    const auto& e = entries[i];
    if (ToolIconButton(e.id, e.glyph, tool_ == e.tool, e.tip, kBtn)) {
      tool_ = e.tool;
      dragging_ = false;
      if (tool_ != Tool::Select) DiscardClipboard();
    }
  }

  // Eraser sub-mode selector: shape it reuses when Eraser is active.
  if (tool_ == Tool::Eraser) {
    ImGui::TextUnformatted("Eraser mode");
    struct ModeEntry {
      EraseMode mode;
      ToolGlyph glyph;
      const char* id;
      const char* tip;
    };
    const ModeEntry modes[] = {
        {EraseMode::Point, ToolGlyph::Pen, "##e_pt", "Single cell"},
        {EraseMode::Line, ToolGlyph::Line, "##e_ln", "Line"},
        {EraseMode::RectOutline, ToolGlyph::RectOutline, "##e_ro", "Rectangle outline"},
        {EraseMode::RectFilled, ToolGlyph::RectFilled, "##e_rf", "Filled rectangle"},
        {EraseMode::Bucket, ToolGlyph::Bucket, "##e_bk", "Flood fill erase"},
    };
    for (std::size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
      if (i > 0) ImGui::SameLine();
      const auto& m = modes[i];
      if (ToolIconButton(m.id, m.glyph, erase_mode_ == m.mode, m.tip, kBtn)) {
        erase_mode_ = m.mode;
        dragging_ = false;
      }
    }
  }

  if (tool_ == Tool::Select && !clipboard_.empty()) {
    ImGui::TextDisabled("%zu tiles in clipboard — LMB paste, RMB discard", clipboard_.size());
  }
}

void GameLayer::DrawPalette() {
  if (ImGui::BeginTabBar("##layers")) {
    const char* labels[] = {"Layer 1", "Layer 2", "Layer 3"};
    for (int i = 0; i < 3; ++i) {
      if (ImGui::BeginTabItem(labels[i])) {
        palette_layer_ = i;
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }

  constexpr float kBtn = 40.0f;
  const ImVec2 btn_size{kBtn, kBtn};
  int col = 0;
  const float avail = ImGui::GetContentRegionAvail().x;
  const int cols = std::max(1, static_cast<int>(avail / (kBtn + ImGui::GetStyle().ItemSpacing.x)));

  auto tint_to_imvec = [](Color c) {
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
  };

  auto button_for_object = [&](ObjectId id) {
    const Texture2D& tex = sprites_.Get(id, current_frame_,
                                        IsDirectional(id) ? DirectionToVariant(Direction::Right) : 0);
    const bool selected = std::holds_alternative<ObjectId>(brush_) && std::get<ObjectId>(brush_) == id;
    if (col > 0 && col < cols) ImGui::SameLine();
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
    ImGui::PushID(static_cast<int>(id));
    bool clicked = false;
    if (tex.id != 0) {
      clicked = ImGui::ImageButton("##obj", (ImTextureID)(intptr_t)tex.id, btn_size, {0, 0}, {1, 1},
                                   {0, 0, 0, 0}, tint_to_imvec(sprites_.TintFor(id)));
    } else {
      clicked = ImGui::Button(PrettyName(id), btn_size);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", PrettyName(id));
    ImGui::PopID();
    if (selected) ImGui::PopStyleColor();
    if (clicked) brush_ = id;
    col = (col + 1) % cols;
  };

  auto button_for_text = [&](TextId id) {
    const Texture2D& tex = sprites_.Get(id, current_frame_);
    const bool selected = std::holds_alternative<TextId>(brush_) && std::get<TextId>(brush_) == id;
    if (col > 0 && col < cols) ImGui::SameLine();
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
    ImGui::PushID(static_cast<int>(id) + 10000);
    bool clicked = false;
    if (tex.id != 0) {
      clicked = ImGui::ImageButton("##txt", (ImTextureID)(intptr_t)tex.id, btn_size, {0, 0}, {1, 1},
                                   {0, 0, 0, 0}, tint_to_imvec(sprites_.TintFor(id)));
    } else {
      clicked = ImGui::Button(PrettyName(id), btn_size);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", PrettyName(id));
    ImGui::PopID();
    if (selected) ImGui::PopStyleColor();
    if (clicked) brush_ = id;
    col = (col + 1) % cols;
  };

  if (palette_layer_ < 2) {
    // Split objects by engine layer: < 14 = backgrounds, >= 14 = objects.
    for (int i = 0; i < kObjectCount; ++i) {
      const auto id = static_cast<ObjectId>(i);
      const int lvl = InfoOf(id).layer;
      const bool is_bg = (lvl < 14);
      if ((palette_layer_ == 0) == is_bg) {
        button_for_object(id);
      }
    }
  } else {
    for (int i = 0; i < kTextCount; ++i) {
      button_for_text(static_cast<TextId>(i));
    }
  }
}

void GameLayer::DrawSettingsPanel() {
  if (!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  ImGui::SeparatorText("Input");
  ImGui::SliderFloat("Repeat delay", &repeat_delay_, 0.05f, 1.0f, "%.2f s");
  ImGui::TextDisabled("How long a direction must be held before auto-repeat starts.");
  ImGui::SliderFloat("Repeat interval", &repeat_interval_, 0.02f, 0.5f, "%.2f s");
  ImGui::TextDisabled("Time between repeated steps once auto-repeat kicks in.");

  ImGui::Spacing();
  ImGui::SeparatorText("Audio");
  const char* current_label = kTracks[std::clamp(track_index_, 0, kTrackCount - 1)].label;
  if (ImGui::BeginCombo("Track", current_label)) {
    for (int i = 0; i < kTrackCount; ++i) {
      const bool selected = (i == track_index_);
      if (ImGui::Selectable(kTracks[i].label, selected)) {
        LoadTrack(i);
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f");
  ImGui::Checkbox("Mute", &muted_);
  ImGui::SameLine();
  if (music_loaded_) {
    ImGui::TextDisabled("[loaded]");
  } else {
    ImGui::TextDisabled("[no track]");
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Animation");
  ImGui::Text("Sprite frame: %d", current_frame_);
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

  ImGui::End();
}

// ---------------------------------------------------------------------------
// Editor mouse
// ---------------------------------------------------------------------------

void GameLayer::DrawEditorOverlay() {
  auto cell_opt = board::ScreenToCell(GetMousePosition());
  const bool in_board = cell_opt.has_value();
  const int col = in_board ? cell_opt->first : 0;
  const int row = in_board ? cell_opt->second : 0;

  auto draw_brush_ghost = [&](int c, int r, Color tint_override, bool use_override) {
    const Rectangle rect = board::CellRect(c, r);
    if (std::holds_alternative<ObjectId>(brush_)) {
      const auto id = std::get<ObjectId>(brush_);
      Color tint = use_override ? tint_override : sprites_.TintFor(id);
      tint.a = 128;
      DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
    } else {
      const auto id = std::get<TextId>(brush_);
      Color tint = use_override ? tint_override : sprites_.TintFor(id);
      tint.a = 128;
      DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
    }
  };

  auto outline_cell = [](int c, int r, Color color, float thickness) {
    if (c < 0 || c >= board::kCols || r < 0 || r >= board::kRows) return;
    DrawRectangleLinesEx(board::CellRect(c, r), thickness, color);
  };

  // Hover highlight on the cell under the cursor.
  if (in_board) outline_cell(col, row, YELLOW, 2.0f);

  // Drag-preview overlays for shape-capable tools.
  const bool erasing = (tool_ == Tool::Eraser);
  const Color preview_col = erasing ? Color{235, 90, 90, 255} : YELLOW;

  auto preview_cells = [&](const std::vector<std::pair<int, int>>& cells) {
    for (const auto& [cx, cy] : cells) {
      outline_cell(cx, cy, preview_col, 2.0f);
      if (!erasing) draw_brush_ghost(cx, cy, preview_col, false);
    }
  };

  if (dragging_ && in_board) {
    std::vector<std::pair<int, int>> cells;
    switch (tool_) {
      case Tool::Line:
        cells = RasterLine(drag_start_x_, drag_start_y_, col, row);
        preview_cells(cells);
        break;
      case Tool::RectOutline:
        cells = RasterRectOutline(drag_start_x_, drag_start_y_, col, row);
        preview_cells(cells);
        break;
      case Tool::RectFilled:
        cells = RasterRectFilled(drag_start_x_, drag_start_y_, col, row);
        preview_cells(cells);
        break;
      case Tool::Select: {
        // Draw the current selection box (no ghost; box-select is cut on release).
        const int lx = std::min(drag_start_x_, col), rx = std::max(drag_start_x_, col);
        const int ly = std::min(drag_start_y_, row), ry = std::max(drag_start_y_, row);
        const Rectangle a = board::CellRect(lx, ly);
        const Rectangle b = board::CellRect(rx, ry);
        const Rectangle box{a.x, a.y, (b.x + b.width) - a.x, (b.y + b.height) - a.y};
        DrawRectangleLinesEx(box, 2.0f, SKYBLUE);
        break;
      }
      case Tool::Eraser: {
        switch (erase_mode_) {
          case EraseMode::Line:
            cells = RasterLine(drag_start_x_, drag_start_y_, col, row);
            break;
          case EraseMode::RectOutline:
            cells = RasterRectOutline(drag_start_x_, drag_start_y_, col, row);
            break;
          case EraseMode::RectFilled:
            cells = RasterRectFilled(drag_start_x_, drag_start_y_, col, row);
            break;
          default:
            break;
        }
        preview_cells(cells);
        break;
      }
      default:
        break;
    }
  } else if (tool_ == Tool::Select && !clipboard_.empty() && in_board) {
    // Show clipboard tiles as a ghost at the cursor (paste preview).
    for (const auto& t : clipboard_) {
      const int cx = col + t.dx;
      const int cy = row + t.dy;
      if (cx < 0 || cx >= board::kCols || cy < 0 || cy >= board::kRows) continue;
      const Rectangle rect = board::CellRect(cx, cy);
      outline_cell(cx, cy, SKYBLUE, 1.5f);
      if (std::holds_alternative<ObjectId>(t.kind)) {
        const auto id = std::get<ObjectId>(t.kind);
        Color tint = sprites_.TintFor(id);
        tint.a = 128;
        DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
      } else {
        const auto id = std::get<TextId>(t.kind);
        Color tint = sprites_.TintFor(id);
        tint.a = 128;
        DrawSpriteInCell(sprites_.Get(id, current_frame_), rect, tint);
      }
    }
  } else if (in_board && (tool_ == Tool::Brush || tool_ == Tool::Bucket ||
                          (tool_ == Tool::Eraser && erase_mode_ == EraseMode::Point))) {
    // Single-cell tools: show brush ghost (or red tint for point eraser).
    if (tool_ == Tool::Eraser) {
      outline_cell(col, row, Color{235, 90, 90, 255}, 2.0f);
    } else {
      draw_brush_ghost(col, row, WHITE, false);
    }
  }
}

void GameLayer::HandleEditorMouse() {
  if (ImGui::GetIO().WantCaptureMouse) {
    dragging_ = false;
    return;
  }
  auto cell_opt = board::ScreenToCell(GetMousePosition());
  const bool in_board = cell_opt.has_value();
  const int col = in_board ? cell_opt->first : 0;
  const int row = in_board ? cell_opt->second : 0;

  const bool lmb_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  const bool lmb_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
  const bool lmb_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  const bool rmb_pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
  const bool rmb_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

  auto commit_shape = [&](bool erase) {
    std::vector<std::pair<int, int>> cells;
    const EraseMode shape = erase ? erase_mode_ : EraseMode::Point;
    const Tool t = erase ? (shape == EraseMode::Line          ? Tool::Line
                            : shape == EraseMode::RectOutline ? Tool::RectOutline
                            : shape == EraseMode::RectFilled  ? Tool::RectFilled
                                                              : Tool::Brush)
                         : tool_;
    switch (t) {
      case Tool::Line:
        cells = RasterLine(drag_start_x_, drag_start_y_, col, row);
        break;
      case Tool::RectOutline:
        cells = RasterRectOutline(drag_start_x_, drag_start_y_, col, row);
        break;
      case Tool::RectFilled:
        cells = RasterRectFilled(drag_start_x_, drag_start_y_, col, row);
        break;
      default:
        return;
    }
    if (erase)
      EraseAtCells(cells);
    else
      PlaceAtCells(cells);
  };

  switch (tool_) {
    case Tool::Brush: {
      if (!in_board) return;
      if (lmb_down) PlaceBrushAt(col, row);
      else if (rmb_down) EraseAt(col, row);
      break;
    }
    case Tool::Line:
    case Tool::RectOutline:
    case Tool::RectFilled: {
      if (lmb_pressed && in_board) {
        dragging_ = true;
        drag_start_x_ = col;
        drag_start_y_ = row;
      }
      if (lmb_released && dragging_) {
        if (in_board) commit_shape(false);
        dragging_ = false;
      }
      if (rmb_pressed) dragging_ = false;  // cancel
      break;
    }
    case Tool::Select: {
      if (!clipboard_.empty()) {
        if (rmb_pressed) {
          DiscardClipboard();
        } else if (lmb_pressed && in_board) {
          PasteClipboardAt(col, row);
        }
        break;
      }
      if (lmb_pressed && in_board) {
        dragging_ = true;
        drag_start_x_ = col;
        drag_start_y_ = row;
      }
      if (lmb_released && dragging_) {
        if (in_board) CutRegionToClipboard(drag_start_x_, drag_start_y_, col, row);
        dragging_ = false;
      }
      if (rmb_pressed) dragging_ = false;
      break;
    }
    case Tool::Bucket: {
      if (lmb_pressed && in_board) {
        auto cells = FloodRegion(col, row);
        PlaceAtCells(cells);
      }
      break;
    }
    case Tool::Eraser: {
      switch (erase_mode_) {
        case EraseMode::Point: {
          if (!in_board) return;
          if (lmb_down) EraseAt(col, row);
          break;
        }
        case EraseMode::Line:
        case EraseMode::RectOutline:
        case EraseMode::RectFilled: {
          if (lmb_pressed && in_board) {
            dragging_ = true;
            drag_start_x_ = col;
            drag_start_y_ = row;
          }
          if (lmb_released && dragging_) {
            if (in_board) commit_shape(true);
            dragging_ = false;
          }
          if (rmb_pressed) dragging_ = false;
          break;
        }
        case EraseMode::Bucket: {
          if (lmb_pressed && in_board) {
            auto cells = FloodRegion(col, row);
            EraseAtCells(cells);
          }
          break;
        }
      }
      break;
    }
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

void GameLayer::PlaceAtCells(const std::vector<std::pair<int, int>>& cells) {
  for (const auto& [c, r] : cells) {
    if (c < 0 || c >= board::kCols || r < 0 || r >= board::kRows) continue;
    PlaceBrushAt(c, r);
  }
}

void GameLayer::EraseAtCells(const std::vector<std::pair<int, int>>& cells) {
  for (const auto& [c, r] : cells) {
    if (c < 0 || c >= board::kCols || r < 0 || r >= board::kRows) continue;
    EraseAt(c, r);
  }
}

std::vector<std::pair<int, int>> GameLayer::RasterLine(int x0, int y0, int x1, int y1) {
  // Bresenham's line algorithm. Generates one cell per step.
  std::vector<std::pair<int, int>> out;
  int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int x = x0, y = y0;
  for (;;) {
    out.emplace_back(x, y);
    if (x == x1 && y == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y += sy;
    }
  }
  return out;
}

std::vector<std::pair<int, int>> GameLayer::RasterRectOutline(int x0, int y0, int x1, int y1) {
  std::vector<std::pair<int, int>> out;
  const int lx = std::min(x0, x1), rx = std::max(x0, x1);
  const int ly = std::min(y0, y1), ry = std::max(y0, y1);
  for (int x = lx; x <= rx; ++x) {
    out.emplace_back(x, ly);
    if (ry != ly) out.emplace_back(x, ry);
  }
  for (int y = ly + 1; y <= ry - 1; ++y) {
    out.emplace_back(lx, y);
    if (rx != lx) out.emplace_back(rx, y);
  }
  return out;
}

std::vector<std::pair<int, int>> GameLayer::RasterRectFilled(int x0, int y0, int x1, int y1) {
  std::vector<std::pair<int, int>> out;
  const int lx = std::min(x0, x1), rx = std::max(x0, x1);
  const int ly = std::min(y0, y1), ry = std::max(y0, y1);
  for (int y = ly; y <= ry; ++y)
    for (int x = lx; x <= rx; ++x) out.emplace_back(x, y);
  return out;
}

std::vector<std::variant<ObjectId, TextId>> GameLayer::CellContents(int x, int y) const {
  std::vector<std::variant<ObjectId, TextId>> out;
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    if (cell.x == x && cell.y == y) out.emplace_back(kind.id);
  }
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    if (cell.x == x && cell.y == y) out.emplace_back(text.id);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<std::pair<int, int>> GameLayer::FloodRegion(int sx, int sy) const {
  if (sx < 0 || sx >= board::kCols || sy < 0 || sy >= board::kRows) return {};
  const auto target = CellContents(sx, sy);

  std::vector<std::vector<unsigned char>> seen(board::kRows,
                                               std::vector<unsigned char>(board::kCols, 0));
  std::vector<std::pair<int, int>> out, stack;
  stack.emplace_back(sx, sy);
  seen[sy][sx] = 1;
  while (!stack.empty()) {
    auto [cx, cy] = stack.back();
    stack.pop_back();
    if (CellContents(cx, cy) != target) continue;
    out.emplace_back(cx, cy);
    const int nbr[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& d : nbr) {
      const int nx = cx + d[0], ny = cy + d[1];
      if (nx < 0 || nx >= board::kCols || ny < 0 || ny >= board::kRows) continue;
      if (seen[ny][nx]) continue;
      seen[ny][nx] = 1;
      stack.emplace_back(nx, ny);
    }
  }
  return out;
}

void GameLayer::CutRegionToClipboard(int x0, int y0, int x1, int y1) {
  DiscardClipboard();
  const int lx = std::min(x0, x1), rx = std::max(x0, x1);
  const int ly = std::min(y0, y1), ry = std::max(y0, y1);

  std::vector<entt::entity> doomed;
  for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
    if (cell.x >= lx && cell.x <= rx && cell.y >= ly && cell.y <= ry) {
      clipboard_.push_back({cell.x - lx, cell.y - ly, kind.id});
      doomed.push_back(e);
    }
  }
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    if (cell.x >= lx && cell.x <= rx && cell.y >= ly && cell.y <= ry) {
      clipboard_.push_back({cell.x - lx, cell.y - ly, text.id});
      doomed.push_back(e);
    }
  }
  for (auto e : doomed) registry_.destroy(e);
  undo_.Clear();
}

void GameLayer::PasteClipboardAt(int col, int row) {
  for (const auto& t : clipboard_) {
    const int x = col + t.dx;
    const int y = row + t.dy;
    if (x < 0 || x >= board::kCols || y < 0 || y >= board::kRows) continue;
    if (std::holds_alternative<ObjectId>(t.kind)) {
      const auto id = std::get<ObjectId>(t.kind);
      bool exists = false;
      for (auto [e, cell, kind] : registry_.view<const Cell, const Kind>().each()) {
        if (cell.x == x && cell.y == y && kind.id == id) {
          exists = true;
          break;
        }
      }
      if (!exists) SpawnObject(id, x, y);
    } else {
      const auto id = std::get<TextId>(t.kind);
      bool exists = false;
      for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
        if (cell.x == x && cell.y == y && text.id == id) {
          exists = true;
          break;
        }
      }
      if (!exists) SpawnText(id, x, y);
    }
  }
  undo_.Clear();
}

void GameLayer::DiscardClipboard() { clipboard_.clear(); }

// ---------------------------------------------------------------------------
// World / progression
// ---------------------------------------------------------------------------

void GameLayer::MarkLevelCompleted(const std::string& id) {
  const bool added = progress_.completed.insert(id).second;
  if (added) SaveProgress(kProgressFile, progress_);
}

bool GameLayer::IsUnlocked(std::size_t level_index) const {
  if (level_index == 0) return true;
  if (level_index >= world_.levels.size()) return false;
  return progress_.completed.contains(world_.levels[level_index - 1].id);
}

void GameLayer::DrawWorldPanel() {
  if (!ImGui::Begin("World", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("Built-in", ImGuiTreeNodeFlags_DefaultOpen)) {
    const char* lvl_label = kBuiltinLevels[std::clamp(selected_level_, 0, kBuiltinLevelCount - 1)].label;
    if (ImGui::BeginCombo("##builtin", lvl_label)) {
      for (int i = 0; i < kBuiltinLevelCount; ++i) {
        const bool selected = (i == selected_level_);
        if (ImGui::Selectable(kBuiltinLevels[i].label, selected)) {
          selected_level_ = i;
          LoadLevelFromPath(std::filesystem::path{kLevelsDir} / kBuiltinLevels[i].file);
        }
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    if (ImGui::Button("Prev##b")) {
      selected_level_ = (selected_level_ - 1 + kBuiltinLevelCount) % kBuiltinLevelCount;
      LoadLevelFromPath(std::filesystem::path{kLevelsDir} / kBuiltinLevels[selected_level_].file);
    }
    ImGui::SameLine();
    if (ImGui::Button("Next##b")) {
      selected_level_ = (selected_level_ + 1) % kBuiltinLevelCount;
      LoadLevelFromPath(std::filesystem::path{kLevelsDir} / kBuiltinLevels[selected_level_].file);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Starter")) {
      LoadLevelFromPath(kDefaultLevel);
    }
  }

  if (!imported_stems_.empty() && ImGui::CollapsingHeader("Imported")) {
    ImGui::Text("(%zu levels)", imported_stems_.size());
    ImGui::InputTextWithHint("##filter", "filter (e.g. 12 or lev)",
                             imported_filter_, sizeof(imported_filter_));
    // Case-insensitive substring match on the filter text.
    auto matches = [&](const std::string& s) {
      if (imported_filter_[0] == '\0') return true;
      auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
      std::string a(s.size(), ' '), b(std::strlen(imported_filter_), ' ');
      std::transform(s.begin(), s.end(), a.begin(), lower);
      std::transform(imported_filter_, imported_filter_ + b.size(), b.begin(), lower);
      return a.find(b) != std::string::npos;
    };
    imported_index_ = std::clamp(imported_index_, 0, static_cast<int>(imported_stems_.size()) - 1);
    const char* cur = imported_stems_[imported_index_].c_str();
    if (ImGui::BeginCombo("##imported", cur)) {
      for (int i = 0; i < static_cast<int>(imported_stems_.size()); ++i) {
        if (!matches(imported_stems_[i])) continue;
        const bool selected = (i == imported_index_);
        if (ImGui::Selectable(imported_stems_[i].c_str(), selected)) {
          imported_index_ = i;
          LoadLevelFromPath(std::filesystem::path{kImportedDir} /
                            (imported_stems_[i] + ".json"));
        }
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load##imp")) {
      LoadLevelFromPath(std::filesystem::path{kImportedDir} /
                        (imported_stems_[imported_index_] + ".json"));
    }
    if (ImGui::Button("Prev##imp")) {
      const int n = static_cast<int>(imported_stems_.size());
      imported_index_ = (imported_index_ - 1 + n) % n;
      LoadLevelFromPath(std::filesystem::path{kImportedDir} /
                        (imported_stems_[imported_index_] + ".json"));
    }
    ImGui::SameLine();
    if (ImGui::Button("Next##imp")) {
      const int n = static_cast<int>(imported_stems_.size());
      imported_index_ = (imported_index_ + 1) % n;
      LoadLevelFromPath(std::filesystem::path{kImportedDir} /
                        (imported_stems_[imported_index_] + ".json"));
    }
  }

  if (!world_.levels.empty() && ImGui::CollapsingHeader("Campaign", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("%s", world_.title.empty() ? "Levels" : world_.title.c_str());
    const std::size_t done = progress_.completed.size();
    ImGui::TextDisabled("Cleared %zu / %zu", done, world_.levels.size());
    ImGui::Separator();

    for (std::size_t i = 0; i < world_.levels.size(); ++i) {
      const auto& lvl = world_.levels[i];
      const bool unlocked = IsUnlocked(i);
      const bool completed = progress_.completed.contains(lvl.id);
      const bool current = (lvl.id == current_level_id_);

      ImGui::PushID(static_cast<int>(i));
      const char* badge = completed ? "[*]" : (unlocked ? "[ ]" : "[X]");
      if (current) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s %s", badge, lvl.label.c_str());
      } else if (unlocked) {
        ImGui::Text("%s %s", badge, lvl.label.c_str());
      } else {
        ImGui::TextDisabled("%s %s", badge, lvl.label.c_str());
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(!unlocked);
      if (ImGui::Button("Play")) {
        LoadLevelFromPath(std::filesystem::path{kLevelsDir} / (lvl.id + ".json"));
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Progress")) {
      progress_.completed.clear();
      SaveProgress(kProgressFile, progress_);
      win_handled_ = false;
    }
  }

  ImGui::End();
}
