import os
import unittest
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import ImpassableError
from treasure_runner.bindings.bindings import Direction, RoomTileType

CONFIG_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../assets/starter.ini"))

#test comment
class TestGameEngine(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.engine.destroy()

    def test_create_succeeds(self):
        self.assertIsNotNone(self.engine)

    def test_player_not_none(self):
        self.assertIsNotNone(self.engine.player)

    def test_destroy_safe_to_call_twice(self):
        self.engine.destroy()
        self.engine.destroy()

    def test_get_room_count_returns_int(self):
        count = self.engine.get_room_count()
        self.assertIsInstance(count, int)
        self.assertGreaterEqual(count, 1)

    def test_render_current_room_returns_string(self):
        room = self.engine.render_current_room()
        self.assertIsInstance(room, str)

    def test_get_room_dimensions_returns_tuple_of_ints(self):
        width, height = self.engine.get_room_dimensions()
        self.assertIsInstance(width, int)
        self.assertIsInstance(height, int)

    def test_ckowal_peek_tile_returns_room_tile_type(self):
        """peek_tile returns a valid RoomTileType for every direction."""
        for direction in [Direction.NORTH, Direction.SOUTH, Direction.EAST, Direction.WEST]:
            tile = self.engine.peek_tile(direction)
            self.assertIsInstance(tile, RoomTileType)

    def test_ckowal_total_treasure_count_is_non_negative(self):
        """get_total_treasure_count returns a non-negative integer."""
        total = self.engine.get_total_treasure_count()
        self.assertIsInstance(total, int)
        self.assertGreaterEqual(total, 0)

    def test_ckowal_room_ids_length_matches_room_count_and_matrix(self):
        """get_room_ids, get_adjacency_matrix, and reset all complete without error."""
        room_ids = self.engine.get_room_ids()
        self.assertIsInstance(room_ids, list)
        self.assertEqual(len(room_ids), self.engine.get_room_count())

        matrix = self.engine.get_adjacency_matrix()
        self.assertIsInstance(matrix, list)
        self.assertEqual(len(matrix), len(room_ids))
        for row in matrix:
            self.assertEqual(len(row), len(room_ids))

        # reset should complete without raising
        self.engine.reset()
        self.assertIsNotNone(self.engine.player)


if __name__ == "__main__":
    unittest.main()
