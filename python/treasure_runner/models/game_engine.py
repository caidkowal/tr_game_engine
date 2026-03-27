import ctypes
from treasure_runner.bindings.bindings import lib, Status, Direction, RoomTileType, GameEngine as GameEnginePtr
from treasure_runner.models.player import Player
from treasure_runner.models.exceptions import status_to_exception, NoPortalError, ImpassableError


class GameEngine:

    def __init__(self, config_path: str):
        self._destroyed = False
        self._eng = None
        self._player = None

        eng = GameEnginePtr()
        status = lib.game_engine_create(config_path.encode("utf-8"), ctypes.byref(eng))

        if status != Status.OK:
            raise status_to_exception(status, "game_engine_create failed")
        self._eng = eng

        player_ptr = lib.game_engine_get_player(self._eng)
        if player_ptr is None:
            raise RuntimeError("game_engine_get_player returned NULL")
        self._player = Player(player_ptr)

    @property
    def player(self) -> Player:
        return self._player

    def destroy(self) -> None:
        if not self._destroyed and self._eng is not None:
            lib.game_engine_destroy(self._eng)
            self._destroyed = True

    def __del__(self):
        self.destroy()

    # ------------------------------------------------------------------
    # Movement
    # ------------------------------------------------------------------

    def move_player(self, direction: Direction) -> None:
        """Move the player in the given direction using normal movement logic.

        Handles walls, treasures, pushables, and automatic portal traversal
        (A2-compatible behaviour). For explicit portal control use use_portal().
        """
        status = lib.game_engine_move_player(self._eng, direction)
        if status != Status.OK:
            raise status_to_exception(status, f"move_player failed with direction {direction}")

    def peek_tile(self, direction: Direction) -> RoomTileType:
        """Return the tile type one step ahead WITHOUT moving the player.

        Used by the UI to decide whether the next tile is a portal before
        committing to a move.
        """
        tile_type = ctypes.c_int()
        status = lib.game_engine_peek_tile(self._eng, int(direction), ctypes.byref(tile_type))
        if status != Status.OK:
            raise status_to_exception(status, "peek_tile failed")
        return RoomTileType(tile_type.value)

    def set_player_position(self, x: int, y: int) -> None:
        """Set the player position directly, bypassing all movement logic.

        Used to step the player onto a portal tile without triggering
        automatic portal traversal.
        """
        status = lib.game_engine_set_player_position(self._eng, x, y)
        if status != Status.OK:
            raise status_to_exception(status, "set_player_position failed")

    def use_portal(self) -> None:
        """Traverse the portal the player is currently standing on.

        Raises:
            NoPortalError    - player is not on a portal tile
            ImpassableError  - portal is locked (switch not pressed)
            NoSuchRoomError  - destination room not found
        """
        status = lib.game_engine_use_portal(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "use_portal failed")

    # ------------------------------------------------------------------
    # Rendering
    # ------------------------------------------------------------------

    def render_current_room(self) -> str:
        str_ptr = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(str_ptr))
        if status != Status.OK:
            raise status_to_exception(status, "render_current_room failed")

        result = str_ptr.value.decode("utf-8")
        lib.game_engine_free_string(str_ptr)
        return result

    # ------------------------------------------------------------------
    # Queries
    # ------------------------------------------------------------------

    def get_room_count(self) -> int:
        count = ctypes.c_int()
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "get_room_count failed")
        return count.value

    def get_room_dimensions(self) -> tuple:
        width = ctypes.c_int()
        height = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(
            self._eng, ctypes.byref(width), ctypes.byref(height)
        )
        if status != Status.OK:
            raise status_to_exception(status, "get_room_dimensions failed")
        return (width.value, height.value)

    def get_room_ids(self) -> list:
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()
        status = lib.game_engine_get_room_ids(
            self._eng, ctypes.byref(ids_ptr), ctypes.byref(count)
        )
        if status != Status.OK:
            raise status_to_exception(status, "get_room_ids failed")

        result = [ids_ptr[i] for i in range(count.value)]
        lib.game_engine_free_string(ids_ptr)
        return result

    def get_total_treasure_count(self) -> int:
        count = ctypes.c_int()
        status = lib.game_engine_get_total_treasure_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "get_total_treasure_count failed")
        return count.value

    def get_adjacency_matrix(self) -> list:
        matrix_ptr = ctypes.POINTER(ctypes.c_int)()
        size = ctypes.c_int()
        status = lib.game_engine_get_adjacency_matrix(
            self._eng, ctypes.byref(matrix_ptr), ctypes.byref(size)
        )
        if status != Status.OK:
            raise status_to_exception(status, "get_adjacency_matrix failed")

        room_count = size.value
        matrix = [[matrix_ptr[i * room_count + j] for j in range(room_count)]
                  for i in range(room_count)]
        lib.game_engine_free_string(matrix_ptr)
        return matrix

    # ------------------------------------------------------------------
    # Reset
    # ------------------------------------------------------------------

    def reset(self) -> None:
        """Reset the game to its initial state."""
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "reset failed")

    # ------------------------------------------------------------------
    # Memory
    # ------------------------------------------------------------------

    def free_string(self, ptr) -> None:
        lib.game_engine_free_string(ptr)