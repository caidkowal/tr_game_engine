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
from treasure_runner.models.exceptions import (
    ImpassableError, NoPortalError, GameError
)


# minimum terminal size required to render the UI
MIN_HEIGHT = 24
MIN_COLS = 60

# Sentinel character emitted by room_render for locked portals.
# The UI displays this as a red X so the player always sees X,
# but the color tells them whether it is locked or open.
LOCKED_PORTAL_SENTINEL = '!'


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

        # Color pairs
        # 1 = YELLOW  : treasure $
        # 2 = CYAN    : player @
        # 3 = WHITE   : wall #
        # 4 = GREEN   : open portal X / visited room in minimap
        # 5 = MAGENTA : pushable block O
        # 6 = RED     : locked portal (rendered as X)
        # 7 = YELLOW  : switch unpressed =
        # 8 = GREEN   : switch pressed + (pushable consumed onto switch)
        curses.init_pair(1, curses.COLOR_YELLOW,  -1)
        curses.init_pair(2, curses.COLOR_CYAN,    -1)
        curses.init_pair(3, curses.COLOR_WHITE,   -1)
        curses.init_pair(4, curses.COLOR_GREEN,   -1)
        curses.init_pair(5, curses.COLOR_MAGENTA, -1)
        curses.init_pair(6, curses.COLOR_RED,     -1)
        curses.init_pair(7, curses.COLOR_MAGENTA,  -1)
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
                self._handle_use_portal()

            elif key in (curses.KEY_UP, ord('w'), ord('W')):
                self._handle_movement(Direction.NORTH)

            elif key in (curses.KEY_DOWN, ord('s'), ord('S')):
                self._handle_movement(Direction.SOUTH)

            elif key in (curses.KEY_LEFT, ord('a'), ord('A')):
                self._handle_movement(Direction.WEST)

            elif key in (curses.KEY_RIGHT, ord('d'), ord('D')):
                self._handle_movement(Direction.EAST)

            if self._victory:
                break

    # ------------------------------------------------------------------
    # Movement helpers
    # ------------------------------------------------------------------

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
            # step the player ONTO the portal tile without entering it
            dx, dy = self._direction_delta(direction)
            x, y = self._engine.player.get_position()
            new_x = x + dx
            new_y = y + dy
            try:
                self._engine.set_player_position(new_x, new_y)
                self._visited_rooms.add(self._engine.player.get_room())
                self._message = "You're standing on a portal. Press > to enter."
            except GameError as e:
                self._message = f"Error: {e}"
        else:
            # all other tile types (floor, wall, treasure, pushable) use normal logic
            self._do_normal_move(direction)

    def _do_normal_move(self, direction: Direction) -> None:
        """Call game_engine_move_player and update the message bar."""
        collected_before = self._engine.player.get_collected_count()

        try:
            self._engine.move_player(direction)
            self._visited_rooms.add(self._engine.player.get_room())
            collected_after = self._engine.player.get_collected_count()

            total = self._engine.get_total_treasure_count()

            if collected_after > collected_before:
                if collected_after == total:
                    self._victory = True
                    self._message = (
                        f"You picked up the FINAL treasure! "
                        f"({collected_after}/{total}) - YOU WIN!"
                    )
                else:
                    self._message = (
                        f"You picked up a treasure! ({collected_after}/{total})"
                    )
            else:
                self._message = "Collect the treasure fast, the clock is ticking!"

        except ImpassableError:
            self._message = "You can't go that way."
        except GameError as e:
            self._message = f"Error: {e}"

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
        except GameError as e:
            self._message = f"Error: {e}"

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
            adj  = random.choice(self._adjectives)
            noun = random.choice(self._nouns)
            self._room_names[room_id] = f"{adj} {noun}"
        return self._room_names[room_id]

    # ------------------------------------------------------------------
    # Splash / end screens
    # ------------------------------------------------------------------

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
            f"  Games played:            {self._profile.get('games_played', 0)}",
            f"  Max treasures collected: {self._profile.get('max_treasure_collected', 0)}",
            f"  Most rooms completed:    {self._profile.get('most_rooms_world_completed', 0)}",
            f"  Last played:             {self._profile.get('timestamp_last_played', 'Never')}",
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
            f"  Player:                       {self._profile.get('player_name', 'Player')}",
            f"  Treasures collected this run: {collected}",
            f"  Games played:                 {self._profile.get('games_played', 0) + 1}",
            f"  All-time best:                {max(collected, self._profile.get('max_treasure_collected', 0))}",
            f"  Most rooms completed:         {max(len(self._visited_rooms), self._profile.get('most_rooms_world_completed', 0))}",
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

    def _victory_screen(self, stdscr) -> None:
        """Show victory screen when all treasure is collected."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        row = 0

        collected = self._engine.player.get_collected_count()
        total     = self._engine.get_total_treasure_count()

        elapsed       = datetime.datetime.utcnow() - self._start_time
        total_seconds = int(elapsed.total_seconds())
        time_str      = f"{total_seconds // 60}m {total_seconds % 60}s"

        lines = [
            "=" * min(40, width - 1),
            "  YOU WIN!",
            "=" * min(40, width - 1),
            "",
            f"  Player:                  {self._profile.get('player_name', 'Player')}",
            f"  Treasure:                {collected}/{total}",
            f"  Games played:            {self._profile.get('games_played', 0) + 1}",
            f"  Max treasures collected: {max(collected, self._profile.get('max_treasure_collected', 0))}",
            f"  Most rooms completed:    {max(len(self._visited_rooms), self._profile.get('most_rooms_world_completed', 0))}",
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

    # ------------------------------------------------------------------
    # Drawing
    # ------------------------------------------------------------------

    def _draw(self, stdscr) -> None:
        """Render the full UI: message bar, room, controls, status bar."""
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        row = 0

        # message bar (row 0)
        self._draw_message_bar(stdscr, row, width)
        row += 1

        # room name line (row 1)
        room_id   = self._engine.player.get_room()
        room_name = self._get_room_name(room_id)
        self._safe_addstr(stdscr, row, 0, f"Room {room_id}: {room_name}"[:width - 1])
        row += 1

        # room render
        room_str   = self._engine.render_current_room()
        room_lines = room_str.splitlines()
        legend_col = 40

        for i, line in enumerate(room_lines):
            if row >= height - 3:
                break

            # draw each character with appropriate color
            for col, ch in enumerate(line[:width - 1]):
                if ch == '@':
                    # player - cyan
                    stdscr.addstr(row, col, ch, curses.color_pair(2))
                elif ch == '$':
                    # treasure - yellow
                    stdscr.addstr(row, col, ch, curses.color_pair(1))
                elif ch == '#':
                    # wall - dim white
                    stdscr.addstr(row, col, ch, curses.color_pair(3) | curses.A_DIM)
                elif ch == 'X':
                    # open portal - green
                    stdscr.addstr(row, col, ch, curses.color_pair(4))
                elif ch == LOCKED_PORTAL_SENTINEL:
                    # locked portal sentinel from C: display as X in red
                    stdscr.addstr(row, col, 'X', curses.color_pair(6))
                elif ch == 'O':
                    # pushable block - magenta
                    stdscr.addstr(row, col, ch, curses.color_pair(5))
                elif ch == '=':
                    # switch unpressed - dim magenta
                    stdscr.addstr(row, col, ch, curses.color_pair(7) | curses.A_DIM)
                elif ch == '+':
                    # switch pressed (pushable consumed onto switch) - green
                    stdscr.addstr(row, col, ch, curses.color_pair(8))
                else:
                    stdscr.addstr(row, col, ch)

            # legend column alongside the room
            if   i == 0: self._safe_addstr(stdscr, row, legend_col, "Game Elements:")
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
                self._safe_addstr(stdscr, row, legend_col + 2, "- portal (open)")
            elif i == 5:
                stdscr.addstr(row, legend_col, "X", curses.color_pair(6))
                self._safe_addstr(stdscr, row, legend_col + 2, "- portal (locked)")
            elif i == 6:
                stdscr.addstr(row, legend_col, "O", curses.color_pair(5))
                self._safe_addstr(stdscr, row, legend_col + 2, "- pushable block")
            elif i == 7:
                stdscr.addstr(row, legend_col, "=", curses.color_pair(7))
                self._safe_addstr(stdscr, row, legend_col + 2, "- switch (unpressed)")
            elif i == 8:
                stdscr.addstr(row, legend_col, "+", curses.color_pair(8))
                self._safe_addstr(stdscr, row, legend_col + 2, "- switch (pressed)")

            row += 1

        # minimap (right of legend)
        minimap_col = legend_col + 25
        self._safe_addstr(stdscr, 1, minimap_col, "Minimap:")
        self._draw_minimap(stdscr, 2, minimap_col)

        # controls legend
        controls = "Controls: WASD/Arrows=move  >=portal  r=reset  q=quit  Push O onto = to unlock red X"
        if row < height - 2:
            self._safe_addstr(stdscr, row, 0, controls[:width - 1])
            row += 1

        # player status bar
        self._draw_status_bar(stdscr, height - 2, width)

        # title / email
        title = "Treasure Runner  |  ckowal02@uoguelph.ca"
        self._safe_addstr(stdscr, height - 1, 0, title[:width - 1])

        stdscr.refresh()

    def _draw_message_bar(self, stdscr, row: int, width: int) -> None:
        """Render the message bar at the given row."""
        msg    = self._message or ""
        padded = msg.ljust(width - 1)
        self._safe_addstr(stdscr, row, 0, padded[:width - 1])

    def _draw_status_bar(self, stdscr, row: int, width: int) -> None:
        """Render player stats at the given row."""
        player    = self._engine.player
        collected = player.get_collected_count()
        total     = self._engine.get_total_treasure_count()
        room_id   = player.get_room()
        name      = self._profile.get("player_name", "Player")
        x, y      = player.get_position()
        status = (
            f"Player: {name}  |  "
            f"Room: {room_id}  |  "
            f"Pos: ({x},{y})  |  "
            f"Treasures: {collected}/{total}"
        )
        self._safe_addstr(stdscr, row, 0, status[:width - 1])

    def _draw_minimap(self, stdscr, start_row: int, start_col: int) -> None:
        """Draw the room adjacency minimap with correct room-ID colouring.

        Room IDs are NOT guaranteed to be 0-based indices, so we fetch the
        real ID list from the engine and use that to map matrix index to room ID.
        """
        matrix   = self._engine.get_adjacency_matrix()
        room_ids = self._engine.get_room_ids()   # actual room IDs in graph order
        n        = len(matrix)
        current  = self._engine.player.get_room()

        for i in range(n):
            room_id = room_ids[i]

            # colour based on actual room ID, not loop index
            if room_id == current:
                attr = curses.color_pair(2)   # cyan  = player's current room
            elif room_id in self._visited_rooms:
                attr = curses.color_pair(4)   # green = visited room
            else:
                attr = 0                       # default = unvisited

            # build neighbour list using real room IDs
            connections = [str(room_ids[j]) for j in range(n) if matrix[i][j] == 1]
            conn_str    = ", ".join(connections)
            line        = f"[{room_id}]-> {conn_str}"

            self._safe_addstr(stdscr, start_row + i, start_col, line, attr)

    # ------------------------------------------------------------------
    # Profile update
    # ------------------------------------------------------------------

    def _update_and_save_profile(self) -> None:
        """Update profile stats after a game run and persist to disk."""
        collected     = self._engine.player.get_collected_count()
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

        self._profile["timestamp_last_played"] = (
            datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
        )

        self._save_fn(self._profile_path, self._profile)

    # ------------------------------------------------------------------
    # Utilities
    # ------------------------------------------------------------------

    def _safe_addstr(self, stdscr, row, col, text, attr=0) -> None:
        try:
            if attr:
                stdscr.addstr(row, col, text, attr)
            else:
                stdscr.addstr(row, col, text)
        except curses.error:
            pass

    def _safe_addch(self, stdscr, row, col, ch, attr=0) -> None:
        try:
            stdscr.addch(row, col, ch, attr)
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