import ctypes
from treasure_runner.bindings.bindings import lib, Treasure

class Player:
    def __init__(self, ptr):
        self._ptr = ptr

    def get_room(self) -> int:
        return lib.player_get_room(self._ptr)

    def get_position(self) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)

    def get_collected_count(self) -> int:
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list[dict]:
        count = ctypes.c_int()
        treasures = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))
        result = []
        for i in range(count.value):
            treasure_obj = treasures[i].contents
            result.append({
                "id": treasure_obj.id,
                "name": treasure_obj.name.decode("utf-8") if treasure_obj.name else None,
                "starting_room_id": treasure_obj.starting_room_id,
                "initial_x": treasure_obj.initial_x,
                "initial_y": treasure_obj.initial_y,
                "x": treasure_obj.x,
                "y": treasure_obj.y,
                "collected": treasure_obj.collected,
            })
        return result
        