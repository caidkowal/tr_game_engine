"""
GameUI - Curses-based user interface for Treasure Runner.

Handles all rendering, input, splash screens, and the main game loop.
All game logic is delegated to GameEngine. This class only reads state
and renders it - it never calls C functions directly.
"""

import curses
import datetime
import random

from treasure_runner.bindings.bindings import Direction, RoomTileType
from treasure_runner.models.exceptions import ImpassableError, NoPortalError, GameError


# minimum terminal size required to render the UI
MIN_HEIGHT = 24
MIN_COLS = 60

# Sentinel character emitted by room_render for locked portals.
# The UI displays this as a red X so the player always sees X,
# but the color tells them whether it is locked or open.
LOCKED_PORTAL_SENTINEL = '!'

# Legend entries: (raw_char, description)
# raw_char is fed through _char_display so '!' renders as red X automatically.
_LEGEND = [
    (None,                   "Game Elements:"),
    ('@',                    "- player"),
    ('#',                    "- wall"),
    ('$',                    "- gold"),
    ('X',                    "- portal (open)"),
    (LOCKED_PORTAL_SENTINEL, "- portal (locked)"),
    ('O',                    "- pushable block"),
    ('=',                    "- switch (unpressed)"),
    ('+',                    "- switch (pressed)"),
]


class GameUI:

    def __init__(self, engine, profile: dict, profile_path: str, save_fn):
        self._engine = engine
        self._profile = profile
        self._profile_path = profile_path
        self._save_fn = save_fn
        self._message = "Use WASD or arrows to move. > to use portal. r to reset. q to quit."

        self._visited_rooms = set()
        self._victory = False
        self._start_time = datetime.datetime.utcnow()

        self._room_names = {}
        self._adjectives = ["Slimy", "Stinky", "Cold", "Hot", "Dark", "Ancient", "Dusty", "Creepy"]
        self._nouns = ["Depths", "Cave", "Basement", "Cellar", "Chamber", "Tunnel", "Vault", "Pit"]

    # ------------------------------------------------------------------
    # Public entry point
    # ------------------------------------------------------------------

    def run(self) -> None:
        """Launch the curses wrapper - blocks until the game ends."""
        curses.wrapper(self._main)

    # ------------------------------------------------------------------
    # Main curses loop
    # ------------------------------------------------------------------

    def _main(self, stdscr) -> None:
        """Called by curses.wrapper with the root window."""
        self._check_terminal_size(stdscr)
        curses.curs_set(0)
        curses.start_color()
        curses.use_default_colors()

        # Color pairs:
        # 1=YELLOW treasure, 2=CYAN player, 3=WHITE wall, 4=GREEN open portal/visited,
        # 5=MAGENTA pushable, 6=RED locked portal, 7=MAGENTA(dim) switch off, 8=GREEN switch on
        curses.init_pair(1, curses.COLOR_YELLOW,  -1)
        curses.init_pair(2, curses.COLOR_CYAN,    -1)
        curses.init_pair(3, curses.COLOR_WHITE,   -1)
        curses.init_pair(4, curses.COLOR_GREEN,   -1)
        curses.init_pair(5, curses.COLOR_MAGENTA, -1)
        curses.init_pair(6, curses.COLOR_RED,     -1)
        curses.init_pair(7, curses.COLOR_MAGENTA, -1)
        curses.init_pair(8, curses.COLOR_GREEN,   -1)

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
        key_dir_map = self._build_key_direction_map()

        while True:
            self._check_terminal_size(stdscr)
            self._draw(stdscr)
            key = stdscr.getch()
            if key == ord('q'):
                break
            self._process_game_key(key, key_dir_map)
            if self._victory:
                break

    def _build_key_direction_map(self) -> dict:
        """Return a mapping of key codes to Direction values."""
        return {
            curses.KEY_UP:    Direction.NORTH,
            ord('w'):         Direction.NORTH,
            ord('W'):         Direction.NORTH,
            curses.KEY_DOWN:  Direction.SOUTH,
            ord('s'):         Direction.SOUTH,
            ord('S'):         Direction.SOUTH,
            curses.KEY_LEFT:  Direction.WEST,
            ord('a'):         Direction.WEST,
            ord('A'):         Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
            ord('d'):         Direction.EAST,
            ord('D'):         Direction.EAST,
        }

    def _process_game_key(self, key: int, key_dir_map: dict) -> None:
        """Dispatch a key press to the appropriate handler."""
        if key == ord('r'):
            self._handle_reset()
        elif key == ord('>'):
            self._handle_use_portal()
        elif key in key_dir_map:
            self._handle_movement(key_dir_map[key])

    # ------------------------------------------------------------------
    # Action handlers
    # ------------------------------------------------------------------

    def _handle_reset(self) -> None:
        """Reset the world and player to initial state."""
        self._engine.reset()
        self._visited_rooms = set()
        self._visited_rooms.add(self._engine.player.get_room())
        self._message = "Game reset to initial state."

    def _handle_movement(self, direction: Direction) -> None:
        """Intercept movement input.

        If the tile ahead is a portal, step ONTO it without traversing it
        and prompt the player to press > to enter.

        For all other tile types, delegate to the normal C movement logic
        which correctly handles walls, treasures, and pushables.
        """
        try:
            tile_ahead = self._engine.peek_tile(direction)
        except GameError:
            # if peek fails for any reason, fall back to normal movement
            self._do_normal_move(direction)
            return

        if tile_ahead == RoomTileType.PORTAL:
            self._step_onto_portal(direction)
        else:
            self._do_normal_move(direction)

    def _step_onto_portal(self, direction: Direction) -> None:
        """Move the player one tile onto a portal without traversing it."""
        dx, dy = self._direction_delta(direction)
        x_pos, y_pos = self._engine.player.get_position()
        try:
            self._engine.set_player_position(x_pos + dx, y_pos + dy)
            self._visited_rooms.add(self._engine.player.get_room())
            self._message = "You're standing on a portal. Press > to enter."
        except GameError as game_err:
            self._message = f"Error: {game_err}"

    def _do_normal_move(self, direction: Direction) -> None:
        """Call game_engine_move_player and update the message bar."""
        collected_before = self._engine.player.get_collected_count()
        try:
            self._engine.move_player(direction)
            self._visited_rooms.add(self._engine.player.get_room())
            self._update_treasure_message(collected_before)
        except ImpassableError:
            self._message = "You can't go that way."
        except GameError as game_err:
            self._message = f"Error: {game_err}"

    def _update_treasure_message(self, collected_before: int) -> None:
        """Update the message bar after a move that may have collected treasure."""
        collected_after = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        if collected_after > collected_before:
            if collected_after == total:
                self._victory = True
                self._message = (
                    f"You picked up the FINAL treasure! ({collected_after}/{total}) - YOU WIN!"
                )
            else:
                self._message = f"You picked up a treasure! ({collected_after}/{total})"
        else:
            self._message = "Collect the treasure fast, the clock is ticking!"

    def _handle_use_portal(self) -> None:
        """Attempt to traverse the portal the player is currently standing on."""
        try:
            self._engine.use_portal()
            new_room = self._engine.player.get_room()
            self._visited_rooms.add(new_room)
            self._message = f"You entered room {new_room}."
        except NoPortalError:
            self._message = "You're not standing on a portal."
        except ImpassableError:
            self._message = "This portal is locked! Push the block (O) onto the switch (=)."
        except GameError as game_err:
            self._message = f"Error: {game_err}"

    @staticmethod
    def _direction_delta(direction: Direction) -> tuple:
        """Return (dx, dy) for a given direction."""
        return {
            Direction.NORTH: (0, -1),
            Direction.SOUTH: (0,  1),
            Direction.EAST:  (1,  0),
            Direction.WEST:  (-1, 0),
        }.get(direction, (0, 0))

    def _get_room_name(self, room_id: int) -> str:
        """Return a consistent randomised name for a room."""
        if room_id not in self._room_names:
            adj = random.choice(self._adjectives)
            noun = random.choice(self._nouns)
            self._room_names[room_id] = f"{adj} {noun}"
        return self._room_names[room_id]

    # ------------------------------------------------------------------
    # Character display helper
    # ------------------------------------------------------------------

    def _char_display(self, tile_char: str) -> tuple:
        """Return (display_char, color_attr) for a tile character.

        The locked portal sentinel '!' is displayed as 'X' in red so the
        player always sees X but color distinguishes locked from open.
        """
        char_map = {
            '@': ('@', curses.color_pair(2)),
            '$': ('$', curses.color_pair(1)),
            '#': ('#', curses.color_pair(3) | curses.A_DIM),
            'X': ('X', curses.color_pair(4)),
            LOCKED_PORTAL_SENTINEL: ('X', curses.color_pair(6)),
            'O': ('O', curses.color_pair(5)),
            '=': ('=', curses.color_pair(7) | curses.A_DIM),
            '+': ('+', curses.color_pair(8)),
        }
        return char_map.get(tile_char, (tile_char, 0))

    # ------------------------------------------------------------------
    # Splash / end screens
    # ------------------------------------------------------------------

    def _splash_screen(self, stdscr) -> None:
        """Show startup screen with player profile. Wait for any key."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        lines = [
            "=" * min(40, width - 1),
            "  TREASURE RUNNER",
            "=" * min(40, width - 1),
            "",
            f"  Welcome, {self._profile.get('player_name', 'Player')}!",
            "",
            f"  Games played:            {self._profile.get('games_played', 0)}",
            f"  Max treasures collected: {self._profile.get('max_treasure_collected', 0)}",
            f"  Most rooms completed:    {self._profile.get('most_rooms_world_completed', 0)}",
            f"  Last played:             {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "  Press any key to start...",
        ]
        self._render_screen_lines(stdscr, lines, height, width)
        stdscr.refresh()
        stdscr.getch()

    def _quit_screen(self, stdscr) -> None:
        """Show end screen with updated profile stats. Wait for any key."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        collected = self._engine.player.get_collected_count()
        best = max(collected, self._profile.get('max_treasure_collected', 0))
        rooms_best = max(len(self._visited_rooms),
                         self._profile.get('most_rooms_world_completed', 0))
        lines = [
            "=" * min(40, width - 1),
            "  GAME OVER",
            "=" * min(40, width - 1),
            "",
            f"  Player:                       {self._profile.get('player_name', 'Player')}",
            f"  Treasures collected this run: {collected}",
            f"  Games played:                 {self._profile.get('games_played', 0) + 1}",
            f"  All-time best:                {best}",
            f"  Most rooms completed:         {rooms_best}",
            "",
            "  Press any key to exit...",
        ]
        self._render_screen_lines(stdscr, lines, height, width)
        stdscr.refresh()
        stdscr.getch()

    def _victory_screen(self, stdscr) -> None:
        """Show victory screen when all treasure is collected."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        time_str = self._elapsed_time_str()
        best = max(collected, self._profile.get('max_treasure_collected', 0))
        rooms_best = max(len(self._visited_rooms),
                         self._profile.get('most_rooms_world_completed', 0))
        lines = [
            "=" * min(40, width - 1),
            "  YOU WIN!",
            "=" * min(40, width - 1),
            "",
            f"  Player:                  {self._profile.get('player_name', 'Player')}",
            f"  Treasure:                {collected}/{total}",
            f"  Games played:            {self._profile.get('games_played', 0) + 1}",
            f"  Max treasures collected: {best}",
            f"  Most rooms completed:    {rooms_best}",
            "",
            f"  Time elapsed: {time_str}",
            "",
            "  Press any key to exit...",
        ]
        self._render_screen_lines(stdscr, lines, height, width)
        stdscr.refresh()
        stdscr.getch()

    def _elapsed_time_str(self) -> str:
        """Return a human-readable string for elapsed game time."""
        elapsed_secs = int((datetime.datetime.utcnow() - self._start_time).total_seconds())
        return f"{elapsed_secs // 60}m {elapsed_secs % 60}s"

    def _render_screen_lines(self, stdscr, lines: list, height: int, width: int) -> None:
        """Render a list of text lines to the screen from row 0."""
        for row, line in enumerate(lines):
            if row >= height - 1:
                break
            self._safe_addstr(stdscr, row, 0, line[:width - 1])

    # ------------------------------------------------------------------
    # Drawing
    # ------------------------------------------------------------------

    def _draw(self, stdscr) -> None:
        """Render the full UI: message bar, room, controls, status bar."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        self._draw_message_bar(stdscr, 0, width)
        next_row = self._draw_room_section(stdscr, height, width)
        self._draw_minimap(stdscr, 2, 65)
        self._draw_controls(stdscr, next_row, width)
        self._draw_status_bar(stdscr, height - 2, width)
        self._draw_footer(stdscr, height - 1, width)
        stdscr.refresh()

    def _draw_room_section(self, stdscr, height: int, width: int) -> int:
        """Draw the room name (row 1) and the room grid (row 2+).

        Returns the next available row after the room.
        """
        room_id = self._engine.player.get_room()
        room_name = self._get_room_name(room_id)
        self._safe_addstr(stdscr, 1, 0, f"Room {room_id}: {room_name}"[:width - 1])

        room_lines = self._engine.render_current_room().splitlines()
        legend_col = 40
        row = 2

        for idx, line in enumerate(room_lines):
            if row >= height - 3:
                break
            self._draw_room_line(stdscr, row, line, width)
            self._draw_legend_entry(stdscr, row, legend_col, idx)
            row += 1

        return row

    def _draw_room_line(self, stdscr, row: int, line: str, width: int) -> None:
        """Draw one row of the room buffer with appropriate colors."""
        for col, tile_char in enumerate(line[:width - 1]):
            display, color_attr = self._char_display(tile_char)
            if color_attr:
                stdscr.addstr(row, col, display, color_attr)
            else:
                stdscr.addstr(row, col, display)

    def _draw_legend_entry(self, stdscr, row: int, legend_col: int, entry_idx: int) -> None:
        """Draw one legend entry alongside the room."""
        if entry_idx >= len(_LEGEND):
            return
        symbol, description = _LEGEND[entry_idx]
        if symbol is None:
            self._safe_addstr(stdscr, row, legend_col, description)
        else:
            display, color_attr = self._char_display(symbol)
            stdscr.addstr(row, legend_col, display, color_attr)
            self._safe_addstr(stdscr, row, legend_col + 2, description)

    def _draw_message_bar(self, stdscr, row: int, width: int) -> None:
        """Render the message bar at the given row."""
        msg = self._message or ""
        self._safe_addstr(stdscr, row, 0, msg.ljust(width - 1)[:width - 1])

    def _draw_status_bar(self, stdscr, row: int, width: int) -> None:
        """Render player stats at the given row."""
        player = self._engine.player
        collected = player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        room_id = player.get_room()
        name = self._profile.get("player_name", "Player")
        x_pos, y_pos = player.get_position()
        status = (
            f"Player: {name}  |  Room: {room_id}  |  "
            f"Pos: ({x_pos},{y_pos})  |  Treasures: {collected}/{total}"
        )
        self._safe_addstr(stdscr, row, 0, status[:width - 1])

    def _draw_controls(self, stdscr, row: int, width: int) -> None:
        """Render the controls legend at the given row."""
        controls = (
            "Controls: WASD/Arrows=move  >=portal  r=reset  q=quit"
            "  Push O onto = to unlock red X"
        )
        self._safe_addstr(stdscr, row, 0, controls[:width - 1])

    def _draw_footer(self, stdscr, row: int, width: int) -> None:
        """Render the title/email footer at the given row."""
        title = "Treasure Runner  |  ckowal02@uoguelph.ca"
        self._safe_addstr(stdscr, row, 0, title[:width - 1])

    def _draw_minimap(self, stdscr, start_row: int, start_col: int) -> None:
        """Draw the room adjacency minimap with correct room-ID colouring.

        Room IDs are NOT guaranteed to be 0-based indices, so we fetch the
        real ID list from the engine and use that to map matrix index to room ID.
        """
        matrix = self._engine.get_adjacency_matrix()
        room_ids = self._engine.get_room_ids()
        room_count = len(matrix)
        current = self._engine.player.get_room()

        self._safe_addstr(stdscr, start_row - 1, start_col, "Minimap:")

        for idx in range(room_count):
            room_id = room_ids[idx]
            if room_id == current:
                attr = curses.color_pair(2)
            elif room_id in self._visited_rooms:
                attr = curses.color_pair(4)
            else:
                attr = 0
            conn_str = ", ".join(
                str(room_ids[j]) for j in range(room_count) if matrix[idx][j] == 1
            )
            self._safe_addstr(stdscr, start_row + idx, start_col,
                               f"[{room_id}]-> {conn_str}", attr)

    # ------------------------------------------------------------------
    # Profile update
    # ------------------------------------------------------------------

    def _update_and_save_profile(self) -> None:
        """Update profile stats after a game run and persist to disk."""
        collected = self._engine.player.get_collected_count()
        rooms_visited = len(self._visited_rooms)

        self._profile["games_played"] = self._profile.get("games_played", 0) + 1
        self._profile["max_treasure_collected"] = max(
            collected, self._profile.get("max_treasure_collected", 0)
        )
        self._profile["most_rooms_world_completed"] = max(
            rooms_visited, self._profile.get("most_rooms_world_completed", 0)
        )
        self._profile["timestamp_last_played"] = (
            datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
        )
        self._save_fn(self._profile_path, self._profile)

    # ------------------------------------------------------------------
    # Utilities
    # ------------------------------------------------------------------

    def _safe_addstr(self, stdscr, row: int, col: int, text: str, attr: int = 0) -> None:
        try:
            if attr:
                stdscr.addstr(row, col, text, attr)
            else:
                stdscr.addstr(row, col, text)
        except curses.error:
            pass

    def _safe_addch(self, stdscr, row: int, col: int, character: str, attr: int = 0) -> None:
        try:
            stdscr.addch(row, col, character, attr)
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