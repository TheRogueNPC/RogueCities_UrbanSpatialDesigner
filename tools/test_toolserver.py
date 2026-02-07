import os
import unittest
import importlib.util
from pathlib import Path


def load_toolserver_module():
    toolserver_path = Path(__file__).with_name("toolserver.py")
    spec = importlib.util.spec_from_file_location("roguecity_toolserver", toolserver_path)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    spec.loader.exec_module(module)
    return module


class ToolserverEndpointTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ["ROGUECITY_TOOLSERVER_MOCK"] = "1"
        cls.toolserver = load_toolserver_module()
        from fastapi.testclient import TestClient

        cls.client = TestClient(cls.toolserver.app)

    def test_health(self):
        r = self.client.get("/health")
        self.assertEqual(r.status_code, 200)
        j = r.json()
        self.assertEqual(j.get("status"), "ok")

    def test_ui_agent_mock(self):
        payload = {"snapshot": {"app": "test"}, "goal": "dock tools", "model": "x"}
        r = self.client.post("/ui_agent", json=payload)
        self.assertEqual(r.status_code, 200)
        j = r.json()
        self.assertTrue(j.get("mock"))
        self.assertIsInstance(j.get("commands"), list)
        self.assertGreaterEqual(len(j["commands"]), 1)
        self.assertIn("cmd", j["commands"][0])

    def test_city_spec_mock(self):
        payload = {"description": "A city", "constraints": {"scale": "city"}, "model": "x"}
        r = self.client.post("/city_spec", json=payload)
        self.assertEqual(r.status_code, 200)
        j = r.json()
        self.assertTrue(j.get("mock"))
        self.assertIn("spec", j)
        self.assertIn("intent", j["spec"])
        self.assertEqual(j["spec"]["intent"].get("scale"), "city")


if __name__ == "__main__":
    unittest.main()

