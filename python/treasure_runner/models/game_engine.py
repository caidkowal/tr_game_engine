import ctypes
from treasure_runner.bindings.bindings import lib, Status, Direction, GameEngine as GameEnginePtr
from treasure_runner.models.player import Player
from treasure_runner.models.exceptions import status_to_exception


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

    def move_player(self, direction: Direction) -> None:
        """Move the player in the given direction."""
        status = lib.game_engine_move_player(self._eng, direction)
        if status != Status.OK:
            raise status_to_exception(status, f"move_player failed with direction {direction}")

    def render_current_room(self) -> str:
        str_ptr = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(str_ptr))
        if status != Status.OK:
            raise status_to_exception(status, "render_current_room failed")

        #decode the C string to Python string
        result = str_ptr.value.decode("utf-8")

        #free the C-allocated memory
        lib.game_engine_free_string(str_ptr)

        return result

    def get_room_count(self) -> int:
        count = ctypes.c_int()
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "get_room_count failed")
        return count.value

    def get_room_dimensions(self) -> tuple[int, int]:
        width = ctypes.c_int()
        height = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(width), ctypes.byref(height))
        if status != Status.OK:
            raise status_to_exception(status, "get_room_dimensions failed")
        return (width.value, height.value)

    def get_room_ids(self) -> list[int]:
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()
        status = lib.game_engine_get_room_ids(self._eng, ctypes.byref(ids_ptr), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "get_room_ids failed")

        #copy IDs from C array to Python list
        result = [ids_ptr[i] for i in range(count.value)]

        #free the C-allocated array
        lib.game_engine_free_string(ids_ptr)

        return result

    def reset(self) -> None:
        """Reset the game to its initial state."""
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "reset failed")
