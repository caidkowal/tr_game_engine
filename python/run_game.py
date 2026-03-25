#!/usr/bin/env python3

import os
import sys
import json
import argparse

#ensure the python folder is on the path when run directly
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to world config .ini file",
    )
    parser.add_argument(
        "--profile",
        required=True,
        help="Path to player profile JSON file",
    )
    return parser.parse_args()


def load_profile(profile_path: str) -> dict:
    """Load profile from disk, creating it with prompted name if missing."""
    if not os.path.exists(profile_path):
        #profile doesn't exist yet, prompt for name and create it
        player_name = input("New profile. Enter your player name: ").strip()
        if not player_name:
            player_name = "Player"
        profile = {
            "player_name": player_name,
            "games_played": 0,
            "max_treasure_collected": 0,
            "most_rooms_world_completed": 0,
            "timestamp_last_played": "",
        }
        save_profile(profile_path, profile)
        return profile

    with open(profile_path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_profile(profile_path: str, profile: dict) -> None:
    """Write profile dict to disk as formatted JSON."""
    with open(profile_path, "w", encoding="utf-8") as f:
        json.dump(profile, f, indent=4)


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    profile_path = os.path.abspath(args.profile)

    #load or create the player profile
    profile = load_profile(profile_path)

    #create the C game engine
    engine = GameEngine(config_path)

    #launch the curses UI - it handles the full game loop
    ui = GameUI(engine, profile, profile_path, save_profile)
    ui.run()

    #always destroy the engine when done to free C memory
    engine.destroy()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())