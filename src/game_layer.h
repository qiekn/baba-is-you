#pragma once

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

  // Optional palette background for imported levels. nullopt means the
  // current level has no override and the ImGui theme colors apply.
  std::optional<Color> LevelBackground() const;

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
  void RecomputeRules();
  bool TryMove(entt::entity who, Direction dir);
  bool CanEnter(int x, int y, int dx, int dy);
  void PushChain(int x, int y, int dx, int dy);
  void RunWinDefeat();

  // Particles (visual only — not part of ECS)
  struct Particle {
    Vector2 pos;
    float max_size;
    float life;
    float max_life;
    float rot_deg;
  };
  void UpdateParticles(float dt);
  void DrawParticles() const;

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
  void HandleEditorMouse();
  void PlaceBrushAt(int col, int row);
  void EraseAt(int col, int row);

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

  // World / progression
  World world_;
  Progress progress_;
  std::string current_level_id_;
  bool win_handled_ = false;
};
