import os
import unittest
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import ImpassableError
from treasure_runner.bindings.bindings import Direction

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


if __name__ == "__main__":
    unittest.main()