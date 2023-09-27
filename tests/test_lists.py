from __future__ import print_function
import logrun
import deepfuzzy_base


class ListsTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    if deepfuzzy == "deepfuzzy-figurative":
       return # Just skip for now, we know it's too slow
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/Lists"],
                  "deepfuzzy.out", 3000)
    self.assertEqual(r, 0)

    self.assertTrue("Passed: Vector_DoubleReversal" in output)
    self.assertFalse("Failed: Vector_DoubleReversal" in output)
