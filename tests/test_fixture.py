from __future__ import print_function
import logrun
import deepfuzzy_base


class FixtureTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/Fixture"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Passed: MyTest_Something" in output)
    self.assertFalse("Failed: MyTest_Something" in output)

    self.assertTrue("Setting up!" in output)
    self.assertTrue("Tearing down!" in output)
