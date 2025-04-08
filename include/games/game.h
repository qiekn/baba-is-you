// Copyright (c) 2020-2023 Chris Ohk

#pragma once

#include "enums/rule_enums.h"
#include "games/map.h"
#include "rules/rule_manager.h"

class Game {
public:
  /**
   * @brief Constructs game with given map file.
   *
   * @param filename The map file name.
   */
  explicit Game(std::string_view filename);

  /**
   * @brief Reset map and rule data.
   */
  void Reset();

  /** @brief Gets a map object */
  Map &GetMap();

  const Map &GetMap() const;

  RuleManager &GetRuleManager();

  GameState GetGameState() const;

  ObjectType GetPlayerIcon() const;

  /** @brief Moves the icon that represents player */
  void MovePlayer(Direction dir);

private:
  //! Parses a list of rules.
  void ParseRules();

  //! Parses a rule that satisfies the condition.
  //! \param x The x position.
  //! \param y The y position.
  //! \param direction The direction to check the rule.
  void ParseRule(std::size_t x, std::size_t y, RuleDirection direction);

  //! Checks an object can move.
  //! \param x The x position.
  //! \param y The y position.
  //! \param dir The direction to move.
  //! \return The flag indicates that an object can move.
  bool CanMove(std::size_t x, std::size_t y, Direction dir);

  //! Processes the move of the player.
  //! \param x The x position.
  //! \param y The y position.
  //! \param dir The direction to move.
  //! \param type The object type to move.
  void ProcessMove(std::size_t x, std::size_t y, Direction dir,
                   ObjectType type);

  //! Checks the play state of the game.
  void CheckPlayState();

  Map map_;
  RuleManager rule_manager_;

  GameState game_state_ = GameState::INVALID;
  ObjectType plaer_icon_ = ObjectType::ICON;
};
