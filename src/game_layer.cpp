#include "game_layer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include <imgui.h>
#include <raylib.h>

#include "board.h"
#include "components.h"

namespace {

constexpr float kAnimFps = 6.0f;  // cycle 1->2->3 every ~0.5s
constexpr const char* kDefaultLevel = "assets/levels/001.json";
constexpr const char* kSpritesDir = "assets/sprites";
constexpr const char* kLevelsDir = "assets/levels";
constexpr const char* kImportedDir = "assets/imported";
constexpr const char* kWorldFile = "assets/worlds/tutorial.json";
constexpr const char* kProgressFile = "progress.json";

constexpr const char* kStepSoundPath = "assets/sfx/044.ogg";
constexpr const char* kWinSoundPath = "assets/sfx/021.ogg";
constexpr const char* kDeadMusicPath = "assets/sfx/037.ogg";
constexpr const char* kDefeatSoundPath = "assets/sfx/100.ogg";
constexpr const char* kSinkSoundPath = "assets/sfx/106.ogg";

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

std::string ResolveCampaignLevelName(const WorldLevel& lvl) {
  Level loaded;
  const auto path = std::filesystem::path{kLevelsDir} / (lvl.id + ".json");
  if (LoadLevelFromJson(path, loaded) && !loaded.name.empty()) return loaded.name;
  return lvl.id;
}

std::optional<int> ParseLevelNumber(const char* text) {
  if (!text || text[0] == '\0') return std::nullopt;
  for (std::size_t i = 0; text[i] != '\0'; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(text[i]))) return std::nullopt;
  }
  return std::stoi(text);
}

std::string LevelFileFromNumber(int level) {
  if (level < 0) level = 0;
  if (level > 999) level = 999;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%03d.json", level);
  return buf;
}

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
    for (auto [e, cell, object] : registry.view<const Cell, const ObjectBlock>().each()) {
      if (cell.x == nx && cell.y == ny && object.id == id) return true;
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
  RefreshBuiltInLevels();
  // Editor toolbar icons. Indices must match the Tool enum order.
  const char* kToolIconPaths[7] = {
      "icon/brush.png",        // Tool::Brush
      "icon/line.png",         // Tool::Line
      "icon/rect-outline.png", // Tool::RectOutline
      "icon/rect-full.png",    // Tool::RectFilled
      "icon/select.png",       // Tool::Select
      "icon/paint.png",        // Tool::Bucket
      "icon/eraser.png",       // Tool::Eraser
  };
  for (std::size_t i = 0; i < tool_icons_.size(); ++i) {
    tool_icons_[i] = LoadTexture(kToolIconPaths[i]);
    if (tool_icons_[i].id != 0) SetTextureFilter(tool_icons_[i], TEXTURE_FILTER_POINT);
  }
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
  // Boot into level 0 of the world (e.g. tutorial's "01-intro"). If the world
  // file is missing or empty, fall back to the bundled starter level.
  if (!world_.levels.empty()) {
    LoadLevelFromPath(std::filesystem::path{kLevelsDir} / (world_.levels[0].id + ".json"));
  } else {
    LoadLevelFromPath(kDefaultLevel);
  }
  LoadTrack(track_index_);
  if (IsAudioDeviceReady()) {
    step_sound_ = LoadSound(kStepSoundPath);
    step_sound_loaded_ = (step_sound_.frameCount > 0);
    if (step_sound_loaded_) SetSoundVolume(step_sound_, sfx_volume_);
    win_sound_ = LoadSound(kWinSoundPath);
    win_sound_loaded_ = (win_sound_.frameCount > 0);
    if (win_sound_loaded_) SetSoundVolume(win_sound_, sfx_volume_);
    defeat_sound_ = LoadSound(kDefeatSoundPath);
    defeat_sound_loaded_ = (defeat_sound_.frameCount > 0);
    if (defeat_sound_loaded_) SetSoundVolume(defeat_sound_, sfx_volume_);
    sink_sound_ = LoadSound(kSinkSoundPath);
    sink_sound_loaded_ = (sink_sound_.frameCount > 0);
    if (sink_sound_loaded_) SetSoundVolume(sink_sound_, sfx_volume_);
    dead_music_ = LoadMusicStream(kDeadMusicPath);
    if (dead_music_.stream.buffer != nullptr) {
      dead_music_.looping = true;
      dead_music_loaded_ = true;
      SetMusicVolume(dead_music_, muted_ ? 0.0f : volume_);
    }
  }
}

void GameLayer::OnDetach() {
  sprites_.Unload();
  for (auto& t : tool_icons_) {
    if (t.id != 0) UnloadTexture(t);
    t = Texture2D{};
  }
  registry_.clear();
  UnloadTrack();
  if (step_sound_loaded_) {
    UnloadSound(step_sound_);
    step_sound_loaded_ = false;
  }
  if (win_sound_loaded_) {
    UnloadSound(win_sound_);
    win_sound_loaded_ = false;
  }
  if (defeat_sound_loaded_) {
    UnloadSound(defeat_sound_);
    defeat_sound_loaded_ = false;
  }
  if (sink_sound_loaded_) {
    UnloadSound(sink_sound_);
    sink_sound_loaded_ = false;
  }
  if (dead_music_loaded_) {
    StopMusicStream(dead_music_);
    UnloadMusicStream(dead_music_);
    dead_music_loaded_ = false;
  }
}

void GameLayer::OnUpdate(float dt) {
  // Animation frame cycle (always on, independent of turn).
  anim_timer_ += dt;
  const float period = 1.0f / kAnimFps;
  while (anim_timer_ >= period) {
    anim_timer_ -= period;
    current_frame_ = (current_frame_ % 3) + 1;
  }
  // Walk-cycle decay: if a directional walker hasn't stepped recently, return
  // to the idle pose (variant offset 0) so they don't freeze mid-stride.
  constexpr float kWalkSettleSeconds = 0.18f;
  for (auto [e, af] : registry_.view<AnimFrame>().each()) {
    af.since_step += dt;
    if (af.since_step > kWalkSettleSeconds) af.walk_phase = 0;
  }

  // Iris transition: collapse the elliptical opening, swap level at full
  // black, then re-open it. Input and rule updates are suspended while a
  // transition runs so the player can't move during it.
  if (transition_state_ != TransitionState::None) {
    transition_t_ += dt / kTransitionPhaseSeconds;
    if (transition_state_ == TransitionState::Closing && transition_t_ >= 1.0f) {
      LoadLevelFromPath(std::filesystem::path{kLevelsDir} / (transition_target_ + ".json"));
      transition_state_ = TransitionState::Opening;
      transition_t_ = 0.0f;
    } else if (transition_state_ == TransitionState::Opening && transition_t_ >= 1.0f) {
      transition_state_ = TransitionState::None;
      transition_t_ = 0.0f;
      transition_target_.clear();
    }
    if (playing_dead_ && dead_music_loaded_) {
      SetMusicVolume(dead_music_, muted_ ? 0.0f : volume_);
      UpdateMusicStream(dead_music_);
    } else if (music_loaded_) {
      SetMusicVolume(music_, muted_ ? 0.0f : volume_);
      UpdateMusicStream(music_);
    }
    return;
  }

  // Recompute rules every frame so the editor sees live feedback.
  RecomputeRules();

  // Stuck-state ambience: any frame where no entity holds IsYou (everyone
  // dead, or "X is you" never written / broken) plays a separate music
  // stream. Restoring an IsYou via Z-undo flips back automatically.
  bool any_you = false;
  for (auto e : registry_.view<const IsYou>()) {
    (void)e;
    any_you = true;
    break;
  }
  if (!any_you && !playing_dead_) {
    if (music_loaded_) PauseMusicStream(music_);
    if (dead_music_loaded_) {
      SeekMusicStream(dead_music_, 0.0f);
      PlayMusicStream(dead_music_);
    }
    playing_dead_ = true;
  } else if (any_you && playing_dead_) {
    if (dead_music_loaded_) StopMusicStream(dead_music_);
    if (music_loaded_) ResumeMusicStream(music_);
    playing_dead_ = false;
  }

  UpdateParticles(dt);

  if (playing_dead_ && dead_music_loaded_) {
    SetMusicVolume(dead_music_, muted_ ? 0.0f : volume_);
    UpdateMusicStream(dead_music_);
  } else if (music_loaded_) {
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
    // Mouse handling for edit mode now runs from OnImGuiRender (after
    // ImGui::NewFrame) so ImGui's WantCaptureMouse is current, not 1 frame
    // stale. Bail out of the gameplay input path regardless.
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
  auto draw_object = [&](entt::entity e, const Cell& cell, const ObjectBlock& object) {
    int variant = 0;
    int frame = current_frame_;
    if (IsAutoTiled(object.id)) {
      variant = ComputeTileMask(registry_, object.id, cell.x, cell.y);
    } else if (IsDirectional(object.id)) {
      const Direction dir = registry_.try_get<Facing>(e) ? registry_.get<Facing>(e).dir : Direction::Right;
      // Sprites are laid out as 4 direction-bases × 8 walk-phase offsets in
      // baba_aa_b.png — pick the right walk pose within the current facing.
      const int base = DirectionToVariant(dir);
      const int phase = registry_.try_get<AnimFrame>(e) ? registry_.get<AnimFrame>(e).walk_phase : 0;
      variant = base + phase;
    }
    const auto& tex = sprites_.Get(object.id, frame, variant);
    DrawSpriteInCell(tex, board::CellRect(cell.x, cell.y), sprites_.TintFor(object.id));
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
  for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
    draws.push_back({LayerOf(object.id), e, cell.x, cell.y, 0});
  }
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    draws.push_back({LayerOf(text.id), e, cell.x, cell.y, 1});
  }
  std::sort(draws.begin(), draws.end(),
            [](const Draw& a, const Draw& b) { return a.layer < b.layer; });
  for (const Draw& d : draws) {
    if (d.kind == 0) {
      const auto& object = registry_.get<const ObjectBlock>(d.e);
      draw_object(d.e, registry_.get<const Cell>(d.e), object);
    } else {
      const auto& text = registry_.get<const TextBlock>(d.e);
      const auto& tex = sprites_.Get(text.id, current_frame_);
      Color tint = sprites_.TintFor(text.id);
      if (!registry_.all_of<IsRuleActiveText>(d.e)) {
        tint.r = static_cast<unsigned char>(tint.r * 0.60f);
        tint.g = static_cast<unsigned char>(tint.g * 0.60f);
        tint.b = static_cast<unsigned char>(tint.b * 0.60f);
      }
      DrawSpriteInCell(tex, board::CellRect(d.x, d.y), tint);
    }
  }
  // 4) Particles (sparkles for IsWin entities).
  DrawParticles();

  if (edit_mode_) {
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
  if (edit_mode_) {
    HandleEditorMouse();
  }

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

  if (show_scene_panel_) DrawScenePanel();
  if (show_rules_panel_) DrawRulesPanel();
  if (show_editor_panel_) DrawEditorPanel();
  if (show_world_panel_) DrawWorldPanel();
  if (show_settings_panel_) DrawSettingsPanel();
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
  if (!current_level_id_.empty() &&
      std::all_of(current_level_id_.begin(), current_level_id_.end(),
                  [](unsigned char c) { return std::isdigit(c) != 0; })) {
    std::snprintf(level_id_input_, sizeof(level_id_input_), "%d", std::stoi(current_level_id_));
  }
  std::strncpy(level_name_, level_.name.c_str(), sizeof(level_name_) - 1);
  level_name_[sizeof(level_name_) - 1] = '\0';
  win_handled_ = false;
  BuildRegistryFromLevel();
  undo_.Clear();
  won_ = false;
  RecomputeRules(true);
}

void GameLayer::ResetToInitial() {
  level_ = initial_level_;
  win_handled_ = false;
  BuildRegistryFromLevel();
  undo_.Clear();
  won_ = false;
  RecomputeRules(true);
}

void GameLayer::SaveLevelToPath(const std::filesystem::path& path) {
  Level current = ExtractLevelFromRegistry();
  current.name = level_.name;
  current.bg_r = level_.bg_r;
  current.bg_g = level_.bg_g;
  current.bg_b = level_.bg_b;
  current.edge_r = level_.edge_r;
  current.edge_g = level_.edge_g;
  current.edge_b = level_.edge_b;
  if (!SaveLevelToJson(path, current)) {
    TraceLog(LOG_WARNING, "Failed to save level: %s", path.string().c_str());
    return;
  }
  level_ = current;
  initial_level_ = current;
  current_level_id_ = path.stem().string();
  RefreshBuiltInLevels();
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
  lvl.name = level_.name;
  lvl.bg_r = level_.bg_r;
  lvl.bg_g = level_.bg_g;
  lvl.bg_b = level_.bg_b;
  lvl.edge_r = level_.edge_r;
  lvl.edge_g = level_.edge_g;
  lvl.edge_b = level_.edge_b;
  for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
    LevelTile tile;
    tile.x = cell.x;
    tile.y = cell.y;
    tile.kind = object.id;
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
  registry_.emplace<ObjectBlock>(e, id);
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

void GameLayer::RecomputeRules(bool apply_transformations) {
  RuleBoard rb(board::kCols, board::kRows);
  for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
    rb.Set(cell.x, cell.y, text.id);
  }
  registry_.clear<IsRuleActiveText>();
  rules_ = ParseRules(rb);
  auto mark_text_at = [&](int x, int y) {
    for (auto [e, cell, text] : registry_.view<const Cell, const TextBlock>().each()) {
      if (cell.x == x && cell.y == y) {
        registry_.emplace_or_replace<IsRuleActiveText>(e);
      }
    }
  };
  for (int y = 0; y < rb.Rows(); ++y) {
    for (int x = 0; x + 2 < rb.Cols(); ++x) {
      const auto a = rb.At(x, y);
      const auto b = rb.At(x + 1, y);
      const auto c = rb.At(x + 2, y);
      if (!a || !b || !c) continue;
      if (CategoryOf(*a) != TextCategory::Noun) continue;
      if (*b != TextId::Is) continue;
      const auto pred_cat = CategoryOf(*c);
      if (pred_cat != TextCategory::Noun && pred_cat != TextCategory::Property) continue;
      mark_text_at(x, y);
      mark_text_at(x + 1, y);
      mark_text_at(x + 2, y);
    }
  }
  for (int x = 0; x < rb.Cols(); ++x) {
    for (int y = 0; y + 2 < rb.Rows(); ++y) {
      const auto a = rb.At(x, y);
      const auto b = rb.At(x, y + 1);
      const auto c = rb.At(x, y + 2);
      if (!a || !b || !c) continue;
      if (CategoryOf(*a) != TextCategory::Noun) continue;
      if (*b != TextId::Is) continue;
      const auto pred_cat = CategoryOf(*c);
      if (pred_cat != TextCategory::Noun && pred_cat != TextCategory::Property) continue;
      mark_text_at(x, y);
      mark_text_at(x, y + 1);
      mark_text_at(x, y + 2);
    }
  }
  if (apply_transformations && ApplyTransformations(registry_, rules_)) {
    // ObjectBlock.ids changed — re-parse just to be safe (text positions are
    // unchanged, but the rule list is the same so this is essentially free)
    // and re-apply tags so they match the new id distribution.
  }
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
  const int from_x = c.x;
  const int from_y = c.y;
  PushChain(tx, ty, dx, dy);
  auto& mut = registry_.get<Cell>(who);
  mut.x = tx;
  mut.y = ty;
  if (auto* facing = registry_.try_get<Facing>(who)) facing->dir = dir;
  // Step the walk frame so directional sprites visibly animate per-move.
  if (auto* af = registry_.try_get<AnimFrame>(who)) {
    af->walk_phase = (af->walk_phase + 1) % 4;
    af->since_step = 0.0f;
  }
  // Dust puff at the vacated cell — only for directional walkers (Baba & co.)
  // so pushed boxes don't spam particles.
  if (auto* obj = registry_.try_get<ObjectBlock>(who); obj && IsDirectional(obj->id)) {
    SpawnSmokeAt(from_x, from_y, obj->id);
    if (step_sound_loaded_ && !muted_) PlaySound(step_sound_);
  }
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

  // Autonomous movers run every turn, regardless of whether YOU moved.
  if (StepMovers()) any_moved = true;

  if (!any_moved) {
    // Discard the no-op snapshot to avoid spamming undo history.
    Snapshot discard;
    undo_.Pop(discard);
  }

  RecomputeRules(true);
  RunWinDefeat();
}

bool GameLayer::StepMovers() {
  // Snapshot the mover set up-front: transformations during this turn could
  // shift ids around, but we only autonomously move entities that were tagged
  // IsMove at the start of the move phase.
  std::vector<entt::entity> movers;
  for (auto e : registry_.view<const IsMove>()) movers.push_back(e);

  bool any = false;
  for (auto e : movers) {
    auto* facing = registry_.try_get<Facing>(e);
    if (!facing) continue;
    if (TryMove(e, facing->dir)) {
      any = true;
    } else {
      // Bump into something — reverse and wait until next turn to try the
      // new direction. Matches Baba Is You's "move bounce" behaviour.
      switch (facing->dir) {
        case Direction::Right: facing->dir = Direction::Left; break;
        case Direction::Left:  facing->dir = Direction::Right; break;
        case Direction::Up:    facing->dir = Direction::Down; break;
        case Direction::Down:  facing->dir = Direction::Up; break;
      }
    }
  }
  return any;
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
          if (win_sound_loaded_ && !muted_) PlaySound(win_sound_);
          // Kick off the eye-blink transition to the next world level. If
          // we're already on the last level there's nothing to advance to —
          // just stay on the win screen.
          if (transition_state_ == TransitionState::None) {
            if (auto next = NextLevelId()) {
              transition_state_ = TransitionState::Closing;
              transition_t_ = 0.0f;
              transition_target_ = *next;
            }
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
  if (!doomed.empty() && defeat_sound_loaded_ && !muted_) PlaySound(defeat_sound_);

  // SINK: any cell where a SINK entity overlaps a different non-empty entity
  // sinks both. Mirrors Baba Is You: water swallows what walks into it
  // (and the water tile too). Pure SINK-on-SINK stacks don't trigger.
  std::vector<entt::entity> sunk;
  auto sink_view = registry_.view<const Cell, const IsSink>();
  for (auto [se, sc] : sink_view.each()) {
    bool has_partner = false;
    for (auto [other, oc] : registry_.view<const Cell>().each()) {
      if (other == se) continue;
      if (registry_.all_of<IsSink>(other)) continue;
      if (oc.x != sc.x || oc.y != sc.y) continue;
      has_partner = true;
      sunk.push_back(other);
    }
    if (has_partner) sunk.push_back(se);
  }
  // De-duplicate (a SINK over multiple partners gets queued once per pair).
  std::sort(sunk.begin(), sunk.end());
  sunk.erase(std::unique(sunk.begin(), sunk.end()), sunk.end());
  for (auto e : sunk) {
    if (registry_.valid(e)) registry_.destroy(e);
  }
  if (!sunk.empty() && sink_sound_loaded_ && !muted_) PlaySound(sink_sound_);
}

std::optional<std::string> GameLayer::NextLevelId() const {
  if (current_level_id_.empty() || world_.levels.empty()) return std::nullopt;
  for (std::size_t i = 0; i + 1 < world_.levels.size(); ++i) {
    if (world_.levels[i].id == current_level_id_) {
      return world_.levels[i + 1].id;
    }
  }
  return std::nullopt;
}

void GameLayer::DrawTransitionOverlay() const {
  if (transition_state_ == TransitionState::None) return;
  // `cover` ramps 0 -> 1 while Closing (eye shuts) and 1 -> 0 while Opening
  // (eye re-opens). Smoothstep gives the eye-blink a soft accel/settle
  // instead of a linear slide.
  float cover = transition_state_ == TransitionState::Closing ? transition_t_
                                                              : (1.0f - transition_t_);
  cover = std::clamp(cover, 0.0f, 1.0f);
  cover = cover * cover * (3.0f - 2.0f * cover);

  const float w = static_cast<float>(GetScreenWidth());
  const float h = static_cast<float>(GetScreenHeight());
  const float cx = w * 0.5f;
  const float cy = h * 0.5f;

  // Eye-shaped opening: an ellipse with the same aspect as the screen, sized
  // so the fully-open shape circumscribes the screen rectangle (axes 0.75 of
  // each dimension safely contains the corners). At cover = 1 it collapses
  // to a point.
  const float openA = w * 0.75f;
  const float openB = h * 0.75f;
  const float a = openA * (1.0f - cover);
  const float b = openB * (1.0f - cover);

  // Outer skirt — far enough that the black fan reaches every corner from
  // every angle.
  const float farR = std::max(w, h) * 1.5f;

  // Build the dark region as a strip of triangles between the ellipse and
  // a far ring. raylib's 2D mode doesn't backface-cull, so winding doesn't
  // matter here.
  constexpr int kSegments = 96;
  for (int i = 0; i < kSegments; ++i) {
    const float t0 = static_cast<float>(i) / kSegments * 2.0f * PI;
    const float t1 = static_cast<float>(i + 1) / kSegments * 2.0f * PI;
    const float c0 = std::cos(t0), s0 = std::sin(t0);
    const float c1 = std::cos(t1), s1 = std::sin(t1);
    const Vector2 inner0{cx + a * c0, cy + b * s0};
    const Vector2 inner1{cx + a * c1, cy + b * s1};
    const Vector2 outer0{cx + farR * c0, cy + farR * s0};
    const Vector2 outer1{cx + farR * c1, cy + farR * s1};
    DrawTriangle(inner0, outer0, inner1, BLACK);
    DrawTriangle(inner1, outer0, outer1, BLACK);
  }
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
    const float t = 1.0f - (p.life / p.max_life);  // 0 -> 1 over lifetime
    if (p.style == ParticleStyle::Smoke) {
      // Pixel-art exhaust: chunky square puffs, expanding and fading.
      const float size = p.max_size * (0.55f + 0.75f * t);
      const float px = std::max(1.0f, std::round(size * 0.5f));
      Color c = p.color;
      c.a = static_cast<unsigned char>(190.0f * (1.0f - t));
      const Rectangle a{std::round(p.pos.x - px), std::round(p.pos.y - px), px, px};
      const Rectangle b{std::round(p.pos.x), std::round(p.pos.y - px * 0.5f), px, px};
      DrawRectangleRec(a, c);
      DrawRectangleRec(b, c);
      continue;
    }
    // Sparkle: pulse + spin.
    const float pulse = 1.0f - std::abs(t - 0.5f) * 2.0f;        // 0 -> 1 -> 0
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

void GameLayer::SpawnSmokeAt(int cell_x, int cell_y, ObjectId source_id) {
  const Rectangle r = board::CellRect(cell_x, cell_y);
  const Color base = sprites_.TintFor(source_id);
  // Two or three little puffs so a step reads as a small cloud, not a single dot.
  const int n = GetRandomValue(2, 3);
  for (int i = 0; i < n; ++i) {
    Particle p;
    p.pos = {
        r.x + r.width * 0.5f + static_cast<float>(GetRandomValue(-6, 6)),
        r.y + r.height * 0.7f + static_cast<float>(GetRandomValue(-4, 4)),
    };
    p.max_size = r.width * 0.18f + static_cast<float>(GetRandomValue(-2, 3));
    p.life = p.max_life = 0.35f + 0.01f * GetRandomValue(-5, 8);
    p.rot_deg = 0.0f;
    p.color = base;
    p.style = ParticleStyle::Smoke;
    particles_.push_back(p);
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
  // Mid-stuck-state track changes: keep dead ambience audible by parking
  // the new track in a paused state. Resume happens when an IsYou returns.
  if (playing_dead_) PauseMusicStream(music_);
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

  int object_count = static_cast<int>(registry_.view<const ObjectBlock>().size());
  int text_count = static_cast<int>(registry_.view<const TextBlock>().size());

  ImGui::Text("Entities: %d objects, %d text", object_count, text_count);
  ImGui::Separator();

  if (ImGui::TreeNodeEx("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
      const bool you = registry_.all_of<IsYou>(e);
      const bool push = registry_.all_of<IsPush>(e);
      const bool stop = registry_.all_of<IsStop>(e);
      const bool win = registry_.all_of<IsWin>(e);
      ImGui::Text("%s @ (%d,%d)%s%s%s%s", PrettyName(object.id), cell.x, cell.y, you ? " [YOU]" : "",
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
    auto to_upper = [](const char* s) {
      std::string out = s ? s : "";
      for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      return out;
    };
    for (const auto& rule : rules_) {
      const std::string subject = to_upper(PrettyName(rule.subject));
      const std::string predicate = to_upper(PrettyName(rule.predicate));
      ImGui::Text("%s . IS . %s", subject.c_str(), predicate.c_str());
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
  if (ImGui::InputText("Name", level_name_, sizeof(level_name_))) {
    level_.name = level_name_;
  }
  ImGui::InputText("Level", level_id_input_, sizeof(level_id_input_),
                   ImGuiInputTextFlags_CharsDecimal);

  auto load_level_from_input = [&]() {
    auto n = ParseLevelNumber(level_id_input_);
    if (!n) return;
    LoadLevelFromPath(std::filesystem::path{kLevelsDir} / LevelFileFromNumber(*n));
  };
  auto save_level_from_input = [&]() {
    auto n = ParseLevelNumber(level_id_input_);
    if (!n) return;
    SaveLevelToPath(std::filesystem::path{kLevelsDir} / LevelFileFromNumber(*n));
  };

  if (ImGui::Button("Load")) {
    load_level_from_input();
  }
  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    save_level_from_input();
  }
  ImGui::SameLine();
  if (ImGui::Button("Prev")) {
    auto n = ParseLevelNumber(level_id_input_);
    if (n) {
      const int prev = std::max(0, *n - 1);
      std::snprintf(level_id_input_, sizeof(level_id_input_), "%d", prev);
      load_level_from_input();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Next")) {
    auto n = ParseLevelNumber(level_id_input_);
    if (n) {
      const int next = std::min(999, *n + 1);
      std::snprintf(level_id_input_, sizeof(level_id_input_), "%d", next);
      load_level_from_input();
    }
  }

  ImGui::End();
}

namespace {

bool ToolIconButton(const char* id, const Texture2D& icon, bool selected,
                    const char* tooltip, float size) {
  if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
  bool clicked = false;
  if (icon.id != 0) {
    clicked = ImGui::ImageButton(id, (ImTextureID)(intptr_t)icon.id, {size, size});
  } else {
    clicked = ImGui::Button(id, {size, size});
  }
  if (selected) ImGui::PopStyleColor();
  if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
  return clicked;
}

}  // namespace

void GameLayer::DrawToolbar() {
  ImGui::TextUnformatted("Tool");
  constexpr float kBtn = 32.0f;

  struct ToolEntry {
    Tool tool;
    const char* id;
    const char* tip;
  };
  const ToolEntry entries[] = {
      {Tool::Brush, "##t_brush", "Brush (pen)"},
      {Tool::Line, "##t_line", "Line"},
      {Tool::RectOutline, "##t_rect", "Rectangle outline"},
      {Tool::RectFilled, "##t_rectf", "Filled rectangle"},
      {Tool::Select, "##t_sel", "Select / cut / paste"},
      {Tool::Bucket, "##t_fill", "Paint bucket (flood)"},
      {Tool::Eraser, "##t_erase", "Eraser"},
  };
  for (std::size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
    if (i > 0) ImGui::SameLine();
    const auto& e = entries[i];
    const Texture2D& icon = tool_icons_[static_cast<int>(e.tool)];
    if (ToolIconButton(e.id, icon, tool_ == e.tool, e.tip, kBtn)) {
      tool_ = e.tool;
      dragging_ = false;
      if (tool_ != Tool::Select) DiscardClipboard();
    }
  }

  // Eraser sub-mode selector: the shape it reuses when Eraser is active.
  // Each sub-mode reuses the Tool icon of its equivalent shape.
  if (tool_ == Tool::Eraser) {
    ImGui::TextUnformatted("Eraser mode");
    struct ModeEntry {
      EraseMode mode;
      Tool icon_tool;  // which tool_icons_ entry to draw
      const char* id;
      const char* tip;
    };
    const ModeEntry modes[] = {
        {EraseMode::Point, Tool::Brush, "##e_pt", "Single cell"},
        {EraseMode::Line, Tool::Line, "##e_ln", "Line"},
        {EraseMode::RectOutline, Tool::RectOutline, "##e_ro", "Rectangle outline"},
        {EraseMode::RectFilled, Tool::RectFilled, "##e_rf", "Filled rectangle"},
        {EraseMode::Bucket, Tool::Bucket, "##e_bk", "Flood fill erase"},
    };
    for (std::size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
      if (i > 0) ImGui::SameLine();
      const auto& m = modes[i];
      const Texture2D& icon = tool_icons_[static_cast<int>(m.icon_tool)];
      if (ToolIconButton(m.id, icon, erase_mode_ == m.mode, m.tip, kBtn)) {
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
  if (ImGui::SliderFloat("SFX volume", &sfx_volume_, 0.0f, 1.0f, "%.2f")) {
    if (step_sound_loaded_) SetSoundVolume(step_sound_, sfx_volume_);
    if (win_sound_loaded_) SetSoundVolume(win_sound_, sfx_volume_);
    if (defeat_sound_loaded_) SetSoundVolume(defeat_sound_, sfx_volume_);
    if (sink_sound_loaded_) SetSoundVolume(sink_sound_, sfx_volume_);
  }
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
  const Vector2 mp = GetMousePosition();
  auto cell_opt = board::ScreenToCell(mp);
  const bool in_board = cell_opt.has_value();
  int col, row;
  if (in_board) {
    col = cell_opt->first;
    row = cell_opt->second;
  } else {
    const Rectangle br = board::BoardRect();
    const float pitch = board::Pitch();
    col = std::clamp(static_cast<int>((mp.x - br.x) / pitch), 0, board::kCols - 1);
    row = std::clamp(static_cast<int>((mp.y - br.y) / pitch), 0, board::kRows - 1);
  }
  // When dragging, preview must follow the cursor even past the board edge.
  // Outside a drag, we only draw the hover ghost when actually over the board.
  const bool show_hover = in_board;

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
  if (show_hover) outline_cell(col, row, YELLOW, 2.0f);

  // Drag-preview overlays for shape-capable tools.
  const bool erasing = (tool_ == Tool::Eraser);
  const Color preview_col = erasing ? Color{235, 90, 90, 255} : YELLOW;

  auto preview_cells = [&](const std::vector<std::pair<int, int>>& cells) {
    for (const auto& [cx, cy] : cells) {
      outline_cell(cx, cy, preview_col, 2.0f);
      if (!erasing) draw_brush_ghost(cx, cy, preview_col, false);
    }
  };

  if (dragging_) {
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
  } else if (tool_ == Tool::Select && !clipboard_.empty() && show_hover) {
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
  } else if (show_hover && (tool_ == Tool::Brush || tool_ == Tool::Bucket ||
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
  // Clamp the cursor to the board so a drag preview keeps tracking the mouse
  // when it slips past the playfield edge. `in_board` stays gated on the real
  // hit-test so single-click actions don't fire from dead zones.
  const Vector2 mp = GetMousePosition();
  auto cell_opt = board::ScreenToCell(mp);
  const bool in_board = cell_opt.has_value();
  int col, row;
  if (in_board) {
    col = cell_opt->first;
    row = cell_opt->second;
  } else {
    const Rectangle br = board::BoardRect();
    const float pitch = board::Pitch();
    col = std::clamp(static_cast<int>((mp.x - br.x) / pitch), 0, board::kCols - 1);
    row = std::clamp(static_cast<int>((mp.y - br.y) / pitch), 0, board::kRows - 1);
  }

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
        commit_shape(false);
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
        CutRegionToClipboard(drag_start_x_, drag_start_y_, col, row);
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
            commit_shape(true);
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
    for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
      if (cell.x == col && cell.y == row && object.id == id) return;
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
  for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
    if (cell.x == x && cell.y == y) out.emplace_back(object.id);
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
  for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
    if (cell.x >= lx && cell.x <= rx && cell.y >= ly && cell.y <= ry) {
      clipboard_.push_back({cell.x - lx, cell.y - ly, object.id});
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
      for (auto [e, cell, object] : registry_.view<const Cell, const ObjectBlock>().each()) {
        if (cell.x == x && cell.y == y && object.id == id) {
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
    RefreshBuiltInLevels();
    if (builtin_level_files_.empty()) {
      ImGui::TextDisabled("(no levels/*.json found)");
    } else {
      selected_level_ = std::clamp(selected_level_, 0, static_cast<int>(builtin_level_files_.size()) - 1);
      const char* lvl_label = builtin_level_files_[selected_level_].c_str();
      if (ImGui::BeginCombo("##builtin", lvl_label)) {
        for (int i = 0; i < static_cast<int>(builtin_level_files_.size()); ++i) {
          const bool selected = (i == selected_level_);
          if (ImGui::Selectable(builtin_level_files_[i].c_str(), selected)) {
            selected_level_ = i;
            LoadLevelFromPath(std::filesystem::path{kLevelsDir} / builtin_level_files_[i]);
          }
          if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      if (ImGui::Button("Prev##b")) {
        const int n = static_cast<int>(builtin_level_files_.size());
        selected_level_ = (selected_level_ - 1 + n) % n;
        LoadLevelFromPath(std::filesystem::path{kLevelsDir} / builtin_level_files_[selected_level_]);
      }
      ImGui::SameLine();
      if (ImGui::Button("Next##b")) {
        const int n = static_cast<int>(builtin_level_files_.size());
        selected_level_ = (selected_level_ + 1) % n;
        LoadLevelFromPath(std::filesystem::path{kLevelsDir} / builtin_level_files_[selected_level_]);
      }
      ImGui::SameLine();
      if (ImGui::Button("Reload Current##b")) {
        LoadLevelFromPath(std::filesystem::path{kLevelsDir} / builtin_level_files_[selected_level_]);
      }
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
      const std::string display_name = ResolveCampaignLevelName(lvl);
      const bool unlocked = IsUnlocked(i);
      const bool completed = progress_.completed.contains(lvl.id);
      const bool current = (lvl.id == current_level_id_);

      ImGui::PushID(static_cast<int>(i));
      const char* badge = completed ? "[*]" : (unlocked ? "[ ]" : "[X]");
      if (current) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s %s", badge, display_name.c_str());
      } else if (unlocked) {
        ImGui::Text("%s %s", badge, display_name.c_str());
      } else {
        ImGui::TextDisabled("%s %s", badge, display_name.c_str());
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

void GameLayer::RefreshBuiltInLevels() {
  builtin_level_files_.clear();
  std::error_code ec;
  if (!std::filesystem::is_directory(kLevelsDir, ec)) return;
  for (const auto& entry : std::filesystem::directory_iterator(kLevelsDir, ec)) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".json") continue;
    builtin_level_files_.push_back(entry.path().filename().string());
  }
  auto key = [](const std::string& file) -> std::pair<long long, std::string> {
    const std::string stem = std::filesystem::path(file).stem().string();
    std::size_t i = 0;
    while (i < stem.size() && std::isdigit(static_cast<unsigned char>(stem[i]))) ++i;
    long long n = (i > 0) ? std::stoll(stem.substr(0, i)) : -1;
    return {n, stem.substr(i)};
  };
  std::sort(builtin_level_files_.begin(), builtin_level_files_.end(),
            [&](const std::string& a, const std::string& b) { return key(a) < key(b); });

  const std::string current_file = current_level_id_.empty() ? "" : (current_level_id_ + ".json");
  if (!current_file.empty()) {
    for (int i = 0; i < static_cast<int>(builtin_level_files_.size()); ++i) {
      if (builtin_level_files_[i] == current_file) {
        selected_level_ = i;
        break;
      }
    }
  }
}
