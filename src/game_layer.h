#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <entt/entt.hpp>

#include "ids.h"
#include "layer.h"
#include "level.h"
#include "rule_engine.h"
#include "sprite_sheet.h"
#include "undo_stack.h"
#include "world.h"

class GameLayer : public Layer {
 public:
  GameLayer();

  void OnAttach() override;
  void OnDetach() override;
  void OnUpdate(float dt) override;
  void OnRender() override;
  void OnImGuiRender() override;

  // Optional palette colors for imported levels. nullopt means the current
  // level has no override and the ImGui theme colors apply.
  // `LevelBackground` is the walkable-interior fill; `LevelEdge` is the
  // outer fill around the playfield.
  std::optional<Color> LevelBackground() const;
  std::optional<Color> LevelEdge() const;

  // Iris (eye-blink) transition shown between levels. Drawn last so it sits
  // on top of ImGui panels too. No-op when no transition is active.
  void DrawTransitionOverlay() const;

 private:
  // Level / registry
  void LoadLevelFromPath(const std::filesystem::path& path);
  void SaveLevelToPath(const std::filesystem::path& path);
  void ResetToInitial();
  void BuildRegistryFromLevel();
  Level ExtractLevelFromRegistry() const;
  void ClearRegistry();
  void SpawnObject(ObjectId id, int x, int y);
  void SpawnText(TextId id, int x, int y);

  // Turn step
  void Step(Direction dir);
  // Re-parses text rules from the board and re-applies tag components. When
  // `apply_transformations` is true, also mutates ObjectBlock.id for entities
  // matching "Noun is Noun" rules (called on Step/level load; not on the
  // per-frame editor preview to avoid cyclic-rule oscillation).
  void RecomputeRules(bool apply_transformations = false);
  bool TryMove(entt::entity who, Direction dir);
  bool CanEnter(int x, int y, int dx, int dy);
  void PushChain(int x, int y, int dx, int dy);
  // Advances every IsMove entity one cell in its facing direction. Blocked
  // movers flip 180° and stay put for the turn (next turn they try the new
  // direction). Returns true if any mover successfully stepped.
  bool StepMovers();
  void RunWinDefeat();

  // Particles (visual only — not part of ECS)
  enum class ParticleStyle : std::uint8_t { Sparkle, Smoke };
  struct Particle {
    Vector2 pos;
    float max_size;
    float life;
    float max_life;
    float rot_deg;
    ParticleStyle style = ParticleStyle::Sparkle;
  };
  void UpdateParticles(float dt);
  void DrawParticles() const;
  void SpawnSmokeAt(int cell_x, int cell_y);

  // Audio
  void LoadTrack(int index);
  void UnloadTrack();

  // World / progression
  void MarkLevelCompleted(const std::string& id);
  bool IsUnlocked(std::size_t level_index) const;

  // Rendering
  void DrawEntities();

  // Panels
  void DrawScenePanel();
  void DrawRulesPanel();
  void DrawEditorPanel();
  void DrawSettingsPanel();
  void DrawWorldPanel();

  // Editor
  enum class Tool : std::uint8_t {
    Brush,
    Line,
    RectOutline,
    RectFilled,
    Select,
    Bucket,
    Eraser,
  };
  // Eraser shares sub-modes with the normal tools; "Point" means one-cell brush.
  enum class EraseMode : std::uint8_t {
    Point,
    Line,
    RectOutline,
    RectFilled,
    Bucket,
  };

  void HandleEditorMouse();
  void DrawEditorOverlay();  // brush ghost, drag preview, clipboard preview
  void DrawToolbar();
  void DrawPalette();

  // One-cell brush placement / erasure.
  void PlaceBrushAt(int col, int row);
  void EraseAt(int col, int row);

  // Multi-cell tool commits.
  void PlaceAtCells(const std::vector<std::pair<int, int>>& cells);
  void EraseAtCells(const std::vector<std::pair<int, int>>& cells);

  // Shape rasterization (inclusive endpoints).
  static std::vector<std::pair<int, int>> RasterLine(int x0, int y0, int x1, int y1);
  static std::vector<std::pair<int, int>> RasterRectOutline(int x0, int y0, int x1, int y1);
  static std::vector<std::pair<int, int>> RasterRectFilled(int x0, int y0, int x1, int y1);

  // 4-neighbor flood: the "content signature" at (sx,sy) defines the target;
  // returns every connected cell with the same signature.
  std::vector<std::pair<int, int>> FloodRegion(int sx, int sy) const;

  // Returns the set of object/text kinds at (x,y) — used as the flood signature.
  std::vector<std::variant<ObjectId, TextId>> CellContents(int x, int y) const;

  // Clipboard (for the select/cut/paste tool).
  void CutRegionToClipboard(int x0, int y0, int x1, int y1);
  void PasteClipboardAt(int col, int row);
  void DiscardClipboard();

  static std::pair<int, int> Delta(Direction dir);
  static const char* PrettyName(ObjectId id);
  static const char* PrettyName(TextId id);

  entt::registry registry_;
  SpriteSheet sprites_;
  Level level_;
  Level initial_level_;
  std::vector<Rule> rules_;
  UndoStack undo_;
  float anim_timer_ = 0.0f;
  int current_frame_ = 1;
  bool won_ = false;
  bool reset_requested_ = false;

  // Key-repeat state for held movement keys.
  std::optional<Direction> held_dir_;
  float hold_time_ = 0.0f;
  bool first_repeat_done_ = false;
  float repeat_delay_ = 0.15f;
  float repeat_interval_ = 0.13f;

  // Editor state
  bool edit_mode_ = false;
  std::variant<ObjectId, TextId> brush_ = ObjectId::Baba;
  Tool tool_ = Tool::Brush;
  EraseMode erase_mode_ = EraseMode::Point;
  int palette_layer_ = 1;  // 0 = backgrounds, 1 = objects, 2 = text

  // Tool icons, indexed by Tool enum (Eraser sub-modes reuse the same art).
  std::array<Texture2D, 7> tool_icons_{};

  // Shape / select drag state. When `dragging_` is true, `drag_start_` holds
  // the anchor cell and the current mouse cell is the live endpoint.
  bool dragging_ = false;
  int drag_start_x_ = 0;
  int drag_start_y_ = 0;

  // Clipboard populated by the Select tool's cut on drag-release. Cells are
  // stored with offsets relative to the top-left of the selection.
  struct ClipTile {
    int dx;
    int dy;
    std::variant<ObjectId, TextId> kind;
  };
  std::vector<ClipTile> clipboard_;

  // Save-as buffer
  char save_name_[64] = "custom.json";
  int selected_level_ = 0;

  // Imported-level browser (assets/imported/)
  std::vector<std::string> imported_stems_;
  int imported_index_ = 0;
  char imported_filter_[64] = "";

  // Visual sparkles for IsWin entities.
  std::vector<Particle> particles_;
  float particle_emit_timer_ = 0.0f;

  // Background music
  Music music_{};
  bool music_loaded_ = false;
  int track_index_ = 0;
  float volume_ = 0.4f;
  bool muted_ = false;

  // Step SFX — short ogg loaded once on attach and played on each successful
  // directional move.
  Sound step_sound_{};
  bool step_sound_loaded_ = false;
  float sfx_volume_ = 0.6f;

  // Win SFX — fanfare played the moment YOU first reaches WIN on a level.
  Sound win_sound_{};
  bool win_sound_loaded_ = false;

  // World / progression
  World world_;
  Progress progress_;
  std::string current_level_id_;
  bool win_handled_ = false;

  // Iris transition between levels. Closing collapses the elliptical opening
  // toward the screen center; at full black we swap to `transition_target_`;
  // Opening reverses it. Input and turn updates pause while State != None.
  enum class TransitionState : std::uint8_t { None, Closing, Opening };
  TransitionState transition_state_ = TransitionState::None;
  float transition_t_ = 0.0f;            // 0..1 progress within the current phase
  std::string transition_target_;        // world level id to load at the black moment
  static constexpr float kTransitionPhaseSeconds = 0.45f;

  // Returns the id of the world level that follows `current_level_id_`, or
  // nullopt if we're on the last level (or unrelated to the world list).
  std::optional<std::string> NextLevelId() const;
};
