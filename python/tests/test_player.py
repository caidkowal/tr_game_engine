import os
import unittest

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.player import Player

CONFIG_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "../../assets/starter.ini")
)


class TestPlayer(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine(CONFIG_PATH)
        self.player = self.engine.player

    def tearDown(self):
        self.engine.destroy()

    def test_player_is_instance(self):
        self.assertIsInstance(self.player, Player)

    def test_get_room_returns_int(self):
        self.assertIsInstance(self.player.get_room(), int)

    def test_get_position_values_are_ints(self):
        x, y = self.player.get_position()

        self.assertIsInstance(x, int)
        self.assertIsInstance(y, int)

    def test_get_collected_count_returns_int(self):
        count = self.player.get_collected_count()
        self.assertIsInstance(count, int)

    def test_initial_collected_count_is_non_negative(self):
        count = self.player.get_collected_count()
        self.assertGreaterEqual(count, 0)

    def test_get_collected_treasures_returns_list(self):
        treasures = self.player.get_collected_treasures()
        self.assertIsInstance(treasures, list)

    def test_collected_treasures_structure(self):
        treasures = self.player.get_collected_treasures()

        if treasures:
            treasure = treasures[0]

            self.assertIn("id", treasure)
            self.assertIn("name", treasure)
            self.assertIn("starting_room_id", treasure)
            self.assertIn("initial_x", treasure)
            self.assertIn("initial_y", treasure)
            self.assertIn("x", treasure)
            self.assertIn("y", treasure)
            self.assertIn("collected", treasure)

    def test_has_collected_treasure_returns_bool(self):
        result = self.player.has_collected_treasure(0)
        self.assertIsInstance(result, bool)

    def test_get_position_returns_tuple_length_2(self):
    pos = self.player.get_position()
    self.assertEqual(len(pos), 2)

    def test_collected_count_matches_list_length(self):
        count = self.player.get_collected_count()
        treasures = self.player.get_collected_treasures()
        self.assertEqual(count, len(treasures))

    def test_has_collected_invalid_id_returns_bool(self):
        # Using a large ID that likely doesn't exist
        result = self.player.has_collected_treasure(9999)
        self.assertIsInstance(result, bool)


if __name__ == "__main__":
    unittest.main()