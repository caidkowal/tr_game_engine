"""
GameUI - Curses-based user interface for Treasure Runner.

Handles all rendering, input, splash screens, and the main game loop.
All game logic is delegated to GameEngine. This class only reads state
and renders it - it never calls C functions directly.
"""

import curses
import datetime
import random

from treasure_runner.bindings.bindings import Direction
from treasure_runner.models.exceptions import ImpassableError, GameError


#minimum terminal size required to render the UI
MIN_HEIGHT = 24
MIN_COLS = 60


class GameUI:

    def __init__(self, engine, profile: dict, profile_path: str, save_fn):
        """
        engine      - GameEngine instance (MVC controller)
        profile     - dict loaded from the player profile JSON
        profile_path - path to the profile file so we can save it on exit
        save_fn     - callable(profile_path, profile) to persist the profile
        """
        self._engine = engine
        self._profile = profile
        self._profile_path = profile_path
        self._save_fn = save_fn
        self._message = "Use WASD or arrows to move. > for portal. r to reset. q to quit."

        self._visited_rooms = set()
        self._victory = False
        self._start_time = datetime.datetime.utcnow()

        self._room_names = {}
        self._adjectives = ["Slimy", "Stinky", "Cold", "Hot", "Dark", "Ancient", "Dusty", "Creepy"]
        self._nouns = ["Depths", "Cave", "Basement", "Cellar", "Chamber", "Tunnel", "Vault", "Pit"]

    #public entry point

    def run(self) -> None:
        """Launch the curses wrapper - blocks until the game ends."""
        curses.wrapper(self._main)

    #main curses loop

    def _main(self, stdscr) -> None:
        """Called by curses.wrapper with the root window."""
        self._check_terminal_size(stdscr)
        curses.curs_set(0)

        curses.start_color()
        curses.use_default_colors()

        # color pairs
        curses.init_pair(1, curses.COLOR_YELLOW, -1)  # treasure $
        curses.init_pair(2, curses.COLOR_CYAN, -1)     # player @
        curses.init_pair(3, curses.COLOR_WHITE, -1)   # walls #
        curses.init_pair(4, curses.COLOR_GREEN, -1)   # exit x
        curses.init_pair(5, curses.COLOR_MAGENTA, -1)   # pushables O

        self._splash_screen(stdscr)
        self._game_loop(stdscr)

        self._update_and_save_profile()

        if self._victory:
            self._victory_screen(stdscr)
        else:
            self._quit_screen(stdscr)

    def _game_loop(self, stdscr) -> None:
        """Main input/render loop."""
        self._visited_rooms.add(self._engine.player.get_room())

        while True:
            self._check_terminal_size(stdscr)
            self._draw(stdscr)

            key = stdscr.getch()

            if key == ord('q'):
                break

            elif key == ord('r'):
                self._engine.reset()
                self._visited_rooms = set()
                self._visited_rooms.add(self._engine.player.get_room())
                self._message = "Game reset to initial state."

            elif key == ord('>'):
                self._try_portal(stdscr)

            elif key in (curses.KEY_UP, ord('w'), ord('W')):
                self._try_move(Direction.NORTH)

            elif key in (curses.KEY_DOWN, ord('s'), ord('S')):
                self._try_move(Direction.SOUTH)

            elif key in (curses.KEY_LEFT, ord('a'), ord('A')):
                self._try_move(Direction.WEST)

            elif key in (curses.KEY_RIGHT, ord('d'), ord('D')):
                self._try_move(Direction.EAST)

            # --- ADDED: victory exit ---
            if self._victory:
                break


    #movement helpers

    def _try_move(self, direction: Direction) -> None:
        """Attempt a move and update the message bar with the result."""
            
        collected_before = self._engine.player.get_collected_count()

        try:
            self._engine.move_player(direction)
            self._visited_rooms.add(self._engine.player.get_room())
            collected_after = self._engine.player.get_collected_count()

            # --- ADDED: total treasure + progress tracking ---
            total = self._engine.get_total_treasure_count()

            if collected_after > collected_before:
                if collected_after == total:
                    self._victory = True
                    self._message = f"You picked up the final treasure! ({collected_after}/{total})"
                else:
                    self._message = f"You picked up a treasure! ({collected_after}/{total})"
            else:
                self._message = f"Collect the treasure fast, the clock is ticking!"

        except ImpassableError:
            self._message = "You can't go that way."
        except GameError as e:
            self._message = f"Error: {e}"

    def _try_portal(self, stdscr) -> None:
        """Attempt to move through a portal (same as moving into portal tile)."""
        # portals are entered by walking into them directionally
        # > key attempts all four directions and takes the first that changes room
        current_room = self._engine.player.get_room()
        for direction in [Direction.NORTH, Direction.SOUTH, Direction.EAST, Direction.WEST]:
            try:
                self._engine.move_player(direction)
                if self._engine.player.get_room() != current_room:
                    self._message = f"Entered room {self._engine.player.get_room()}."
                    return
                # moved but didn't change room, undo by not counting it
            except (ImpassableError, GameError):
                continue
        self._message = "No portal here."

    def _get_room_name(self, room_id: int) -> str:
        """Return a consistent randomized name for a room."""
        if room_id not in self._room_names:
            adj = random.choice(self._adjectives)
            noun = random.choice(self._nouns)
            self._room_names[room_id] = f"{adj} {noun}"
        return self._room_names[room_id]

    #victory screen
    def _victory_screen(self, stdscr) -> None:
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        row = 0

        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasure_count()

        elapsed = datetime.datetime.utcnow() - self._start_time
        total_seconds = int(elapsed.total_seconds())
        minutes = total_seconds // 60
        seconds = total_seconds % 60
        time_str = f"{minutes}m {seconds}s"

        lines = [
            "=" * min(40, width - 1),
            "  YOU WIN!",
            "=" * min(40, width - 1),
            "",
            f"  Player: {self._profile.get('player_name', 'Player')}",
            f"  Treasure progress: {collected}/{total}",
            f"  Games played: {self._profile.get('games_played', 0) + 1}",
            f"  Max treasures collected: {max(collected, self._profile.get('max_treasure_collected', 0))}",
            f"  Most rooms completed: {max(len(self._visited_rooms), self._profile.get('most_rooms_world_completed', 0))}",
            "",
            f"  Time elapsed: {time_str}",
            "",
            "  Press any key to exit...",
        ]

        for line in lines:
            if row >= height - 1:
                break
            self._safe_addstr(stdscr, row, 0, line[:width - 1])
            row += 1

        stdscr.refresh()
        stdscr.getch()

    # ------------------------------------------------------------------ #
    # Drawing                                                              #
    # ------------------------------------------------------------------ #

    def _draw(self, stdscr) -> None:
        """Render the full UI: message bar, room, controls, status bar."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()

        row = 0

        # --- message bar (row 0) ---
        self._draw_message_bar(stdscr, row, width)
        row += 1

        # --- room name line (row 1) ---
        room_id = self._engine.player.get_room()
        room_name = self._get_room_name(room_id)
        room_label = f"Room {room_id}: {room_name}"
        self._safe_addstr(stdscr, row, 0, room_label[:width - 1])
        row += 1

        # --- room render ---
        room_str = self._engine.render_current_room()
        room_lines = room_str.splitlines()

        legend_col = 30

        for i, line in enumerate(room_lines):
            if row >= height - 3:
                break

            # --- draw room ---
            for col, ch in enumerate(line[:width - 1]):
                if ch == '@':
                    stdscr.addstr(row, col, ch, curses.color_pair(2))
                elif ch == '$':
                    stdscr.addstr(row, col, ch, curses.color_pair(1))
                elif ch == '#':
                    stdscr.addstr(row, col, ch, curses.color_pair(3) | curses.A_DIM)
                elif ch == 'X':
                    stdscr.addstr(row, col, ch, curses.color_pair(4))
                elif ch == 'O':
                    stdscr.addstr(row, col, ch, curses.color_pair(5))
                else:
                    stdscr.addstr(row, col, ch)

            # --- draw legend (colored) ---
            if i == 0:
                self._safe_addstr(stdscr, row, legend_col, "Game Elements:")
            elif i == 1:
                stdscr.addstr(row, legend_col, "@", curses.color_pair(2))
                self._safe_addstr(stdscr, row, legend_col + 2, "- player")
            elif i == 2:
                stdscr.addstr(row, legend_col, "#", curses.color_pair(3))
                self._safe_addstr(stdscr, row, legend_col + 2, "- wall")
            elif i == 3:
                stdscr.addstr(row, legend_col, "$", curses.color_pair(1))
                self._safe_addstr(stdscr, row, legend_col + 2, "- gold")
            elif i == 4:
                stdscr.addstr(row, legend_col, "X", curses.color_pair(4))
                self._safe_addstr(stdscr, row, legend_col + 2, "- exit")
            elif i == 5:
                stdscr.addstr(row, legend_col, "O", curses.color_pair(5))
                self._safe_addstr(stdscr, row, legend_col + 2, "- pushable")

            row += 1

        # --- controls legend ---
        controls = "Controls: WASD/Arrows=move  >=portal  r=reset  q=quit"
        if row < height - 2:
            self._safe_addstr(stdscr, row, 0, controls[:width - 1])
            row += 1

        # --- player status bar ---
        status_row = height - 2
        self._draw_status_bar(stdscr, status_row, width)

        # --- title ---
        title_row = height - 1
        title = "Treasure Runner  |  ckowal02@uoguelph.ca"
        self._safe_addstr(stdscr, title_row, 0, title[:width - 1])

        stdscr.refresh()

    def _draw_message_bar(self, stdscr, row: int, width: int) -> None:
        """Render the message bar at the given row."""
        msg = self._message if self._message else ""
        padded = msg.ljust(width - 1)
        self._safe_addstr(stdscr, row, 0, padded[:width - 1])

    def _draw_status_bar(self, stdscr, row: int, width: int) -> None:
        """Render player stats at the given row."""
        player = self._engine.player
        collected = player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        room_id = player.get_room()
        name = self._profile.get("player_name", "Player")
        x, y = player.get_position()
        status = (
            f"Player: {name}  |  "
            f"Room: {room_id}  |  "
            f"Pos: ({x},{y})  |  "
            f"Treasures: {collected}/{total}"
        )
        self._safe_addstr(stdscr, row, 0, status[:width - 1])

    #spash screens                                                       

    def _splash_screen(self, stdscr) -> None:
        """Show startup screen with player profile. Wait for any key."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        row = 0

        lines = [
            "=" * min(40, width - 1),
            "  TREASURE RUNNER",
            "=" * min(40, width - 1),
            "",
            f"  Welcome, {self._profile.get('player_name', 'Player')}!",
            "",
            f"  Games played:           {self._profile.get('games_played', 0)}",
            f"  Max treasures collected: {self._profile.get('max_treasure_collected', 0)}",
            f"  Most rooms completed:   {self._profile.get('most_rooms_world_completed', 0)}",
            f"  Last played:            {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "  Press any key to start...",
        ]

        for line in lines:
            if row >= height - 1:
                break
            self._safe_addstr(stdscr, row, 0, line[:width - 1])
            row += 1

        stdscr.refresh()
        stdscr.getch()

    def _quit_screen(self, stdscr) -> None:
        """Show end screen with updated profile stats. Wait for any key."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        row = 0

        collected = self._engine.player.get_collected_count()

        lines = [
            "=" * min(40, width - 1),
            "  GAME OVER",
            "=" * min(40, width - 1),
            "",
            f"  Player:            {self._profile.get('player_name', 'Player')}",
            f"  Treasures collected this run: {collected}",
            f"  Games played:      {self._profile.get('games_played', 0) + 1}",
            f"  All-time best:     {max(collected, self._profile.get('max_treasure_collected', 0))}",
            f"  Most rooms completed: {max(len(self._visited_rooms), self._profile.get('most_rooms_world_completed', 0))}",
            "",
            "  Press any key to exit...",
        ]

        for line in lines:
            if row >= height - 1:
                break
            self._safe_addstr(stdscr, row, 0, line[:width - 1])
            row += 1

        stdscr.refresh()
        stdscr.getch()

    # ------------------------------------------------------------------ #
    # Profile update                                                       #
    # ------------------------------------------------------------------ #

    def _update_and_save_profile(self) -> None:
        """Update profile stats after a game run and persist to disk."""
        collected = self._engine.player.get_collected_count()

        rooms_visited = len(self._visited_rooms)

        self._profile["games_played"] = self._profile.get("games_played", 0) + 1

        self._profile["max_treasure_collected"] = max(
            collected,
            self._profile.get("max_treasure_collected", 0)
        )

        self._profile["most_rooms_world_completed"] = max(
            rooms_visited,
            self._profile.get("most_rooms_world_completed", 0)
        )

        self._profile["timestamp_last_played"] = datetime.datetime.utcnow().strftime(
            "%Y-%m-%dT%H:%M:%SZ"
        )

        self._save_fn(self._profile_path, self._profile)

        

    # ------------------------------------------------------------------ #
    # Utilities                                                            #
    # ------------------------------------------------------------------ #

    def _safe_addstr(self, stdscr, row: int, col: int, text: str) -> None:
        """Write text without crashing if it hits the window edge."""
        height, width = stdscr.getmaxyx()
        if row < 0 or row >= height:
            return
        try:
            stdscr.addstr(row, col, text[:width - col - 1])
        except curses.error:
            pass

    def _check_terminal_size(self, stdscr) -> None:
        """Raise an error and exit cleanly if the terminal is too small."""
        height, width = stdscr.getmaxyx()
        if height < MIN_HEIGHT or width < MIN_COLS:
            stdscr.erase()
            msg = f"Terminal too small ({width}x{height}). Need {MIN_COLS}x{MIN_HEIGHT}."
            try:
                stdscr.addstr(0, 0, msg)
            except curses.error:
                pass
            stdscr.refresh()
            stdscr.getch()
            raise RuntimeError(msg)